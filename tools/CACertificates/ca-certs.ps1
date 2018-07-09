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
$_rootCertSubject            = "/CN='$_rootCertCommonName'"
$_intermediateCertCommonName = "Azure IoT CA TestOnly Intermediate {0} CA"
$_intermediateCertSubject    = "CN=$_intermediateCertCommonName"
$_privateKeyPassword         = "1234"
$_keyBitsLength              = "4096"
$_days_until_expiration      = 30
$_opensslRootConfigFile      = "$_basePath/openssl_root_ca.cnf"
$_opensslIntermediateConfigFile = "$_basePath/openssl_device_intermediate_ca.cnf"

$rootCAPrefix                = "azure-iot-test-only.root.ca"
$intermediateCAPrefix        = "azure-iot-test-only.intermediate"
$keySuffix                   = "key.pem"
$certSuffix                  = "cert.pem"
$csrSuffix                   = "csr.pem"

$rootCAPemFileName          = "./RootCA.pem"
$intermediate1CAPemFileName = "./Intermediate1.pem"

# Variables containing file paths for Edge certificates
$edgePublicCertDir          = "certs"
$edgePrivateCertDir         = "private"
$edgeDeviceCertificate      = Join-Path $edgePublicCertDir "new-edge-device.cert.pem"
$edgeDevicePrivateKey       = Join-Path $edgePrivateCertDir "new-edge-device.key.pem"
$edgeDeviceFullCertChain    = Join-Path $edgePublicCertDir "new-edge-device-full-chain.cert.pem"
$edgeIotHubOwnerCA          = Join-Path $edgePublicCertDir "azure-iot-test-only.root.ca.cert.pem"

# Whether to use ECC or RSA is stored in a file.  If it doesn't exist, we default to ECC.
$algorithmUsedFile       = "./algorithmUsed.txt"

function Print-WarningCertsNotForProduction()
{
    Write-Warning "This script is provided for prototyping only."
    Write-Warning "DO NOT USE CERTIFICATES FROM THIS SCRIPT FOR PRODUCTION!"
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
    Remove-Item -Path $_basePath/csr -Recurse -ErrorAction Ignore
    Remove-Item -Path $_basePath/private -Recurse -ErrorAction Ignore
    Remove-Item -Path $_basePath/certs -Recurse -ErrorAction Ignore
    Remove-Item -Path $_basePath/intermediateCerts -Recurse -ErrorAction Ignore
    Remove-Item -Path $_basePath/newcerts -Recurse -ErrorAction Ignore

    mkdir -Path "csr"
    mkdir -Path "private"
    mkdir -Path "certs"
    mkdir -Path "intermediateCerts"
    mkdir -Path "newcerts"

    Remove-Item -Path $_basePath/index.txt -ErrorAction Ignore
    New-Item $_basePath/index.txt -ItemType file

    #Remove-Item -Path $_basePath/serial -ErrorAction Ignore
    #New-Item $_basePath/serial -ItemType file
    #"1000" | Out-File $_basePath/serial
}

