#
#  Routines to help *experiment* (not necessarily productize) CA certificates.
#

# This will make PowerShell complain more on unsafe practices
Set-StrictMode -Version 2.0

#
#  Globals
#

# Errors in system routines will stop script execution
$errorActionPreference       = "stop"

$_basePath                   = "."
$_rootCertCommonName         = "Azure IoT CA TestOnly Root CA"
$_rootCertSubject            = "`"/CN=$_rootCertCommonName`""
$_rootCAPrefix               = "azure-iot-test-only.root.ca"
$_intermediateCertCommonName = "Azure IoT CA TestOnly Intermediate {0} CA"
$_intermediate1CommonName    = ($_intermediateCertCommonName -f "1")
$_intermediate1Prefix        = "azure-iot-test-only.intermediate1"
$_privateKeyPassword         = "1234"
$_keyBitsLength              = "4096"
$_days_until_expiration      = 30
$_opensslRootConfigFile      = "$_basePath/openssl_root_ca.cnf"
$_opensslIntermediateConfigFile = "$_basePath/openssl_device_intermediate_ca.cnf"
$_keySuffix                  = "key.pem"
$_certSuffix                 = "cert.pem"
$_csrSuffix                  = "csr.pem"
# Whether to use ECC or RSA is stored in a file.  If it doesn't exist, we default to ECC.
$algorithmUsedFile           = "./algorithmUsed.txt"

<#
    .SYNOPSIS
        Invoke an external OS command using PowerShell semantics.
    .DESCRIPTION
        The Invoke-External cmdlet enables the synchronous execution of external commands. The external
        error stream is automatically redirected into the external output stream. Execution information is
        written to the verbose stream. If a nonzero exit code is returned from the external command, the
        output string is thrown as an exception.
    .PARAMETER Command
        The command to execute.
    .PARAMETER Passthru
        Return the output string for each executed command.
#>
function Invoke-External()
{
    #Requires -Version 5
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true, ValueFromPipeline = $true)]
        [String] $Command,

        [Switch] $Passthru
    )

    process {
        $output = $null
        Write-Verbose "Executing: $Command"
        cmd /c "$Command 2>&1" 2>&1 | Tee-Object -Variable "output" | Write-Verbose
        Write-Verbose "Exit code: $LASTEXITCODE"

        if ($LASTEXITCODE) {
            throw $output
        } elseif ($Passthru) {
            $output
        }
    }
}

<#
    .SYNOPSIS
        Helper function to obtain a path to a certificate given its prefix.
    .DESCRIPTION
        Helper function to obtain a path to a certificate given its prefix.
        A certificate file is created using the following scheme ./certs/$prefix.cert.pem.
#>
function Get-CertPathForPrefix([string]$prefix)
{
    return "$_basePath/certs/$prefix.$_certSuffix"
}

<#
    .SYNOPSIS
        Helper function to obtain a path to a key file given its prefix.
    .DESCRIPTION
        Helper function to obtain a path to the certificate given its prefix.
        A certificate file is created using the following scheme ./private/$prefix.key.pem.
#>
function Get-KeyPathForPrefix([string]$prefix)
{
    return "$_basePath/private/$prefix.$_keySuffix"
}

<#
    .SYNOPSIS
        Helper function to obtain a path to a CSR file given its prefix.
    .DESCRIPTION
        Helper function to obtain a path to the certificate signing request (CSR)
        given its prefix. A certificate file is created using the following
        scheme ./csr/$prefix.csr.pem.
#>
function Get-CSRPathForPrefix([string]$prefix)
{
    return "$_basePath/csr/$prefix.$_csrSuffix"
}

<#
    .SYNOPSIS
    Function to check if there is already a cert of the root CA present in the certificate store.
    .DESCRIPTION
    This function detects if a the root CA certifcate is already installed in the cert store
    and warns the user to clean it up since they might be installing newer ones after
    executing this script.
    .PARAMETER printMsg
    An optional bool that controls if additional diagnotics messages are to be printed or not.
#>
function Test-CACertNotInstalledAlready([bool]$printMsg=$true)
{
    if ($TRUE -eq $printMsg)
    {
        Write-Host ("Testing if any test root certificates have already been installed...")
    }
    $certInstalled = $null
    try
    {
        $certInstalled = Get-CACertsCertBySubjectName $_rootCertSubject
    }
    catch
    {

    }

    if ($NULL -ne $certInstalled)
    {
        $nl = [Environment]::NewLine
        $cleanup_msg  = "$nl$nl"
        $cleanup_msg += "This utility detected test certificates already installed in the certificate store.$nl"
        $cleanup_msg += "Since newer root CA certificates will be generated, it is recommended to clean any stale root certificates.$nl"
        $cleanup_msg += "Steps to cleanup, from the 'Start' menu, type 'open manage computer certificates':$nl"
        $cleanup_msg += " - Navigate to Certificates -> Trusted Root Certification Authority -> Certificates. Remove certificates issued by 'Azure IoT CA TestOnly*'.$nl"
        $cleanup_msg += " - Navigate to Certificates -> Intermediate Certificate Authorities -> Certificates. Remove certificates issued by 'Azure IoT CA TestOnly*'.$nl"
        $cleanup_msg += " - Navigate to Certificates -> Local Computer -> Personal. Remove certificates issued by 'Azure IoT CA TestOnly*'.$nl"
        $cleanup_msg += "$nl$nl"
        Write-Warning("Certificate {0} already installed in the certificate store. {1}" -f $_rootCertSubject,  $cleanup_msg)
        throw ("Certificate {0} already installed." -f $_rootCertSubject)
    }
    if ($TRUE -eq $printMsg)
    {
        Write-Host ("  Ok.")
    }
}


<#
    .SYNOPSIS
    Verify that the prerequisites for this script are met.
    .DESCRIPTION
    Helper function verify that the prerequisites dependencies for this script are met
    .PARAMETER printMsg
        An optional bool that controls if additional diagnotics messages are to be printed or not.
#>
function Test-CACertsPrerequisites([bool]$printMsg=$true)
{
    Test-CACertNotInstalledAlready($printMsg)
    if ($TRUE -eq $printMsg)
    {
        Write-Host ("Testing if openssl.exe is set in PATH...")
    }
    if ((Get-Command "openssl.exe" -ErrorAction SilentlyContinue) -eq $NULL)
    {
        throw ("Openssl is unavailable. Please install openssl and set it in the PATH before proceeding.")
    }
    if ($TRUE -eq $printMsg)
    {
        Write-Host ("  Ok.")
        Write-Host ("Success")
    }
}

<#
    .SYNOPSIS
    Prepare the nessary files and directories to hold the resulting certificates.
    .DESCRIPTION
    Prepare the nessary files and directories to hold the resulting certificates. This will be
    called if a new Root CA is being created thus wiping out any prior generated certificates.
#>
function PrepareFilesystem()
{
    Remove-Item -Path $_basePath/csr -Recurse
    Remove-Item -Path $_basePath/private -Recurse
    Remove-Item -Path $_basePath/certs -Recurse
    Remove-Item -Path $_basePath/intermediateCerts
    Remove-Item -Path $_basePath/newcerts -Recurse

    mkdir -Path "csr"
    mkdir -Path "private"
    mkdir -Path "certs"
    mkdir -Path "intermediateCerts"
    mkdir -Path "newcerts"

    Remove-Item -Path $_basePath/index.txt
    New-Item $_basePath/index.txt -ItemType file

    Remove-Item -Path $_basePath/serial -ErrorAction Ignore
    New-Item $_basePath/serial -ItemType file
    "1000`n" | Out-File -NoNewline -Encoding ASCII $_basePath/serial
}

