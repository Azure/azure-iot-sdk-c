function ConvertTo-Base64 {
    param($Content)
    $ContentBytes  = [System.Text.Encoding]::UTF8.GetBytes($Content)
    $Base64Content = [System.Convert]::ToBase64String($ContentBytes)
    return $Base64Content
}


function Set-FileContent {
    param(
        $Path = $null,
        $Content = $null
    )

    $OutFileDir = Split-Path -Path $Path -Parent
    if ($OutFileDir -ne "" -and $(Test-Path $OutFileDir) -eq $false) {
        New-Item -ItemType Directory -Force -Path $OutFileDir | Out-Null        
    }

    if ($PSVersionTable.PSVersion.Major -lt 7) {
        $Utf8NoBom = New-Object System.Text.UTF8Encoding($false)  # $false = no BOM
        [System.IO.File]::WriteAllText("$Path", "$Content", $Utf8NoBom)
    } else {
        Set-Content -Path "$Path" -Value $Content -Encoding utf8 -NoNewline
    }
}

function Stop-OnError {
    param([string]$Step = 'Command', [int]$ExpectedReturn = 0, [switch]$Throw)
    if ($LASTEXITCODE -ne $ExpectedReturn) {
        $ErrorMessage = "ERROR: `"$Step`" failed (exit code $LASTEXITCODE)"
        if ($Throw) {
            throw $ErrorMessage
        } else {
            Write-Host $ErrorMessage 
            exit 1
        }
    }
}

function New-DpsDerivedSymmetricKey {
    param(
        $SymmetricKey = $null,
        $DeviceId = $null
    )

    $hmacsha256 = New-Object System.Security.Cryptography.HMACSHA256
    try {
        $hmacsha256.key = [Convert]::FromBase64String($SymmetricKey)
        $sig = $hmacsha256.ComputeHash([Text.Encoding]::ASCII.GetBytes($DeviceId))
        return [Convert]::ToBase64String($sig)
    } finally {
        $hmacsha256.Dispose()
    }
}

function Export-Pkcs8PrivateKeyPem {
    param($Key)

    if ($PSVersionTable.PSVersion.Major -lt 7) {
        $KeyPemHex = [Convert]::ToBase64String($Key.Key.Export([System.Security.Cryptography.CngKeyBlobFormat]::Pkcs8PrivateBlob), 'InsertLineBreaks')
        return "-----BEGIN PRIVATE KEY-----`n$KeyPemHex`n-----END PRIVATE KEY-----"
    } else {
        return $Key.ExportPkcs8PrivateKeyPem()
    }
}

function New-PrivateKey {
    param([string]$Path = $null)

    $rsa = [System.Security.Cryptography.RSA]::Create(4096)

    if ($Path -ne $null -and $Path -ne "") {
        $pem = Export-Pkcs8PrivateKeyPem -Key $rsa
        Write-Host "Saving private key to $Path"
        Set-FileContent -Path $Path -Content $pem
    }

    return $rsa
}

function New-RandomNumber {
    param($Length = 16)

    if ($PSVersionTable.PSVersion.Major -lt 7) {
        $RandomNumber = New-Object byte[] $Length
        [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($RandomNumber)
        return $RandomNumber
    } else {
        return [System.Security.Cryptography.RandomNumberGenerator]::GetBytes($Length)
    }
}

function New-Certificate {
    param(
        [string]$Subject,
        [System.Security.Cryptography.RSA]$Key,
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$IssuerCert = $null,
        [System.Security.Cryptography.RSA]$IssuerKey = $null,
        [bool]$IsCA = $false,
        [int]$Days = 30
    )
    Write-Host "Running New-Certificate($Subject)"

    $req = [System.Security.Cryptography.X509Certificates.CertificateRequest]::new($Subject, [System.Security.Cryptography.RSA]$Key, [System.Security.Cryptography.HashAlgorithmName]::SHA256, [System.Security.Cryptography.RSASignaturePadding]::Pkcs1)

    $SubjectKeyIdentifierExtension = [System.Security.Cryptography.X509Certificates.X509SubjectKeyIdentifierExtension]::new($req.PublicKey, $false)
    $req.CertificateExtensions.Add($SubjectKeyIdentifierExtension)

    if ($IsCA) {
        $BasicConstraintExtension = New-Object System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension -ArgumentList $true, $false, 0, $true
        $req.CertificateExtensions.Add($BasicConstraintExtension)
    } else {
        $BasicConstraintExtension = New-Object System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension -ArgumentList $false, $false, 0, $false
        $req.CertificateExtensions.Add($BasicConstraintExtension)
    }

    if ($IsCA) {
        $KeyUsageExtension = [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new( `
            [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature `
            -bor [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::CrlSign `
            -bor [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::KeyCertSign,
            $true)
    } else {
        $KeyUsageExtension = [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new( `
            [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature `
            -bor [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::KeyEncipherment,
            $true)
    }

    $req.CertificateExtensions.Add($KeyUsageExtension)

    if ($IsCA) {

    } else {
        $EkuOids = [System.Security.Cryptography.OidCollection]::new()
        [void]$EkuOids.Add(
            [System.Security.Cryptography.Oid]::new("1.3.6.1.5.5.7.3.2") # clientAuth = 1.3.6.1.5.5.7.3.2
        ) 
        $EnhancedKeyUsageExtension = [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($EkuOids, $false)
        $req.CertificateExtensions.Add($EnhancedKeyUsageExtension)
    }

    $NotBefore = [datetime]::UtcNow
    $NotAfter = [datetime]::UtcNow.AddDays($Days)

    if ($IssuerCert -eq $null) {
        # Self-signed (Root CA)
        return $req.CreateSelfSigned($NotBefore, $NotAfter)
    } else {
        # Signed by issuer (Intermediate or Device)

        # Adjust NotBefore and NotAfter to match issuer certificate.
        if ($IssuerCert.NotBefore -ne $null -and $NotBefore -lt $IssuerCert.NotBefore) {
            $NotBefore = $IssuerCert.NotBefore
        }

        if ($IssuerCert.NotAfter -ne $null -and $NotAfter -gt $IssuerCert.NotAfter) {
            $NotAfter = $IssuerCert.NotAfter
        }

        # Serial number must be random bytes (8–20 bytes is typical)
        $SerialNumber = New-RandomNumber -Length 16

        # Use the issuer *private key* for signing
        $SignatureGenerator = [System.Security.Cryptography.X509Certificates.X509SignatureGenerator]::CreateForRSA(
            $IssuerKey,
            [System.Security.Cryptography.RSASignaturePadding]::Pkcs1
        )

        # Use issuer subject name from the issuer cert
        return $req.Create(
            $IssuerCert.SubjectName,
            $SignatureGenerator,
            $NotBefore,
            $NotAfter,
            $SerialNumber
        )
    }
}

function Export-X509CertificateToPem {
    param([System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate)

    if ($PSVersionTable.PSVersion.Major -lt 7) {
        $CertificatePemHex = [Convert]::ToBase64String($Certificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert), 'InsertLineBreaks')
        return "-----BEGIN CERTIFICATE-----`n$CertificatePemHex`n-----END CERTIFICATE-----"
    } else {
        return $Certificate.ExportCertificatePem()
    }
}

function Export-X509CertificateToPemFile {
    param([System.Security.Cryptography.X509Certificates.X509Certificate2]$Cert, [string]$Path)
    $pem = Export-X509CertificateToPem -Certificate $Cert
    Write-Host "Exporting certificate to $Path"
    Set-FileContent -Path $Path -Content $pem
}

function New-DpsCACertificateChain {
    param(
        $ResourceGroup,
        $DpsName,
        [TimeSpan]$CertificateExpiration = [TimeSpan]::FromDays(30),
        $CertDir = "$(pwd)/certs",
        $PrivateDir = "$(pwd)/private",
        $CsrDir = "$(pwd)/csr"
    )

    New-Item -ItemType Directory -Force -Path $CertDir, $CsrDir, $PrivateDir | Out-Null

    $Id = Get-Random -Minimum 10000 -Maximum 99999
    $CertPrefix = "azure-iot-test-$Id"

    $CertInfo = [pscustomobject]@{
        RootCASubjectName             = "CN=Azure IoT Hub CA Cert Test Only $Id"
        IntermediateCASubjectName     = "CN=Azure IoT Hub Intermediate Cert Test Only $Id"
        RootCACertificatePath         = "$CertDir/$CertPrefix-root.cert.pem"
        RootCAPrivateKeyPath          = "$PrivateDir/$CertPrefix-root.key.pem"
        IntermediateCACertificatePath = "$CertDir/$CertPrefix-intermediate.cert.pem"
        IntermediateCAPrivateKeyPath  = "$PrivateDir/$CertPrefix-intermediate.key.pem"
        IntermediateCACertificate     = $null
        IntermediateCAPrivateKey      = $null
        DpsRootCAName                 = "azure-iot-test-only-root-$Id"
    }

    # Root CA
    $rootKey = New-PrivateKey -Path $CertInfo.RootCAPrivateKeyPath
    $rootCert = New-Certificate -Subject $CertInfo.RootCASubjectName -Key $rootKey -IssuerCert $null -IssuerKey $null -IsCA $true -Days $CertificateExpiration.Days
    Export-X509CertificateToPemFile -Cert $rootCert -Path $CertInfo.RootCACertificatePath

    # Intermediate CA
    $CertInfo.IntermediateCAPrivateKey = New-PrivateKey -Path $CertInfo.IntermediateCAPrivateKeyPath
    $CertInfo.IntermediateCACertificate = New-Certificate -Subject $CertInfo.IntermediateCASubjectName -Key $CertInfo.IntermediateCAPrivateKey -IssuerCert $rootCert -IssuerKey $rootKey -IsCA $true -Days $CertificateExpiration.Days
    Export-X509CertificateToPemFile -Cert $CertInfo.IntermediateCACertificate -Path $CertInfo.IntermediateCACertificatePath

    az iot dps certificate create --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --path $CertInfo.RootCACertificatePath | Out-Null
    Stop-OnError -Step "az iot dps certificate create"

    $etag = az iot dps certificate show --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --query etag -o tsv
    Stop-OnError -Step "az iot dps certificate show"

    $DpsVerificationCodeInfo = az iot dps certificate generate-verification-code --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --etag $etag | ConvertFrom-Json
    Stop-OnError -Step "az iot dps certificate generate-verification-code"

    # Create verification cert
    $DpsVerificationKeyPath = "$PrivateDir/dps-root-ca-verification.key.pem"
    $DpsVerificationCertificatePath = "$CertDir/dps-root-ca-verification.cert.pem"
    $DpsVerificationKey = New-PrivateKey -Path $DpsVerificationKeyPath
    $DpsVerificationCertificate = New-Certificate -Subject "CN=$($DpsVerificationCodeInfo.properties.verificationCode)" -Key $DpsVerificationKey -IssuerCert $rootCert -IssuerKey $rootKey -IsCA $false -Days 30
    Export-X509CertificateToPemFile -Cert $DpsVerificationCertificate -Path $DpsVerificationCertificatePath

    # Verify with DPS
    $etag = az iot dps certificate show --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --query etag -o tsv

    az iot dps certificate verify --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --path $DpsVerificationCertificatePath --etag $etag | Out-Null
    Stop-OnError -Step "az iot dps certificate verify"

    Remove-Item $DpsVerificationKeyPath
    Remove-Item $DpsVerificationCertificatePath

    return $CertInfo 
}

function New-DpsSymmetricKeyIndividualEnrollment {
    param(
        $ResourceGroup = $null,
        $DpsName = $null,
        $EnrollmentId = $null,
        $AdrPolicyName = $null
    )

    Write-Host "Creating Azure DPS symmetric-key individual enrollment ($EnrollmentId; $AdrPolicyName)."

    if ($AdrPolicyName -eq $null) {
        $EnrollmentInfo = az iot dps enrollment create --dps-name $DpsName --resource-group $ResourceGroup --at symmetricKey --enrollment-id $EnrollmentId  | ConvertFrom-Json
    } else {
        $EnrollmentInfo = az iot dps enrollment create --dps-name $DpsName --resource-group $ResourceGroup --at symmetricKey --enrollment-id $EnrollmentId --credential-policy $AdrPolicyName | ConvertFrom-Json
    }

    Stop-OnError -Step "Create an Azure DPS symmetric-key individual enrollment ($EnrollmentId; $AdrPolicyName)"

    return $EnrollmentInfo
}

function New-DpsX509IndividualEnrollment {
    param(
        $ResourceGroup = $null,
        $DpsName = $null,
        $EnrollmentId = $null,
        $AdrPolicyName = $null,
        $CertificatesDir = "$(pwd)/certs",
        $PrivateKeysDir = "$(pwd)/private"
    )

    Write-Host "Creating Azure DPS x509 individual enrollment ($EnrollmentId; $AdrPolicyName; $CertificatesDir; $PrivateKeysDir)."

    $DpsDevicePrivateKeyPath = Join-Path $PrivateKeysDir ("{0}.key.pem"  -f $EnrollmentId)
    $DpsDeviceCertificatePath   = Join-Path $CertificatesDir ("{0}.cert.pem" -f $EnrollmentId)
    $DpsDevicePrivateKey = New-PrivateKey -Path $DpsDevicePrivateKeyPath
    $DpsDeviceCertificate = New-Certificate -Subject "CN=$EnrollmentId" -Key $DpsDevicePrivateKey -IssuerCert $null -IssuerKey $null -IsCA $false -Days 30
    Export-X509CertificateToPemFile -Cert $DpsDeviceCertificate -Path $DpsDeviceCertificatePath

    if ($AdrPolicyName -eq $null) {
        $EnrollmentInfo = az iot dps enrollment create --dps-name $DpsName --resource-group $ResourceGroup --at x509 --enrollment-id $EnrollmentId --cp $DpsDeviceCertificatePath | ConvertFrom-Json
    } else {
        $EnrollmentInfo = az iot dps enrollment create --dps-name $DpsName --resource-group $ResourceGroup --at x509 --enrollment-id $EnrollmentId --cp $DpsDeviceCertificatePath --credential-policy $AdrPolicyName | ConvertFrom-Json
    }

    Stop-OnError -Step "Create an Azure DPS x509 individual enrollment ($EnrollmentId; $AdrPolicyName)"

    return [pscustomobject]@{
        Enrollment = $EnrollmentInfo
        DpsDevicePrivateKeyPath = $DpsDevicePrivateKeyPath
        DpsDeviceCertificatePath = $DpsDeviceCertificatePath
    }
}

function New-DpsSymmetricKeyEnrollmentGroup {
    param(
        $ResourceGroup = $null,
        $DpsName = $null,
        $EnrollmentId = $null,
        $AdrPolicyName = $null
    )

    Write-Host "Creating Azure DPS symmetric-key enrollment group ($EnrollmentId; $AdrPolicyName)."

    if ($AdrPolicyName -eq $null) {
        $EnrollmentInfo = az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $EnrollmentId | ConvertFrom-Json
    } else {
        $EnrollmentInfo = az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $EnrollmentId --credential-policy $AdrPolicyName | ConvertFrom-Json
    }

    Stop-OnError -Step "Create an Azure Device Provisioning symmetric-key enrollment group ($EnrollmentId; $AdrPolicyName)"

    return $EnrollmentInfo
}

function New-DpsX509EnrollmentGroup {
    param(
        $ResourceGroup = $null,
        $DpsName = $null,
        $EnrollmentId = $null,
        $AdrPolicyName = $null, 
        $IntermediateCACertificatePath = $null,
        $IotHubFqdn
    )

    Write-Host "Creating Azure DPS x509 enrollment group ($EnrollmentId; $AdrPolicyName)."

    if ($AdrPolicyName -eq $null) {
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $EnrollmentId --ap static --cp $IntermediateCACertificatePath --provisioning-status enabled --iot-hubs $IotHubFqdn  | Out-Null
    } else {
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $EnrollmentId --ap static --cp $IntermediateCACertificatePath --provisioning-status enabled --iot-hubs $IotHubFqdn --credential-policy $AdrPolicyName | Out-Null
    }

    Stop-OnError -Step "Create an Azure Device Provisioning x509 enrollment group ($EnrollmentId; $AdrPolicyName)"
}

function New-AzureResourceGroupName {
    param($Prefix = "rg-", $OutFile = $null)
    $ResourceGroupName = $Prefix + $([guid]::NewGuid().Guid.Replace('-', ''))

    if ($OutFile -ne $null) {
        $OutFileDir = Split-Path -Path $OutFile -Parent
        if ($OutFileDir -ne "" -and $(Test-Path $OutFileDir) -eq $false) {
            New-Item -ItemType Directory -Force -Path $OutFileDir | Out-Null
        }

        Set-FileContent -Path $OutFile -Content $ResourceGroupName

    }

    return $ResourceGroupName
}

function New-AzIotTestEnvironment {
    <#
    .SYNOPSIS
    Creates a new set of Azure Resources for testing Azure IoT scenarios, including an IoT Hub and optionally a Device Provisioning Service, with different types of enrollments and devices.

    .DESCRIPTION
    Creates a new set of Azure Resources for testing Azure IoT scenarios, including an IoT Hub and optionally a Device Provisioning Service, with different types of enrollments and devices.

    .PARAMETER AzureLocation
    Specifies the Azure location for the resources. Default is "centraluseuap".
    .PARAMETER AzureSubscriptionId
    Specifies the Azure subscription ID. If not provided, the current Azure CLI session default subscription will be used.
    .PARAMETER ResourceGroup
    Specifies the name of the resource group. If not provided, a new resource group will be created.
    .PARAMETER DpsName
    Specifies the name of the Device Provisioning Service. If not provided, a new DPS will be created.
    .PARAMETER IotHubName
    Specifies the name of the IoT Hub. If not provided, a new IoT Hub will be created.
    .PARAMETER IotHubDomainName
    Specifies the domain name of the IoT Hub. Default is "azure-devices.net".
    .PARAMETER StorageAccountName
    Specifies the name of the Storage Account. If not provided, a new Storage Account will be created.
    .PARAMETER DpsSymmKeyIndividualEnrollments
    Specifies the number of symmetric key individual enrollments to create in DPS. Default is 0.
    .PARAMETER DpsX509IndividualEnrollments
    Specifies the number of x509 individual enrollments to create in DPS. Default is 0.
    .PARAMETER DpsSymmKeyGroupEnrollmentDevices
    Specifies the number of devices to create under a symmetric key enrollment group in DPS. Default is 0.
    .PARAMETER DpsX509GroupEnrollmentDevices
    Specifies the number of devices to create under an x509 enrollment group in DPS. Default is 1.
    .PARAMETER IotHubSymmKeyDevices
    Specifies the number of symmetric key devices to create in IoT Hub. Default is 1.
    .PARAMETER IotHubX509ThumbprintDevices
    Specifies the number of x509 thumbprint devices to create in IoT Hub. Default is 1.
    .PARAMETER IotHubX509CADevices
    Specifies the number of x509 CA devices to create in IoT Hub. Default is 0.
    .PARAMETER EnableFileUpload
    Specifies whether to enable file upload in IoT Hub. Default is false.
    .PARAMETER NoDps
    Specifies whether to skip creating a Device Provisioning Service. Default is false.
    .PARAMETER EnableCertificateManagement
    Specifies whether to enable certificate management in IoT Hub and DPS using Azure Device Registration (ADR). Default is false.
    .PARAMETER CertDir
    Specifies the directory to store generated certificates. Default is "$(pwd)/certs". The path is created if it does not exist.
    .PARAMETER PrivateDir
    Specifies the directory to store generated private keys. Default is "$(pwd)/private". The path is created if it does not exist.
    .PARAMETER CsrDir
    Specifies the directory to store generated certificate signing requests (CSRs). Default is "$(pwd)/csr". The path is created if it does not exist.

    .OUTPUTS
    A custom object containing information about the created Azure resources and devices, including connection strings, certificate paths, and enrollment details.

    .EXAMPLE
    PS> $TestEnvInfo = New-AzIotTestEnvironment

    This command creates a new Azure IoT test environment in the default location with 1 x509 group enrollment device in DPS and 1 symmetric key device and 1 x509 thumbprint device in IoT Hub, without enabling certificate management nor file upload on Azure IoT Hub.

    .EXAMPLE
    PS> $TestEnvInfo = New-AzIotTestEnvironment -AzureLocation "eastus2" -DpsSymmKeyIndividualEnrollments 2 -DpsX509IndividualEnrollments 1 -DpsSymmKeyGroupEnrollmentDevices 2 -DpsX509GroupEnrollmentDevices 2 -IotHubSymmKeyDevices 2 -IotHubX509ThumbprintDevices 1 -IotHubX509CADevices 1
    
    This command creates a new Azure IoT test environment in the "eastus2" location with 2 symmetric key individual enrollments, 1 x509 individual enrollment, 2 devices under a symmetric key enrollment group, 2 devices under an x509 enrollment group in DPS, and 2 symmetric key devices, 1 x509 thumbprint device, and 1 x509 CA device in IoT Hub.

    .EXAMPLE
    PS> $TestEnvInfo = New-AzIotTestEnvironment -EnableCertificateManagement
    
    This command creates a new Azure IoT test environment with an IoT Hub and Device Provisioning Service that has certificate management enabled using Azure Device Registration (ADR).
    #>
    param(
        [string]$AzureLocation = "centraluseuap", # Other locations (e.g.): eastus2euap, westus2, ...
        [string]$AzureSubscriptionId = $null,
        [string]$ResourceGroup = $(New-AzureResourceGroupName),
        [string]$DpsName       = "dps-$([guid]::NewGuid().Guid.Replace('-', ''))",
        [string]$IotHubName    = "iothub-$([guid]::NewGuid().Guid.Replace('-', ''))",
        [string]$IotHubDomainName = "azure-devices.net",
        [string]$StorageAccountName = "teststoacc$([guid]::NewGuid().Guid.Replace('-', ''))".Substring(0, 24), # Max size of Storage Account name
        [int]$DpsSymmKeyIndividualEnrollments = 0,
        [int]$DpsX509IndividualEnrollments = 0,
        [int]$DpsSymmKeyGroupEnrollmentDevices = 0,
        [int]$DpsX509GroupEnrollmentDevices = 1,
        [int]$IotHubSymmKeyDevices = 1,
        [int]$IotHubX509ThumbprintDevices = 1,
        [int]$IotHubX509CADevices = 0,
        [switch]$EnableFileUpload,
        [switch]$NoDps,
        [switch]$EnableCertificateManagement,
        [string]$CertDir = "$(pwd)/certs",
        [string]$PrivateDir = "$(pwd)/private",
        [string]$CsrDir = "$(pwd)/csr"
    )

    $TestEnvInfo = [pscustomobject]@{
        AzureResourceGroup = $null
        IotHubConnectionString = $null
        IotHubEventHubConnectionString = $null
        IotHubEventHubCompatibleName = $null
        IotHubEventHubPartitionCount = 0
        IotHubEventHubConsumerGroups = @()
        DpsDeviceFqdn = $null
        DpsServiceFqdn = $null
        DpsConnectionString = $null
        DpsIdScope = $null
        DpsRootCACertificatePath = $null
        DpsRootCAPrivateKeyPath = $null
        DpsIntermediateCACertificatePath = $null
        DpsIntermediateCAPrivateKeyPath = $null
        IotHubSymmKeyDevices = @()
        IotHubX509ThumbprintDevices = @()
        IotHubX509CADevices = @()
        DpsIndividualSymKeyEnrollments = @()
        DpsIndividualX509Enrollments = @()
        DpsGroupSymKeyEnrollments = @()
        DpsGroupX509Enrollments = @()
        AzureAdrPolicyName = $null
    }

    $IotHubFqdn = "$($IotHubName).$($IotHubDomainName)"

    # Login to Azure if not already
    $null = & az account show 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Not logged in to Azure. Running 'az login'..."
        & az login --use-device-code
        if ($LASTEXITCODE -ne 0) {
            throw "Azure login failed."
        }
    }
    else {
        Write-Host "Already logged in to Azure."
    }

    $AzureAccount = az account show | ConvertFrom-Json
    $IsAzureAccountServicePrincipal = $AzureAccount.user.type -eq "servicePrincipal"

    # Subscription id...
    if ($AzureSubscriptionId -eq $null -or $AzureSubscriptionId -imatch "[ `t]*") {
        Stop-OnError -Step "Get Azure account information"
        $AzureSubscriptionId = $AzureAccount.id
    } else {
        $discard = az account set --subscription "$AzureSubscriptionId" --only-show-errors 2>$null
        Stop-OnError -Step "Set Azure subscription"
    }

    # Add Azure IoT extension if not already added (required for some az iot commands, e.g. az iot adr ns create)
    # TODO: install non-preview version after GA.
    $AzCliAzureIotExtension = $(az extension list | Convertfrom-json | ?{$_.name -eq "azure-iot"})

    if ($AzCliAzureIotExtension -ne $null -and $AzCliAzureIotExtension.preview -eq $false) {
        Write-Host "Non-preview Azure IoT extension found (version $($AzCliAzureIotExtension.version)). Removing..."
        az extension remove --name azure-iot --only-show-errors | Out-Null
        Stop-OnError -Step "Remove non-preview Azure IoT extension"
        $AzCliAzureIotExtension = $null
    }

    if ($AzCliAzureIotExtension -eq $null) {
        Write-Host "Installing Azure IoT extension."
        az extension add --name azure-iot --allow-preview --only-show-errors | Out-Null
        Stop-OnError -Step "Install Azure IoT extension"
    }

    # Create resource group (if does not exist).
    $ResouceGroupExists = az group exists --name $ResourceGroup -o tsv
    if ($ResouceGroupExists -eq 'true') {
       $AzureResourceGroup = az group show --name "$ResourceGroup" | ConvertFrom-Json
    } else {
        Write-Host "Creating Azure resource group ($ResourceGroup)"
        $AzureResourceGroup = az group create --name "$ResourceGroup" --location $AzureLocation --only-show-errors 2>$null | ConvertFrom-json 
        Stop-OnError -Step "Create Azure resource group"
    }

    if ($EnableCertificateManagement -eq $true) {
        $AzureIotHubAppId = "89d10474-74af-4874-99a7-c23c2f643083" # Azure IoT Hub application ID (same for all tenants)
        $AzureIotHubObjectId = "0aab4033-4ad9-4b0b-9934-542334eceffb" # Manually obtained...

        $ResouceGroupScope = "/subscriptions/$AzureSubscriptionId/resourceGroups/$ResourceGroup"
        Write-Host "Creating Azure role assignment for certificate management (scope=$ResouceGroupScope)"
        if ($IsAzureAccountServicePrincipal) {
            az role assignment create --assignee-object-id $AzureIotHubObjectId --assignee-principal-type ServicePrincipal --role Contributor --scope "$ResouceGroupScope" --only-show-errors | Out-Null
        } else {
            az role assignment create --assignee $AzureIotHubAppId --role Contributor --scope "$ResouceGroupScope" --only-show-errors | Out-Null
        }
        Stop-OnError -Step "Create Azure role assignment for certificate management"

        $CertMgmtUserIdentity = "$($ResourceGroup)cmuid"
        Write-Host "Creating User-Assigned Managed Identity (UAMI) ($CertMgmtUserIdentity)"
        $AzureCertMgmtIdentity = az identity create --name "$CertMgmtUserIdentity" --resource-group "$ResourceGroup" --location "$AzureLocation" 2>$null | ConvertFrom-Json
        Stop-OnError -Step "Create User-Assigned Managed Identity (UAMI)"

        $AzureAdrNamespaceName = "azure-adr-ns"
        $AzureAdrPolicyName = "azure-adr-policy"
        Write-Host "Creating ADR Namespace (ns=$AzureAdrNamespaceName; policy=$AzureAdrPolicyName)"
        $AzureAdrNamespace = az iot adr ns create --name "$AzureAdrNamespaceName" --enable-credential-policy --resource-group "$ResourceGroup" --location "$AzureLocation" --policy-name "$AzureAdrPolicyName" 2>$null | ConvertFrom-Json
        Stop-OnError -Step "Create ADR Namespace"    

        Write-Host "Assigning ADR custom role to UAMI (a5c3590a-3a1a-4cd4-9648-ea0a32b15137)"
        az role assignment create --assignee "$($AzureCertMgmtIdentity.principalId)" --role "a5c3590a-3a1a-4cd4-9648-ea0a32b15137" --scope "$($AzureAdrNamespace.id)" --only-show-errors | Out-Null
        Stop-OnError -Step "Assign ADR custom role 1 to UAMI"

        Write-Host "Assigning ADR custom role to UAMI (547f7f0a-69c0-4807-bd9e-0321dfb66a84)"
        az role assignment create --assignee "$($AzureCertMgmtIdentity.principalId)" --role "547f7f0a-69c0-4807-bd9e-0321dfb66a84" --scope "$($AzureAdrNamespace.id)" --only-show-errors | Out-Null
        Stop-OnError -Step "Assign ADR custom role 2 to UAMI"

        Write-Host "Creating Azure IoT Hub ($IotHubName, with certificate management support)"
        $AzureIoTHub = az iot hub create --name "$IotHubName" --resource-group "$ResourceGroup" --location "$AzureLocation" --sku GEN2 `
            --mi-user-assigned "$($AzureCertMgmtIdentity.id)" --ns-resource-id "$($AzureAdrNamespace.id)" --ns-identity-id "$($AzureCertMgmtIdentity.id)" | ConvertFrom-Json
        Stop-OnError -Step "Create Azure IoT Hub (with certificate management support)"

        Write-Host "Assigning Contributor role on Azure IoT for ADR principal"
        az role assignment create --assignee "$($AzureAdrNamespace.identity.principalId)" --role "Contributor" --scope "$($AzureIoTHub.id)" --only-show-errors | Out-Null
        Stop-OnError -Step "Assign Contributor role on Azure IoT for ADR principal"

        Write-Host "Assigning IoT Hub Registry Contributor role on Azure IoT for ADR principal"
        az role assignment create --assignee "$($AzureAdrNamespace.identity.principalId)" --role "IoT Hub Registry Contributor" --scope "$($AzureIoTHub.id)" --only-show-errors | Out-Null
        Stop-OnError -Step "Assign IoT Hub Registry Contributor role on Azure IoT for ADR principal"
        
        if ($NoDps -eq $false) {
            Write-Host "Creating Device Provisioning Service with ADR integration ($DpsName)"
            $AzureDps = az iot dps create --name "$DpsName" --resource-group "$ResourceGroup" --location "$AzureLocation" `
                --mi-user-assigned "$($AzureCertMgmtIdentity.id)" --ns-resource-id "$($AzureAdrNamespace.id)" --ns-identity-id "$($AzureCertMgmtIdentity.id)" --only-show-errors | ConvertFrom-Json
            Stop-OnError -Step "Create Device Provisioning Service with ADR integration"
        }
    } else {
        Write-Host "Creating Azure IoT Hub ($IotHubName)."
        $AzureIoTHub = az iot hub create --name "$IotHubName" --resource-group "$ResourceGroup" --location "$AzureLocation" --mintls "1.2" | ConvertFrom-Json
        Stop-OnError -Step "Create Azure IoT Hub"

        if ($NoDps -eq $false) {
            Write-Host "Creating Azure Device Provisioning Service ($DpsName)."
            $AzureDps = az iot dps create --name "$DpsName" --resource-group "$ResourceGroup" --location "$AzureLocation" | ConvertFrom-Json
            Stop-OnError -Step "Create Device Provisioning Service"
        }
    }

    if ($NoDps -eq $false) {    
        Write-Host "Linking Azure IoT Hub ($IotHubName) to Azure Device Provisioning service ($DpsName)"
        az iot dps linked-hub create --dps-name "$DpsName" --resource-group "$ResourceGroup" --hub-name "$IotHubName" | Out-Null
        Stop-OnError -Step "Link Azure IoT Hub to Azure Device Provisioning service"

        # Step was put here to optimize if blocks, since it's common down.
        Write-Host "Creating DPS certificates"
        $DpsCertificates = New-DpsCACertificateChain -ResourceGroup $ResourceGroup -DpsName $DpsName
    }

    if ($EnableCertificateManagement -eq $true) {
        Write-Host "Syncing ADR credentials ($AzureAdrNamespaceName)."
        az iot adr ns credential sync --ns "$AzureAdrNamespaceName" --resource-group "$ResourceGroup" --only-show-errors | Out-Null
        Stop-OnError -Step "Sync ADR credentials"
    } else {
        $AzureAdrPolicyName = $null # Used below on enrollment creation.
    }

    # Create IoT Hub Devices
    for ($i = 0; $i -lt $IotHubSymmKeyDevices; $i++) {
        $IotHubDeviceId = "sk-$([guid]::NewGuid().Guid.Replace('-', ''))"

        Write-Host "Creating Azure IoT Hub symmetric-key device ($IotHubDeviceId)"
        $IotHubDeviceInfo = az iot hub device-identity create --resource-group $ResourceGroup --hub-name $IotHubName --device-id $IotHubDeviceId | ConvertFrom-Json
        Stop-OnError -Step "Create Azure IoT Hub symmetric-key device ($IotHubDeviceId)"
        $PrimaryConnectionString = az iot hub device-identity connection-string show --resource-group $ResourceGroup --hub-name $IotHubName -d $IotHubDeviceId --kt primary | ConvertFrom-Json
        Stop-OnError -Step "Get Azure IoT Hub device primary connection-string ($IotHubDeviceId)"
        $SecondaryConnectionString = az iot hub device-identity connection-string show --resource-group $ResourceGroup --hub-name $IotHubName -d $IotHubDeviceId --kt secondary | ConvertFrom-Json
        Stop-OnError -Step "Get Azure IoT Hub device secondary connection-string ($IotHubDeviceId)"

        $DeviceMinimalInfo = [pscustomobject]@{
            Id = $IotHubDeviceId
            PrimaryKey = $IotHubDeviceInfo.authentication.symmetricKey.primaryKey
            SecondaryKey = $IotHubDeviceInfo.authentication.symmetricKey.secondaryKey
            PrimaryConnectionString = $PrimaryConnectionString.connectionString
            SecondaryConnectionString = $SecondaryConnectionString.connectionString
        }

        $TestEnvInfo.IotHubSymmKeyDevices += $DeviceMinimalInfo
    }

    for ($i = 0; $i -lt $IotHubX509ThumbprintDevices; $i++) {
        $IotHubDeviceId = "x509tp-$([guid]::NewGuid().Guid.Replace('-', ''))"
        $CertificateSubjectName = "C=US, ST=Washington, L=Redmond, O=Company, OU=Org, CN=www.company.com"

        $IotHubDevicePrivateKey = New-PrivateKey
        $IotHubDevicePrimaryCertificate = New-Certificate -Subject $CertificateSubjectName -Key $IotHubDevicePrivateKey
        $IotHubDeviceSecondaryCertificate = New-Certificate -Subject $CertificateSubjectName -Key $IotHubDevicePrivateKey

        Write-Host "Creating Azure IoT Hub x509 thumbprint device ($IotHubDeviceId)"
        $IotHubDeviceInfo = az iot hub device-identity create --resource-group $ResourceGroup --hub-name $IotHubName --device-id $IotHubDeviceId `
            --am x509_thumbprint --ptp $IotHubDevicePrimaryCertificate.Thumbprint --stp $IotHubDeviceSecondaryCertificate.Thumbprint | ConvertFrom-Json
        Stop-OnError -Step "Create Azure IoT Hub x509 thumbprint device ($IotHubDeviceId)"

        $ConnectionString = az iot hub device-identity connection-string show --resource-group $ResourceGroup --hub-name $IotHubName -d $IotHubDeviceId | ConvertFrom-Json
        Stop-OnError -Step "Get Azure IoT Hub device connection-string ($IotHubDeviceId)"

        $DeviceMinimalInfo = [pscustomobject]@{
            Id = $IotHubDeviceId
            PrimaryCertificateBase64 = $(ConvertTo-Base64 -Content $(Export-X509CertificateToPem -Certificate $IotHubDevicePrimaryCertificate))
            PrimaryCertificateThumbprint = $IotHubDevicePrimaryCertificate.Thumbprint
            PrimaryPrivateKeyBase64 = $(ConvertTo-Base64 -Content $(Export-Pkcs8PrivateKeyPem -Key $IotHubDevicePrivateKey))
            SecondaryCertificateBase64 = $(ConvertTo-Base64 -Content $(Export-X509CertificateToPem -Certificate $IotHubDeviceSecondaryCertificate))
            SecondaryCertificateThumbprint = $IotHubDeviceSecondaryCertificate.Thumbprint
            SecondaryPrivateKeyBase64 = $(ConvertTo-Base64 -Content $(Export-Pkcs8PrivateKeyPem -Key $IotHubDevicePrivateKey))
            ConnectionString = $ConnectionString.connectionString
        }

        $TestEnvInfo.IotHubX509ThumbprintDevices += $DeviceMinimalInfo
    }

    # TODO: implement this...
    # $IotHubX509CADevices = 0
    # IotHubX509CADevices = @()

    $DpsSymmKeyEnrollmentIdPrefix = "test-enrollment-sk"
    $DpsX509EnrollmentIdPrefix = "test-enrollment-x509"

    # Create all enrollments 
    if ($NoDps -eq $false) {
        for ($i = 0; $i -lt $DpsSymmKeyIndividualEnrollments; $i++) {
            $EnrollmentId = "$DpsSymmKeyEnrollmentIdPrefix-$i"
            $EnrollmentInfo = New-DpsSymmetricKeyIndividualEnrollment -ResourceGroup $ResourceGroup -DpsName $DpsName -EnrollmentId $EnrollmentId -AdrPolicyName $AzureAdrPolicyName

            $EnrollmentMininalInfo = [pscustomobject]@{
                Id = $EnrollmentId
                PrimaryKey = $EnrollmentInfo.attestation.symmetricKey.primaryKey
                SecondaryKey = $EnrollmentInfo.attestation.symmetricKey.secondaryKey
            }

            $TestEnvInfo.DpsIndividualSymKeyEnrollments += $EnrollmentMininalInfo
        }

        for ($i = 0; $i -lt $DpsX509IndividualEnrollments; $i++) {
            $EnrollmentId = "$DpsX509EnrollmentIdPrefix-$i"
            $EnrollmentInfo = New-DpsX509IndividualEnrollment -ResourceGroup $ResourceGroup -DpsName $DpsName -EnrollmentId $EnrollmentId -AdrPolicyName $AzureAdrPolicyName -CertDir $CertDir -PrivateDir $PrivateDir

            $EnrollmentMininalInfo = [pscustomobject]@{
                Id = $EnrollmentId
                CertificateBase64 = ConvertTo-Base64 -Content $(gc $EnrollmentInfo.DpsDeviceCertificatePath)
                PrivateKeyBase64 = ConvertTo-Base64 -Content $(gc $EnrollmentInfo.DpsDevicePrivateKeyPath)
            }

            $TestEnvInfo.DpsIndividualX509Enrollments += $EnrollmentMininalInfo
        }

        if ($DpsSymmKeyGroupEnrollmentDevices -gt 0) {
            $EnrollmentId = "$DpsSymmKeyEnrollmentIdPrefix-group"
            $EnrollmentInfo = New-DpsSymmetricKeyEnrollmentGroup -ResourceGroup $ResourceGroup -DpsName $DpsName -EnrollmentId $EnrollmentId -AdrPolicyName $AzureAdrPolicyName

            for ($i = 0; $i -lt $DpsSymmKeyGroupEnrollmentDevices; $i++) {
                $DeviceId = "group-prov-sk-$i"

                $ProvDeviceInfo = [pscustomobject]@{
                    Id = $DeviceId
                    PrimaryKey = $(New-DpsDerivedSymmetricKey -SymmetricKey $EnrollmentInfo.attestation.symmetricKey.primaryKey -DeviceId $DeviceId)
                    SecondaryKey = $(New-DpsDerivedSymmetricKey -SymmetricKey $EnrollmentInfo.attestation.symmetricKey.secondaryKey -DeviceId $DeviceId)
                }

                $TestEnvInfo.DpsGroupSymKeyEnrollments += $ProvDeviceInfo   
            }
        }

        if ($DpsX509GroupEnrollmentDevices -gt 0) {
            $EnrollmentId = "$DpsX509EnrollmentIdPrefix-group"
            New-DpsX509EnrollmentGroup -ResourceGroup $ResourceGroup -DpsName $DpsName -EnrollmentId $EnrollmentId -AdrPolicyName $AzureAdrPolicyName -IntermediateCACertificatePath $DpsCertificates.IntermediateCACertificatePath -IotHubFqdn $IotHubFqdn

            for ($i = 0; $i -lt $DpsX509GroupEnrollmentDevices; $i++) {
                $DeviceId = "group-prov-x509-$i"
                $DpsDevicePrivateKeyPath = "$PrivateDir/$DeviceId.key.pem"
                $DpsDeviceCertificatePath = "$CertDir/$DeviceId.cert.pem"
                $DpsDevicePrivateKey = New-PrivateKey -Path $DpsDevicePrivateKeyPath
                $DpsDeviceCertificate = New-Certificate -Subject "CN=$DeviceId" -Key $DpsDevicePrivateKey -IssuerCert $DpsCertificates.IntermediateCACertificate -IssuerKey $DpsCertificates.IntermediateCAPrivateKey -IsCA $false -Days 30
                Export-X509CertificateToPemFile -Cert $DpsDeviceCertificate -Path $DpsDeviceCertificatePath

                $ProvDeviceInfo = [pscustomobject]@{
                    Id = $DeviceId
                    CertificateBase64 = ConvertTo-Base64 -Content $(gc $DpsDeviceCertificatePath)
                    PrivateKeyBase64 = ConvertTo-Base64 -Content $(gc $DpsDevicePrivateKeyPath)
                }

                $TestEnvInfo.DpsGroupX509Enrollments += $ProvDeviceInfo   
            }
        }
    }

    # File Upload
    if ($EnableFileUpload -eq $true) {
        Write-Host "Creating Azure Storage account ($StorageAccountName)"
        $AzureStorageAccount = az storage account create --name "$StorageAccountName" --resource-group "$ResourceGroup" --location "$AzureLocation" --sku Standard_LRS --kind StorageV2 | ConvertFrom-Json
        Stop-OnError -Step "Creating Azure Storage account"

        $AzureStorageContainerName = "iothubuploads"

        Write-Host "Creating Azure Storage container ($AzureStorageContainerName on $StorageAccountName)"
        az storage container create --name $AzureStorageContainerName --account-name "$StorageAccountName" --only-show-errors | Out-Null
        Stop-OnError -Step "Creating Azure Storage container"

        Write-Host "Getting Azure Storage account connection string"
        $AzureStorageConnectionString=$(az storage account show-connection-string --name "$StorageAccountName" --resource-group "$ResourceGroup" --query connectionString -o tsv)
        Stop-OnError -Step "Getting Azure Storage account connection string"

        if ($EnableCertificateManagement -eq $true) {
            Write-Host "Updating Azure IoT Hub file upload settings (certificate management)"
            az iot hub update --name "$IotHubName" --resource-group "$ResourceGroup" --fcs "$AzureStorageConnectionString" --fc $AzureStorageContainerName --fileupload-sas-ttl 1 --ns-identity-id "$($AzureAdrNamespace.identity.principalId)" | Out-Null
            Stop-OnError -Step "Updating Azure IoT Hub file upload settings (certificate management)"
        } else {    
            Write-Host "Updating Azure IoT Hub file upload settings"
            az iot hub update --name "$IotHubName" --resource-group "$ResourceGroup" --fcs "$AzureStorageConnectionString" --fc $AzureStorageContainerName --fileupload-sas-ttl 1 | Out-Null
            Stop-OnError -Step "Updating Azure IoT Hub file upload settings"
        }
    }


    # Gathering Test Environment settings.
    $TestEnvInfo.AzureResourceGroup = $ResourceGroup
    $TestEnvInfo.AzureAdrPolicyName = $AzureAdrPolicyName

    Write-Host "Getting IoT Hub Connection String"
    $TestEnvInfo.IotHubConnectionString = $(az iot hub connection-string show -g $ResourceGroup -n $IotHubName --kt primary --pn iothubowner --query connectionString -o tsv)
    Stop-OnError -Step "Get IoT Hub Connection String"

    Write-Host "Getting IoT Hub's Event Hub Connection String"
    $TestEnvInfo.IotHubEventHubConnectionString = $(az iot hub connection-string show -g $ResourceGroup -n $IotHubName --kt primary --pn iothubowner --eh --query connectionString -o tsv)
    Stop-OnError -Step "Get IoT Hub's Event Hub Connection String"

    $TestEnvInfo.IotHubEventHubCompatibleName = $AzureIoTHub.properties.eventHubEndpoints.events.path
    $TestEnvInfo.IotHubEventHubPartitionCount = $AzureIoTHub.properties.eventHubEndpoints.events.partitionCount

    Write-Host "Getting IoT Hub's Event Hub Consumer Groups"
    az iot hub consumer-group list --hub-name $IotHubName --resource-group $ResourceGroup | ConvertFrom-Json | %{ $TestEnvInfo.IotHubEventHubConsumerGroups += $_.name }
    Stop-OnError -Step "Get IoT Hub's Event Hub Consumer Groups"

    if ($NoDps -eq $false) {
        $TestEnvInfo.DpsDeviceFqdn = $AzureDps.properties.deviceProvisioningHostName
        $TestEnvInfo.DpsServiceFqdn = $AzureDps.properties.serviceOperationsHostName
        $TestEnvInfo.DpsIdScope = $AzureDps.properties.idScope

        Write-Host "Getting DPS Connection String"
        $TestEnvInfo.DpsConnectionString = $(az iot dps connection-string show -g $ResourceGroup -n $DpsName --kt primary --pn provisioningserviceowner --query connectionString -o tsv)
        Stop-OnError -Step "Get DPS Connection String"

        $TestEnvInfo.DpsRootCACertificatePath = $DpsCertificates.RootCACertificatePath
        $TestEnvInfo.DpsRootCAPrivateKeyPath = $DpsCertificates.RootCAPrivateKeyPath
        $TestEnvInfo.DpsIntermediateCACertificatePath = $DpsCertificates.IntermediateCACertificatePath
        $TestEnvInfo.DpsIntermediateCAPrivateKeyPath = $DpsCertificates.IntermediateCAPrivateKeyPath
    }

    return $TestEnvInfo
}

function New-AzIotCSDKE2ETestConfig {
    param(
        $TestEnvInfo = $null,
        [ValidateSet('powershell', 'bash')]
        [string]$Target = "bash",
        $OutFile
    )

    if ($OutFile -eq $null) {
        $OutFile = "./azure-iot-sdk-c-e2e-test-config"
        if ($Target -eq "powershell") {
            $OutFile += ".ps1"
        } else {
            $OutFile += ".sh"
        }
    }

    if ($Target -eq "powershell") {
        $Lines = @(
            "`$env:IOTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubConnectionString)`""
            "`$env:IOTHUB_EVENTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "`$env:IOTHUB_EVENTHUB_LISTEN_NAME = `"$($TestEnvInfo.IotHubEventHubCompatibleName)`""
            "`$env:IOTHUB_PARTITION_COUNT = $($TestEnvInfo.IotHubEventHubPartitionCount)"
            "`$env:IOT_DPS_GLOBAL_ENDPOINT = `"$($TestEnvInfo.DpsDeviceFqdn)`""
            "`$env:IOT_DPS_CONNECTION_STRING = `"$($TestEnvInfo.DpsConnectionString)`""
            "`$env:IOT_DPS_ID_SCOPE = `"$($TestEnvInfo.DpsIdScope)`""
            "`$env:IOTHUB_DEVICE_CONN_STRING_INVALIDCERT = `"HostName=invalidcertiothub1.westus.cloudapp.azure.com;DeviceId=DoNotDelete1;SharedAccessKey=zWmeTGWmjcgDG1dpuSCVjc5ZY4TqVnKso5+g1wt/K3E=`""
            "`$env:IOTHUB_CONN_STRING_INVALIDCERT = `"HostName=invalidcertiothub1.westus.cloudapp.azure.com;SharedAccessKeyName=iothubowner;SharedAccessKey=Fk1H0asPeeAwlRkUMTybJasksTYTd13cgI7SsteB05U=`""
            "`$env:DPS_GLOBALDEVICEENDPOINT_INVALIDCERT = `"invalidcertgde1.westus.cloudapp.azure.com`""
            "`$env:PROVISIONING_CONNECTION_STRING_INVALIDCERT = `"HostName=invalidcertdps1.westus.cloudapp.azure.com;SharedAccessKeyName=provisioningserviceowner;SharedAccessKey=lGO7OlXNhXlFyYV1rh9F/lUCQC1Owuh5f/1P0I1AFSY=`""
            "`$env:IOTHUB_E2E_X509_CERT_BASE64 = `"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryCertificateBase64)`""
            "`$env:IOTHUB_E2E_X509_PRIVATE_KEY_BASE64 = `"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryPrivateKeyBase64)`""
            "`$env:IOTHUB_E2E_X509_THUMBPRINT = `"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryCertificateThumbprint)`""
            "`$env:IOT_DPS_INDIVIDUAL_X509_CERTIFICATE = `"$($TestEnvInfo.DpsIndividualX509Enrollments[0].CertificateBase64)`""
            "`$env:IOT_DPS_INDIVIDUAL_X509_KEY = `"$($TestEnvInfo.DpsIndividualX509Enrollments[0].PrivateKeyBase64)`""
            "`$env:IOT_DPS_INDIVIDUAL_REGISTRATION_ID = `"$($TestEnvInfo.DpsIndividualX509Enrollments[0].Id)`""
            "`$env:AZURE_RESOURCE_GROUP = `"$($TestEnvInfo.AzureResourceGroup)`""
        )
    } else { # bash
        $Lines = @(
            "#!/bin/bash"
            "export IOTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubConnectionString)`""
            "export IOTHUB_EVENTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "export IOTHUB_EVENTHUB_LISTEN_NAME=`"$($TestEnvInfo.IotHubEventHubCompatibleName)`""
            "export IOTHUB_PARTITION_COUNT=$($TestEnvInfo.IotHubEventHubPartitionCount)"
            "export IOT_DPS_GLOBAL_ENDPOINT=`"$($TestEnvInfo.DpsDeviceFqdn)`""
            "export IOT_DPS_CONNECTION_STRING=`"$($TestEnvInfo.DpsConnectionString)`""
            "export IOT_DPS_ID_SCOPE=`"$($TestEnvInfo.DpsIdScope)`""
            "export IOTHUB_DEVICE_CONN_STRING_INVALIDCERT=`"HostName=invalidcertiothub1.westus.cloudapp.azure.com;DeviceId=DoNotDelete1;SharedAccessKey=zWmeTGWmjcgDG1dpuSCVjc5ZY4TqVnKso5+g1wt/K3E=`""
            "export IOTHUB_CONN_STRING_INVALIDCERT=`"HostName=invalidcertiothub1.westus.cloudapp.azure.com;SharedAccessKeyName=iothubowner;SharedAccessKey=Fk1H0asPeeAwlRkUMTybJasksTYTd13cgI7SsteB05U=`""
            "export DPS_GLOBALDEVICEENDPOINT_INVALIDCERT=`"invalidcertgde1.westus.cloudapp.azure.com`""
            "export PROVISIONING_CONNECTION_STRING_INVALIDCERT=`"HostName=invalidcertdps1.westus.cloudapp.azure.com;SharedAccessKeyName=provisioningserviceowner;SharedAccessKey=lGO7OlXNhXlFyYV1rh9F/lUCQC1Owuh5f/1P0I1AFSY=`""
            "export IOTHUB_E2E_X509_CERT_BASE64=`"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryCertificateBase64)`""
            "export IOTHUB_E2E_X509_PRIVATE_KEY_BASE64=`"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryPrivateKeyBase64)`""
            "export IOTHUB_E2E_X509_THUMBPRINT=`"$($TestEnvInfo.IotHubX509ThumbprintDevices[0].PrimaryCertificateThumbprint)`""
            "export IOT_DPS_INDIVIDUAL_X509_CERTIFICATE=`"$($TestEnvInfo.DpsIndividualX509Enrollments[0].CertificateBase64)`""
            "export IOT_DPS_INDIVIDUAL_X509_KEY=`"$($TestEnvInfo.DpsIndividualX509Enrollments[0].PrivateKeyBase64)`""
            "export IOT_DPS_INDIVIDUAL_REGISTRATION_ID=`"$($TestEnvInfo.DpsIndividualX509Enrollments[0].Id)`""
            "export AZURE_RESOURCE_GROUP=`"$($TestEnvInfo.AzureResourceGroup)`""
        )
    }

    $Content = $($Lines -join "`n") + "`n"

    Set-FileContent -Path "$OutFile" -Content "$Content"

    Write-Host "End-to-End test configuration written to $OutFile"

    return $OutFile

    # Cut list?
        #   IOTHUB_POLICY_KEY: $(IOTHUB-POLICY-KEY) OJdGPkx9HgWecCSECw7D7Hv8AuiKE+A7TWjAcEUv5tk=
        #   IOTHUB_CA_ROOT_CERT: $(IOTHUB-CA-ROOT-CERT)
        #   IOTHUB_CA_ROOT_CERT_KEY: $(IOTHUB-CA-ROOT-CERT-KEY)
}