function New-PrivateKey([string]$prefix, [string]$keyPass=$NULL)
{
    Write-Host ("Creating the $prefix private Key")
    $keyFile = "$_basePath/private/$prefix.$keySuffix"
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
        $passwordCreateCmd = "| openssl ec $passwordCreateCmd"
    }

    $cmd = "openssl $algorithm $passwordCreateCmd -out $keyFile"
    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd
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
    $issuerKeyFile = "$_basePath/private/$issuerPrefix.$keySuffix"
    $issuerCertFile = "$_basePath/certs/$issuerPrefix.$certSuffix"

    if (-not (Test-Path $issuerKeyFile -PathType Leaf))
    {
        Write-Host ("Private key file not found: $issuerKeyFile")
        throw ("Issuer '$issuerPrefix' private key not found")
    }
    if (-not (Test-Path $issuerCertFile -PathType Leaf))
    {
        Write-Host ("Certificate file not found: $issuerCertFile")
        throw ("Issuer '$issuerPrefix' certificate not found")
    }

    $keyFile = New-PrivateKey $prefix $keyPass
    $csrFile = "$_basePath/csr/$prefix.$csrSuffix"
    $certFile = "$_basePath/certs/$prefix.$certSuffix"

    Write-Host ("Creating the Intermediate CSR for $prefix")
    Write-Host "-----------------------------------"
    $keyPassUseCmd = ""
    if ($keyPass -ne $NULL)
    {
        $keyPassUseCmd = "-passin pass:$keyPass"
    }
    $cmd =  "openssl req -new -x509 -sha256 $keyPassUseCmd "
    $cmd += "-config $_opensslIntermediateConfigFile "
    $cmd += "-key $keyFile "
    $cmd += "-subj $subject "
    $cmd += "-out $csrFile"

    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd

    Write-Host ("Signing the certificate for $prefix with issuer certificate $issuerPrefix")
    Write-Host "-----------------------------------"
    $keyPassUseCmd = ""
    if ($issuerKeyPass -ne $NULL)
    {
        $keyPassUseCmd = "-key $issuerKeyPass"
    }
    $cmd =  "openssl ca -batch "
    $cmd += "-config $_opensslIntermediateConfigFile "
    $cmd += "-extensions $x509Ext "
    $cmd += "-days $expirationDays -notext -md sha256 "
    $cmd += "-cert $issuerCertFile "
    $cmd += "$keyPassUseCmd -keyfile $issuerKeyFile -keyform PEM "
    $cmd += "-in $csrFile -out $certFile"

    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd

    Write-Host ("Verifying the certificate for $prefix with issuer certificate $issuerPrefix")
    Write-Host "-----------------------------------"
    $cmd =  "openssl verify -CAfile $issuerCertFile $certFile"
    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd

    Write-Host ("Certificate for prefix $prefix generated at:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    Invoke-Expression -Command "openssl x509 -noout -text -in $certFile"
}

function New-ClientCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "/CN='$commonName'"
    New-IntermediateCertificate "usr_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
}

function New-ServerCertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "/CN='$commonName'"
    New-IntermediateCertificate "server_cert" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
}

function New-IntermediateCACertificate([string]$prefix, [string]$issuerPrefix, [string]$commonName)
{
    $subject = "/CN='$commonName'"
    New-IntermediateCertificate "v3_intermediate_ca" $_days_until_expiration $subject $prefix  $issuerPrefix $_privateKeyPassword $_privateKeyPassword
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
}