<#
    .SYNOPSIS
    Creates an asymmetric private key to be used for certificate generation.
    .DESCRIPTION
    Generate an ECC or RSA key with an optional passphrase. The choice of what type of key
    to create is determined by function Get-CACertsCertUseRSA(). The private key file
    will be saved to the private dir and named using the provided prefix.
    .PARAMETER prefix
        Prefix is used to name the file containing the private key
    .PARAMETER keyPass
        Optional passphrase used to encrypt the private key
#>
function New-PrivateKey([string]$prefix, [string]$keyPass=$NULL)
{
    Write-Host ("Creating the $prefix private Key")

    $passwordCreateCmd = ""
    if ($keypass -ne $NULL)
    {
        $passwordCreateCmd = "-aes256 -passout pass:$keyPass"
    }

    $algorithm = ""
    $cmdEpilog = ""
    if ((Get-CACertsCertUseRSA) -eq $TRUE)
    {
        $algorithm = "genrsa"
        $cmdEpilog = "$_keyBitsLength"
    }
    else
    {
        $algorithm = "ecparam -genkey -name secp256k1"
        $passwordCreateCmd = " | openssl ec $passwordCreateCmd"
        $cmdEpilog = ""
    }

    $keyFile = Get-KeyPathForPrefix($prefix)
    $cmd = "openssl $algorithm $passwordCreateCmd -out $keyFile $cmdEpilog"
    Invoke-External -verbose $cmd
    return $keyFile
}