function New-AzIotPythonSDKE2ETestConfig {
    param(
        $TestEnvInfo = $null,
        [ValidateSet('powershell', 'bash')]
        [string]$Target = "bash",
        [switch]$EnableCertificateManagement,
        $OutFile
    )

    if ($OutFile -eq $null) {
        $OutFile = "./azure-iot-sdk-python-e2e-test-config"
        if ($Target -eq "powershell") {
            $OutFile += ".ps1"
        } else {
            $OutFile += ".sh"
        }
    }

# registration_id = os.getenv("PROVISIONING_REGISTRATION_ID")

# dps_x509_cert_file = os.getenv("PROVISIONING_X509_CERT_FILE")
# dps_x509_key_file = os.getenv("PROVISIONING_X509_KEY_FILE")

# dps_sas_key = os.getenv("PROVISIONING_SYMMETRIC_KEY")

# dps_csr_data = os.getenv("PROVISIONING_CSR")
# dps_csr_key_file = os.getenv("PROVISIONING_CSR_KEY_FILE")
# dps_issued_cert_file = os.getenv("PROVISIONING_ISSUED_CERT_FILE")

# iothub_csr_data = os.getenv("IOTHUB_CSR")
# iothub_csr_key_file = dps_csr_key_file  # Must be the same.
# iothub_issued_cert_file = os.getenv("IOTHUB_ISSUED_CERT_FILE")

    $DpsRootCACertificateBase64 = ConvertTo-Base64 -Content $(gc $TestEnvInfo.DpsRootCACertificatePath)
    $DpsRootCAPrivateKeyBase64 = ConvertTo-Base64 -Content $(gc $TestEnvInfo.DpsRootCAPrivateKeyPath)

    if ($Target -eq "powershell") {
        $Lines = @(
            "`$env:IOTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubConnectionString)`""
            "`$env:IOTHUB_E2E_IOTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubConnectionString)`""
            "`$env:IOTHUB_EVENTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "`$env:IOTHUB_E2E_EVENTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "`$env:EVENTHUB_CONNECTION_STRING = `"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "`$env:IOTHUB_E2E_EVENTHUB_CONSUMER_GROUP = `"``$($TestEnvInfo.IotHubEventHubConsumerGroups[0])`""
            "`$env:EVENTHUB_CONSUMER_GROUP = `"``$($TestEnvInfo.IotHubEventHubConsumerGroups[0])`""

            "`$env:PROVISIONING_DEVICE_IDSCOPE = `"$($TestEnvInfo.DpsIdScope)`""
            "`$env:PROVISIONING_IDSCOPE = `"$($TestEnvInfo.DpsIdScope)`""
            "`$env:PROVISIONING_DEVICE_ENDPOINT = `"$($TestEnvInfo.DpsDeviceFqdn)`""
            "`$env:PROVISIONING_HOST = `"$($TestEnvInfo.DpsIdScope)`""
            "`$env:PROVISIONING_SERVICE_CONNECTION_STRING = `"$($TestEnvInfo.DpsConnectionString)`""

            "`$env:PROVISIONING_ROOT_CERT = `"$DpsRootCACertificateBase64`""
            "`$env:PROVISIONING_ROOT_CERT_KEY = `"$DpsRootCAPrivateKeyBase64`""
            "`$env:PROVISIONING_ROOT_PASSWORD = `"`""

            "`$env:ADR_CERT_MGMT_POLICY_NAME = `"$($TestEnvInfo.AzureAdrPolicyName)`""

            "`$env:PYTHONUNBUFFERED = `"True`""

            "`$env:AZURE_RESOURCE_GROUP = `"$($TestEnvInfo.AzureResourceGroup)`""
        )
    } else { # bash
        $Lines = @(
            "#!/bin/bash"
            "export IOTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubConnectionString)`""
            "export IOTHUB_E2E_IOTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubConnectionString)`""
            "export IOTHUB_EVENTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "export IOTHUB_E2E_EVENTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "export EVENTHUB_CONNECTION_STRING=`"$($TestEnvInfo.IotHubEventHubConnectionString)`""
            "export IOTHUB_E2E_EVENTHUB_CONSUMER_GROUP=`"`\$($TestEnvInfo.IotHubEventHubConsumerGroups[0])`""
            "export EVENTHUB_CONSUMER_GROUP=`"`\$($TestEnvInfo.IotHubEventHubConsumerGroups[0])`""


            "export PROVISIONING_DEVICE_IDSCOPE=`"$($TestEnvInfo.DpsIdScope)`""
            "export PROVISIONING_IDSCOPE=`"$($TestEnvInfo.DpsIdScope)`""
            "export PROVISIONING_DEVICE_ENDPOINT=`"$($TestEnvInfo.DpsDeviceFqdn)`""
            "export PROVISIONING_HOST=`"$($TestEnvInfo.DpsIdScope)`""
            "export PROVISIONING_SERVICE_CONNECTION_STRING=`"$($TestEnvInfo.DpsConnectionString)`""

            "export PROVISIONING_ROOT_CERT=`"$DpsRootCACertificateBase64`""
            "export PROVISIONING_ROOT_CERT_KEY=`"$DpsRootCAPrivateKeyBase64`""
            "export PROVISIONING_ROOT_PASSWORD=`"`""

            "export ADR_CERT_MGMT_POLICY_NAME=`"$($TestEnvInfo.AzureAdrPolicyName)`""

            "export PYTHONUNBUFFERED=`"True`""

            "export AZURE_RESOURCE_GROUP=`"$($TestEnvInfo.AzureResourceGroup)`""
        )
    }

    $Content = $($Lines -join "`n") + "`n"

    Set-FileContent -Path "$OutFile" -Content "$Content"

    Write-Host "End-to-End test configuration written to $OutFile"

    return $OutFile
}

Export-ModuleMember -Function New-AzureResourceGroupName
Export-ModuleMember -Function New-AzIotTestEnvironment
Export-ModuleMember -Function New-AzIotCSDKE2ETestConfig
Export-ModuleMember -Function New-AzIotPythonSDKE2ETestConfig