function New-RootCACertificate()
{
    $keyFile = New-PrivateKey $rootCAPrefix $_privateKeyPassword
    $certFile = "$_basePath/certs/$rootCAPrefix.$certSuffix"

    Write-Host ("Creating the Root CA certificate")
    $passwordUseCmd = "-passin pass:$_privateKeyPassword"
    $cmd =  "openssl req -new -x509 -config $_opensslRootConfigFile $passwordUseCmd "
    $cmd += "-key $keyFile -subj $_rootCertSubject -days $_days_until_expiration "
    $cmd += "-sha256 -extensions v3_ca -out $certFile"

    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd

    Write-Host ("CA Root Certificate Generated At:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    Invoke-Expression -Command "openssl x509 -noout -text -in $certFile"
    # Now use splatting to process this
    Write-Warning ("Generating certificate {0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $_rootCertSubject)
}

function New-CACertsSelfsignedCertificate([string]$commonName, [object]$signingCert, [bool]$isASigner=$true)
{
    # Build up argument list
    $selfSignedArgs =@{"-DnsName"=$commonName;
                       "-CertStoreLocation"="cert:LocalMachine\My";
                       "-NotAfter"=(get-date).AddDays(30);
                      }

    if ($isASigner -eq $true)
    {
        $selfSignedArgs += @{"-KeyUsage"="CertSign"; }
        $selfSignedArgs += @{"-TextExtension"= @(("2.5.29.19={text}ca=TRUE&pathlength=12"))  ; }
    }
    else
    {
        $selfSignedArgs += @{"-TextExtension"= @("2.5.29.37={text}1.3.6.1.5.5.7.3.2,1.3.6.1.5.5.7.3.1", "2.5.29.19={text}ca=FALSE&pathlength=0")  }
    }

    if ($signingCert -ne $null)
    {
        $selfSignedArgs += @{"-Signer"=$signingCert }
    }

    if ((Get-CACertsCertUseRSA) -eq $false)
    {
        $selfSignedArgs += @{"-KeyAlgorithm"="ECDSA_nistP256";
                             "-CurveExport"="CurveName" }
    }

    # Now use splatting to process this
    Write-Warning ("Generating certificate CN={0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $commonName)
    write (New-SelfSignedCertificate @selfSignedArgs)
}


function New-CACertsIntermediateCert([string]$commonName, [Microsoft.CertificateServices.Commands.Certificate]$signingCert, [string]$pemFileName)
{
    $certFileName = ($commonName + ".cer")
    $newCert = New-CACertsSelfsignedCertificate $commonName $signingCert
    Export-Certificate -Cert $newCert -FilePath $certFileName -Type CERT | Out-Null
    Import-Certificate -CertStoreLocation "cert:\LocalMachine\CA" -FilePath $certFileName | Out-Null

    # Store public PEM for later chaining
    openssl x509 -inform der -in $certFileName -out $pemFileName

    del $certFileName

    Write-Output $newCert
}

function New-RootCACertificate()
{
    Write-Host ("Creating the Root CA private key")
    $keyFile = New-PrivateKey $rootCAPrefix $_privateKeyPassword
    $certFile = "$_basePath/certs/$rootCAPrefix.$certSuffix"

    Write-Host ("Creating the Root CA certificate")
    $passwordUseCmd = "-passin pass:$_privateKeyPassword"
    $cmd =  "openssl req -new -x509 -config $_opensslRootConfigFile $passwordUseCmd "
    $cmd += "-key $keyFile -subj $_rootCertSubject -days $_days_until_expiration "
    $cmd += "-sha256 -extensions v3_ca -out $certFile"

    Write-Host ("Executing command: $cmd")
    Invoke-Expression -Command $cmd

    Write-Host ("CA Root Certificate Generated At:")
    Write-Host ("---------------------------------")
    Write-Host ("    $certFile`r`n")
    Invoke-Expression -Command "openssl x509 -noout -text -in $certFile"
    # Now use splatting to process this
    Write-Warning ("Generating certificate {0} which is for prototyping, NOT PRODUCTION.  It has a hard-coded password and will expire in 30 days." -f $_rootCertSubject)
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
    New-IntermediateCACertificate "intermediate1" $rootCAPrefix ($_intermediateCertCommonName -f "1")
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
    $sourceCertPath = "$_basePath/certs/verifyCert4.$certSuffix"
    $sourceKeyPath = "$_basePath/private/verifyCert4.$keySuffix"
    Remove-Item -Path $sourceCertPath -ErrorAction Ignore
    Remove-Item -Path $sourceKeyPath -ErrorAction Ignore
    New-ClientCertificate "verifyCert4" $rootCAPrefix $requestedCommonName
    $verifyRequestedFileName = ".\verifyCert4.cer"
    if (-not (Test-Path $sourceCertPath))
    {
        throw ("Error: CERT file {0} doesn't exist" -f $sourceCertPath)
    }
    Copy-Item -Path $sourceCertPath -Destination $verifyRequestedFileName
    if (-not (Test-Path $verifyRequestedFileName))
    {
        throw ("Error: Copying cert file file {0} to {1}" -f $sourceCertPath, $verifyRequestedFileName)
    }
    Write-Host ("Certificate with subject CN={0} has been output to {1}" -f $requestedCommonName, (Join-Path (get-location).path $verifyRequestedFileName))
}

function New-CACertsDevice([string]$deviceName, [string]$signingCertSubject=$_rootCertSubject, [bool]$isEdgeDevice=$false)
{
    $newDevicePfxFileName = ("./{0}.pfx" -f $deviceName)
    $newDevicePemAllFileName      = ("./{0}-all.pem" -f $deviceName)
    $newDevicePemPrivateFileName  = ("./{0}-private.pem" -f $deviceName)
    $newDevicePemPublicFileName   = ("./{0}-public.pem" -f $deviceName)

    $signingCert = Get-CACertsCertBySubjectName $signingCertSubject ## "CN=Azure IoT CA TestOnly Intermediate 1 CA"

    # Certificates for edge devices need to be able to sign other certs.
    if ($isEdgeDevice -eq $true)
    {
        $isASigner = $true
    }
    else
    {
        $isASigner = $false
    }

    $newDeviceCertPfx = New-CACertsSelfSignedCertificate $deviceName $signingCert $isASigner

    $certSecureStringPwd = ConvertTo-SecureString -String $_privateKeyPassword -Force -AsPlainText

    # Export the PFX of the cert we've just created.  The PFX is a format that contains both public and private keys but is NOT something
    # clients written to IOT Hub SDK's now how to process, so we'll need to do some massaging.
    Export-PFXCertificate -cert $newDeviceCertPfx -filePath $newDevicePfxFileName -password $certSecureStringPwd
    if (-not (Test-Path $newDevicePfxFileName))
    {
        throw ("Error: CERT file {0} doesn't exist" -f $newDevicePfxFileName)
    }

    # Begin the massaging.  First, turn the PFX into a PEM file which contains public key, private key, and bunches of attributes.
    # We're closer to what IOTHub SDK's can handle but not there yet.
    openssl pkcs12 -in $newDevicePfxFileName -out $newDevicePemAllFileName -nodes -password pass:$_privateKeyPassword

    # Now that we have a PEM, do some conversions on it to get formats we can process
    if ((Get-CACertsCertUseRSA) -eq $true)
    {
        openssl rsa -in $newDevicePemAllFileName -out $newDevicePemPrivateFileName
    }
    else
    {
        openssl ec -in $newDevicePemAllFileName -out $newDevicePemPrivateFileName
    }
    openssl x509 -in $newDevicePemAllFileName -out $newDevicePemPublicFileName

    Write-Host ("Certificate with subject CN={0} has been output to {1}" -f $deviceName, (Join-Path (get-location).path $newDevicePemPublicFileName))
}

function New-CACertsEdgeDevice([string]$deviceName, [string]$signingCertSubject=($_intermediateCertSubject -f "1"))
{
    New-CACertsDevice $deviceName $signingCertSubject $true
}

function Write-CACertsCertificatesToEnvironment([string]$deviceName, [string]$iothubName, [bool]$useIntermediate)
{
    $newDevicePemPrivateFileName  = ("./{0}-private.pem" -f $deviceName)
    $newDevicePemPublicFileName  = ("./{0}-public.pem" -f $deviceName)

    $rootCAPem          = Get-CACertsPemEncodingForEnvironmentVariable $rootCAPemFileName
    $devicePublicPem    = Get-CACertsPemEncodingForEnvironmentVariable $newDevicePemPublicFileName
    $devicePrivatePem   = Get-CACertsPemEncodingForEnvironmentVariable $newDevicePemPrivateFileName

    if ($useIntermediate -eq $true)
    {
        $intermediate1CAPem = Get-CACertsPemEncodingForEnvironmentVariable $intermediate1CAPemFileName
    }
    else
    {
        $intermediate1CAPem = $null
    }

    $env:IOTHUB_CA_X509_PUBLIC                 = $devicePublicPem + $intermediate1CAPem + $rootCAPem
    $env:IOTHUB_CA_X509_PRIVATE_KEY            = $devicePrivatePem
    $env:IOTHUB_CA_CONNECTION_STRING_TO_DEVICE = "HostName={0};DeviceId={1};x509=true" -f $iothubName, $deviceName
    if ((Get-CACertsCertUseRSA) -eq $true)
    {
        $env:IOTHUB_CA_USE_ECC = "0"
    }
    else
    {
        $env:IOTHUB_CA_USE_ECC = "1"
    }

    Write-Host "Success"
}

# Outputs certificates for Edge device using naming conventions from tutorials
function Write-CACertsCertificatesForEdgeDevice([string]$deviceName)
{
    $originalDevicePublicPem  = ("./{0}-public.pem" -f $deviceName)
    $originalDevicePrivatePem  = ("./{0}-private.pem" -f $deviceName)

    if (-not (Test-Path $edgePublicCertDir))
    {
        mkdir $edgePublicCertDir | Out-Null
    }

    if (-not (Test-Path $edgePrivateCertDir))
    {
        mkdir $edgePrivateCertDir | Out-Null
    }

    Copy-Item $originalDevicePublicPem $edgeDeviceCertificate
    Copy-Item $originalDevicePrivatePem $edgeDevicePrivateKey
    Get-Content $originalDevicePublicPem, $intermediate1CAPemFileName, $rootCAPemFileName | Set-Content $edgeDeviceFullCertChain
    Copy-Item $rootCAPemFileName $edgeIotHubOwnerCA
    Write-Host "Success"
}

# This will read in a given .PEM file and output it in a format that we can
# immediately set ENV variable in it with \r\n done right.
function Get-CACertsPemEncodingForEnvironmentVariable([string]$fileName)
{
    $outputString = $null
    $data = Get-Content $fileName
    foreach ($line in $data)
    {
        $outputString += ($line + "`r`n")
    }

    Write-Output $outputString
}

Print-WarningCertsNotForProduction
