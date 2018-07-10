#
#  Routines to help *experiment* (not necessarily productize) CA certificates.
#

# This will make PowerShell complain more on unsafe practices
Set-StrictMode -Version 2.0

#
#  Globals
#

# Errors in system routines will stop script execution
$errorActionPreference    = "stop"

$_basePath                   = "."
$_rootCertCommonName         = "Azure IoT CA TestOnly Root CA"
$_rootCertSubject            = "`"/CN=$_rootCertCommonName`""
$_intermediateCertCommonName = "Azure IoT CA TestOnly Intermediate {0} CA"
#todo remove
$_intermediateCertSubject    = "`"/CN=$_intermediateCertCommonName`""
$_intermediate1Prefix        = "azure-iot-test-only.intermediate1"
$_privateKeyPassword         = "1234"
$_keyBitsLength              = "4096"
$_days_until_expiration      = 30
$_opensslRootConfigFile      = "$_basePath/openssl_root_ca.cnf"
$_opensslIntermediateConfigFile = "$_basePath/openssl_device_intermediate_ca.cnf"

$rootCAPrefix                = "azure-iot-test-only.root.ca"
$keySuffix                   = "key.pem"
$certSuffix                  = "cert.pem"
$csrSuffix                   = "csr.pem"

#todo remove
$rootCAPemFileName          = "./RootCA.pem"
$intermediate1CAPemFileName = "./Intermediate1.pem"

# Variables containing file paths for Edge certificates
$edgeDeviceCertificate      = "$_basePath/certs/new-edge-device.cert.pem"
$edgeDevicePrivateKey       = "$_basePath/private/new-edge-device.key.pem"
$edgeDeviceFullCertChain    = "$_basePath/certs/new-edge-device-full-chain.cert.pem"

# Whether to use ECC or RSA is stored in a file.  If it doesn't exist, we default to ECC.
$algorithmUsedFile       = "./algorithmUsed.txt"

<#
    .SYNOPSIS
        Prints a warning message noting that the certificates generated are not for production
        environments.
    .DESCRIPTION
        Prints a warning message noting that the certificates generated are not for production
        environments.
#>
function Print-WarningCertsNotForProduction()
{
    Write-Warning "This script is provided for prototyping only."
    Write-Warning "DO NOT USE CERTIFICATES FROM THIS SCRIPT FOR PRODUCTION!"
}

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
    return "$_basePath/certs/$prefix.$certSuffix"
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
    return "$_basePath/private/$prefix.$keySuffix"
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
    return "$_basePath/csr/$prefix.$csrSuffix"
}

# The script puts certs into the global certificate store. If there is already a cert of the
# same name present, we're not going to be able to tell the new apart from the old, so error out.
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
    Verify that the prerequisites for this script are met
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

function New-PrivateKey([string]$prefix, [string]$keyPass=$NULL)
{
    Write-Host ("Creating the $prefix private Key")

    $passwordCreateCmd = ""
    if ($keypass -ne $NULL)
    {
        $passwordCreateCmd = "-aes256 -passout pass:$keyPass"
    }

    $algorithm = ""
    if ((Get-CACertsCertUseRSA) -eq $TRUE)
    {
        $algorithm = "genrsa $_keyBitsLength"
    }
    else
    {
        $algorithm = "ecparam -genkey -name secp256k1"
        $passwordCreateCmd = " | openssl ec $passwordCreateCmd"
    }

    $keyFile = Get-KeyPathForPrefix($prefix)
    $cmd = "openssl $algorithm $passwordCreateCmd -out $keyFile"
    Invoke-External -verbose $cmd
    return $keyFile
}

function New-IntermediateCertificate
(
    [string]$x509Ext,
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
    $rootCertFile = Get-CertPathForPrefix($rootCAPrefix)
    $cmd =  "openssl verify -CAfile $rootCertFile -untrusted $issuerCertFile $certFile"
    Invoke-External -verbose $cmd

    Write-Host ("Certificate for prefix $prefix generated at:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    $cmd = "openssl x509 -noout -text -in $certFile"
    Invoke-External $cmd

    return $certFile
}

function New-ClientCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "usr_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

function New-ServerCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "server_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

function New-IntermediateCACertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "`"/CN=$commonName`""
    $certFile = New-IntermediateCertificate "v3_intermediate_ca" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    return $certFile
}

function New-RootCACertificate()
{
    Write-Host ("Creating the Root CA private key")
    $keyFile = New-PrivateKey $rootCAPrefix $_privateKeyPassword
    $certFile = Get-CertPathForPrefix($rootCAPrefix)

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
    New-IntermediateCACertificate $_intermediate1Prefix $rootCAPrefix ($_intermediateCertCommonName -f "1")
    Write-Host "Success"
}

# Get-CACertsCertUseEdge retrieves the algorithm (RSA vs ECC) that was specified during New-CACertsCertChain
function Get-CACertsCertUseRsa()
{
    Write-Output ((Get-Content $algorithmUsedFile -ErrorAction SilentlyContinue) -eq "rsa")
}

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

function New-CACertsVerificationCert([string]$requestedCommonName)
{
    if ([string]::IsNullOrWhiteSpace($requestedCommonName))
    {
        throw "Verification string parameter is required and cannot be null or empty"
    }
    $verificationPrefix = "verifyCert4"
    $requestedCommonName = $requestedCommonName.Trim()
    $verifCertPath = Get-CertPathForPrefix($verificationPrefix)
    $verifKeyPath = Get-KeyPathForPrefix($verificationPrefix)
    Remove-Item -Path $verifCertPath
    Remove-Item -Path $verifKeyPath
    New-ClientCertificate $verificationPrefix $rootCAPrefix $requestedCommonName
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

function New-CACertsDevice([string]$deviceName, [string]$signingCertPrefix=$rootCAPrefix, [bool]$isEdgeDevice=$false)
{
    if ([string]::IsNullOrWhiteSpace($deviceName))
    {
        throw "Device name string parameter is required and cannot be null or empty"
    }
    $deviceName = $deviceName.Trim()

    # Certificates for edge devices need to be able to sign other certs.
    if ($isEdgeDevice -eq $true)
    {
        $certFile = New-IntermediateCACertificate $deviceName $signingCertPrefix $deviceName
    }
    else
    {
        $certFile = New-ClientCertificate $deviceName $signingCertPrefix $deviceName
    }

    Write-Host ("Certificate with subject CN={0} has been output to {1}" -f $deviceName, $certFile)
    return $certFile
}

function New-CACertsEdgeDevice([string]$deviceName, [string]$signingCertPrefix=$_intermediate1Prefix)
{
    if ([string]::IsNullOrWhiteSpace($deviceName))
    {
        throw "Edge device name string parameter is required and cannot be null or empty"
    }
    $deviceName = $deviceName.Trim()
    $edgeDeviceCertificate = New-CACertsDevice $deviceName $signingCertPrefix $true
    $intermediate1CertFileName  = Get-CertPathForPrefix($_intermediate1Prefix)
    $rootCACertFileName  = Get-CertPathForPrefix($rootCAPrefix)
    $edgeDeviceFullCertChain = Get-CertPathForPrefix("$deviceName-full-chain")

    Get-Content $edgeDeviceCertificate, $intermediate1CAPemFileName, $rootCACertFileName | Set-Content $edgeDeviceFullCertChain
}

# Outputs certificates for Edge device using naming conventions from tutorials
function Write-CACertsCertificatesForEdgeDevice([string]$deviceName)
{
    Write-Warning("This command no longer needs to be run")
    Write-Host "Success"
}

Print-WarningCertsNotForProduction