<#
    .SYNOPSIS
    Generate a certificate using an issuer certificate and key. Certificates could be
    either CA, server and client certificates.
    .DESCRIPTION
    Generate a client certificate using the supplied certificate and key parameters and
    have it signed by an issuer certificate and key.
    Issuer is spcified by its prefix and its corresponding key and certificate
    will be retrieved from the filesystem.
    .PARAMETER x509Ext
        A string to identify which configuration to use to generate a certificate.
    .PARAMETER expirationDays
        Number of days from now to set the certificate expiration timestamp.
    .PARAMETER subject
        Value of the certificate subject to use to generate a certificate.
    .PARAMETER prefix
        Prefix is used to name the file containing the private key
    .PARAMETER issuerPrefix
        Issuer prefix to look up the certifcate and key
    .PARAMETER keyPass
        Optional passphrase used to encrypt the private key
    .PARAMETER issuerKeyPass
        Optional passphrase used to decrypt the issuer private key when signing the certificate.
#>
function New-IntermediateCertificate
(
    [ValidateSet("usr_cert","v3_intermediate_ca","server_cert")][string]$x509Ext,
    [string]$expirationDays,
    [string]$subject,
    [string]$prefix,
    [string]$issuerPrefix,
    [string]$keyPass=$NULL,
    [string]$issuerKeyPass=$NULL
)
{
    $issuerKeyFile = Get-KeyPathForPrefix($issuerPrefix)
    if (-not (Test-Path $issuerKeyFile -PathType Leaf))
    {
        Write-Host ("Private key file not found: $issuerKeyFile")
        throw ("Issuer '$issuerPrefix' private key not found")
    }

    $issuerCertFile = Get-CertPathForPrefix($issuerPrefix)
    if (-not (Test-Path $issuerCertFile -PathType Leaf))
    {
        Write-Host ("Certificate file not found: $issuerCertFile")
        throw ("Issuer '$issuerPrefix' certificate not found")
    }

    Write-Host ("Creating the Intermediate CSR for $prefix")
    Write-Host "-----------------------------------"
    $keyFile = New-PrivateKey $prefix $keyPass

    $keyPassUseCmd = ""
    if ($keyPass -ne $NULL)
    {
        $keyPassUseCmd = "-passin pass:$keyPass"
    }
    $csrFile = Get-CSRPathForPrefix($prefix)

    $cmd =  "openssl req -new -sha256 $keyPassUseCmd "
    $cmd += "-config $_opensslIntermediateConfigFile "
    $cmd += "-subj $subject "
    $cmd += "-key $keyFile "
    $cmd += "-out $csrFile "
    Invoke-External -verbose $cmd

    Write-Host ("Signing the certificate for $prefix with issuer certificate $issuerPrefix")
    Write-Host "-----------------------------------"
    $keyPassUseCmd = ""
    if ($issuerKeyPass -ne $NULL)
    {
        $keyPassUseCmd = "-key $issuerKeyPass"
    }
    $certFile = Get-CertPathForPrefix($prefix)

    $cmd =  "openssl ca -batch "
    $cmd += "-config $_opensslIntermediateConfigFile "
    $cmd += "-extensions $x509Ext "
    $cmd += "-days $expirationDays -notext -md sha256 "
    $cmd += "-cert $issuerCertFile "
    $cmd += "$keyPassUseCmd -keyfile $issuerKeyFile -keyform PEM "
    $cmd += "-in $csrFile -out $certFile "
    Invoke-External -verbose $cmd

    Write-Host ("Verifying the certificate for $prefix with issuer certificate $issuerPrefix")
    Write-Host "-----------------------------------"
    $rootCertFile = Get-CertPathForPrefix($_rootCAPrefix)
    $cmd =  "openssl verify -CAfile $rootCertFile -untrusted $issuerCertFile $certFile"
    Invoke-External -verbose $cmd

    Write-Host ("Certificate for prefix $prefix generated at:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    $cmd = "openssl x509 -noout -text -in $certFile"
    Invoke-External $cmd

    return $certFile
}

<#
    .SYNOPSIS
    Generate a client certificate using an issuer certificate and key
    .DESCRIPTION
    Generate a client certificate using the supplied common name and have it signed by
    an issuer certificate and key. Issuer is spcified by its prefix and its corresponding
    key and certificate will be retrieved from the filesystem.
    .PARAMETER prefix
        Prefix is used to name the file containing the private key
    .PARAMETER issuerPrefix
        Issuer prefix to look up the certifcate and key
    .PARAMETER commonName
        Value of the CN field to set when generating the certifcate
#>
function New-ClientCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "usr_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

<#
    .SYNOPSIS
    Generate a server certificate using an issuer certificate and key
    .DESCRIPTION
    Generate a server certificate using the supplied common name and have it signed by
    an issuer certificate and key. Issuer is spcified by its prefix and its corresponding
    key and certificate will be retrieved from the filesystem.
    .PARAMETER prefix
        Prefix is used to name the file containing the private key
    .PARAMETER issuerPrefix
        Issuer prefix to look up the certifcate and key
    .PARAMETER commonName
        Value of the CN field to set when generating the certifcate
#>
function New-ServerCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "server_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

<#
    .SYNOPSIS
    Generate an intermediate CA certificate using an issuer certificate and key
    .DESCRIPTION
    Generate an intermediate CA certificate using the supplied common name and have it signed by
    an issuer certificate and key. Issuer is spcified by its prefix and its corresponding
    key and certificate will be retrieved from the filesystem.
    .PARAMETER prefix
        Prefix is used to name the file containing the private key
    .PARAMETER issuerPrefix
        Issuer prefix to look up the certifcate and key
    .PARAMETER commonName
        Value of the CN field to set when generating the certifcate
#>
function New-IntermediateCACertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "v3_intermediate_ca" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

<#
    .SYNOPSIS
    Generate a root CA certificate
    .DESCRIPTION
    Generate a root CA certificate using the parameters specified in the script.
#>
function New-RootCACertificate()
{
    Write-Host ("Creating the Root CA private key")
    $keyFile = New-PrivateKey $_rootCAPrefix $_privateKeyPassword
    $certFile = Get-CertPathForPrefix($_rootCAPrefix)

    Write-Host ("Creating the Root CA certificate")
    $passwordUseCmd = "-passin pass:$_privateKeyPassword"
    $cmd =  "openssl req -new -x509 -config $_opensslRootConfigFile $passwordUseCmd "
    $cmd += "-key $keyFile -subj $_rootCertSubject -days $_days_until_expiration "
    $cmd += "-sha256 -extensions v3_ca -out $certFile "
    Invoke-External -verbose $cmd

    Write-Host ("CA Root Certificate Generated At:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    $cmd = "openssl x509 -noout -text -in $certFile"
    Invoke-External $cmd

    # Now use splatting to process this
    Write-Warning ("Generating certificate {0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $_rootCertSubject)

    return $certFile
}

# Creates a new certificate chain.
function New-CACertsCertChain([Parameter(Mandatory=$TRUE)][ValidateSet("rsa","ecc")][string]$algorithm)
{
    Write-Host "Beginning to create certificate chain to you filesystem here $PWD"
    Test-CACertsPrerequisites($FALSE)
    PrepareFilesystem

    # Store the algorithm we're using in a file so later stages always use the same one (without forcing user to keep passing it around)
    Set-Content $algorithmUsedFile $algorithm

    New-RootCACertificate
    New-IntermediateCACertificate $_intermediate1Prefix $_rootCAPrefix $_intermediate1CommonName
    Write-Host "Success"
}

# Get-CACertsCertUseEdge retrieves the algorithm (RSA vs ECC) that was specified during New-CACertsCertChain
function Get-CACertsCertUseRsa()
{
    Write-Output ((Get-Content $algorithmUsedFile -ErrorAction SilentlyContinue) -eq "rsa")
}

<#
    .SYNOPSIS
    Queries the Windows certificate store to check if a certificate is already installed using the
    provided subjectName.
    .DESCRIPTION
    Queries the Windows certificate store to check if a certificate is already installed using the
    provided subjectName.
    .PARAMETER subjectName
        Subject name to be used when querying the certificate store.
#>
function Get-CACertsCertBySubjectName([string]$subjectName)
{
    $certificates = gci -Recurse Cert:\LocalMachine\ |? { $_.gettype().name -eq "X509Certificate2" }
    $cert = $certificates |? { $_.subject -eq $subjectName -and $_.PSParentPath -eq "Microsoft.PowerShell.Security\Certificate::LocalMachine\My" }
    if ($NULL -eq $cert)
    {
        throw ("Unable to find certificate with subjectName {0}" -f $subjectName)
    }

    Write-Output $cert
}

<#
    .SYNOPSIS
    Generate a client certificate to be used for private key possession test for IoT Hub
    .DESCRIPTION
    The verification certificate is client certificate issued by the root CA. The IoT hub
    generated a common name which is to be supplied as parameter requestedCommonName.
    .PARAMETER requestedCommonName
        Common name to be used when generating the IoT Hub certificate.
#>
function New-CACertsVerificationCert([Parameter(Mandatory=$TRUE)][string]$requestedCommonName)
{
    if ([string]::IsNullOrWhiteSpace($requestedCommonName))
    {
        throw "Verification string parameter is required and cannot be null or empty"
    }
    $verificationPrefix = "verifyCert4"
    $requestedCommonName = $requestedCommonName.Trim()
    $verifCertPath = Get-CertPathForPrefix($verificationPrefix)
    $verifKeyPath = Get-KeyPathForPrefix($verificationPrefix)
    Remove-Item -Path $verifCertPath -ErrorAction SilentlyContinue
    Remove-Item -Path $verifKeyPath -ErrorAction SilentlyContinue
    New-ClientCertificate $verificationPrefix $_rootCAPrefix $requestedCommonName
    $verifyRequestedFileName = ".\$verificationPrefix.cer"
    if (-not (Test-Path $verifCertPath))
    {
        throw ("Error: CERT file {0} doesn't exist" -f $verifCertPath)
    }
    Copy-Item -Path $verifCertPath -Destination $verifyRequestedFileName
    if (-not (Test-Path $verifyRequestedFileName))
    {
        throw ("Error: Copying cert file file {0} to {1}" -f $verifCertPath, $verifyRequestedFileName)
    }
    Write-Host ("Certificate with subject CN={0} has been output to {1}" -f $requestedCommonName, (Join-Path (get-location).path $verifyRequestedFileName))
}

<#
    .SYNOPSIS
    Generate an IoT Edge/Hub device certificate using an issuer's certificate and key
    .DESCRIPTION
    Generate a certificate using the supplied common name and have it signed by
    an issuer certificate and key. Issuer is spcified by its prefix and its corresponding
    key and certificate will be retrieved from the filesystem.
    If parameter isEdgeDevice is true the resulting certificate is a CA certificate else a
    client certificate will be created.
    .PARAMETER deviceName
        Device name will be used as a prefix and as the value of the CN field
    .PARAMETER issuerPrefix
        Optional issuer prefix to look up the certifcate and key. If null the root CA is used.
    .PARAMETER isEdgeDevice
        Identifies if this certificate is to be used for Edge runtime. Default is false.
#>
function New-CACertsDevice([Parameter(Mandatory=$TRUE)][string]$deviceName, [string]$issuerPrefix=$_rootCAPrefix, [bool]$isEdgeDevice=$false)
{
    if ([string]::IsNullOrWhiteSpace($deviceName))
    {
        throw "Device name string parameter is required and cannot be null or empty"
    }
    $deviceName = $deviceName.Trim()

    # Certificates for edge devices need to be able to sign other certs.
    if ($isEdgeDevice -eq $true)
    {
        $certFile = New-IntermediateCACertificate $deviceName $issuerPrefix $deviceName
    }
    else
    {
        $certFile = New-ClientCertificate $deviceName $issuerPrefix $deviceName
    }

    Write-Host ("Certificate with subject CN={0} has been output to {1}" -f $deviceName, $certFile)
    return $certFile
}

<#
    .SYNOPSIS
    Generate an IoT Edge device CA certificate, private key and full certificate chain
    using an issuer's certificate and key
    .DESCRIPTION
    Generate a certificate using the supplied common name and have it signed by
    an issuer certificate and key. Issuer is spcified by its prefix and its corresponding
    key and certificate will be retrieved from the filesystem.
    .PARAMETER deviceName
        Device name will be used as a prefix and as the value of the CN field
    .PARAMETER issuerPrefix
        Optional issuer prefix to look up the certifcate and key. If null the default intermediate
        CA certificate is used.
#>
function New-CACertsEdgeDevice([Parameter(Mandatory=$TRUE)][string]$deviceName, [string]$issuerPrefix=$_intermediate1Prefix)
{
    if ([string]::IsNullOrWhiteSpace($deviceName))
    {
        throw "Edge device name string parameter is required and cannot be null or empty"
    }
    $deviceName = $deviceName.Trim()
    $edgeDeviceCertificate = New-CACertsDevice $deviceName $issuerPrefix $true
    $intermediate1CertFileName  = Get-CertPathForPrefix($_intermediate1Prefix)
    $rootCACertFileName  = Get-CertPathForPrefix($_rootCAPrefix)
    $edgeDeviceFullCertChain = Get-CertPathForPrefix("$deviceName-full-chain")

    Get-Content $edgeDeviceCertificate, $intermediate1CertFileName, $rootCACertFileName | Set-Content $edgeDeviceFullCertChain
}

Write-Warning "This script is provided for prototyping only."
Write-Warning "DO NOT USE CERTIFICATES FROM THIS SCRIPT FOR PRODUCTION!"
