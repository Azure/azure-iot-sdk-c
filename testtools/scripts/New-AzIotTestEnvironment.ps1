param(
    $AzureLocation = "westus2",
    $AzureSubscriptionId = $null,
    $ResourceGroup = "rg-$([guid]::NewGuid().Guid.Replace('-', ''))",
    $DpsName       = "dps-$([guid]::NewGuid().Guid.Replace('-', ''))",
    $IotHubName    = "iothub-$([guid]::NewGuid().Guid.Replace('-', ''))",
    $IotHubDomainName = "azure-devices.net",
    $DeviceId      = "myDeviceId",
    $DpsSymmKeyEnrollmentId = "test-enrollment-group-sk",
    $DpsX509EnrollmentId = "test-enrollment-group-x509",
    $StorageAccountName = "teststoacc$([guid]::NewGuid().Guid.Replace('-', ''))".Substring(0, 24), # Max size of Storage Account name
    [switch]$EnableFileUpload,
    [switch]$NoDps,
    [switch]$EnableCertificateManagement
)

$TestEnvInfo = [pscustomobject]@{
    IotHubConnectionString = $null
    IotHubEventHubConnectionString = $null
    DpsDeviceFqdn = $null
    DpsServiceFqdn = $null
    DpsConnectionString = $null
    DpsIdScope = $null
    DpsRootCACertificatePath = $null
    DpsRootCAPrivateKeyPath = $null
    DpsIntermediateCACertificatePath = $null
    DpsIntermediateCAPrivateKeyPath = $null
}

$IotHubFqdn = "$($IotHubName).$($IotHubDomainName)"

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

function New-KeyPair {
    param([string]$Path)

    $rsa = [System.Security.Cryptography.RSA]::Create(4096)
    $pem = $rsa.ExportPkcs8PrivateKeyPem()
    Set-Content -Path $Path -Value $pem
    return $rsa
}

function New-Certificate {
    param(
        [string]$Subject,
        [System.Security.Cryptography.RSA]$Key,
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$IssuerCert,
        [System.Security.Cryptography.RSA]$IssuerKey,
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
        $KeyUsageExtension = [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new([System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature, $true)
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

    if ($IssuerCert.NotBefore -ne $null -and $NotBefore -lt $IssuerCert.NotBefore) {
        $NotBefore = $IssuerCert.NotBefore
    }

    if ($IssuerCert.NotAfter -ne $null -and $NotAfter -gt $IssuerCert.NotAfter) {
        $NotAfter = $IssuerCert.NotAfter
    }

    if ($IssuerCert -eq $null) {
        # Self-signed (Root CA)
        return $req.CreateSelfSigned($NotBefore, $NotAfter)
    } else {
        # Signed by issuer (Intermediate or Device)

        # Serial number must be random bytes (8–20 bytes is typical)
        $SerialNumber = [System.Security.Cryptography.RandomNumberGenerator]::GetBytes(16)

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

function Export-X509CertificateToPemFile {
    param([System.Security.Cryptography.X509Certificates.X509Certificate2]$Cert, [string]$Path)
    $pem = $Cert.ExportCertificatePem()
    Set-Content -Path $Path -Value $pem
}

function New-DpsCACertificateChain {
    param(
        $ResourceGroup,
        $DpsName,
        [TimeSpan]$CertificateExpiration = [TimeSpan]::FromDays(30)
    )

    $CertDir   = "$(pwd)/certs"
    $PrivateDir = "$(pwd)/private"
    $CsrDir    = "$(pwd)/csr"

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
        DpsRootCAName                 = "azure-iot-test-only-root-$Id"
    }

    # Root CA
    $rootKey = New-KeyPair -Path $CertInfo.RootCAPrivateKeyPath
    $rootCert = New-Certificate -Subject $CertInfo.RootCASubjectName -Key $rootKey -IssuerCert $null -IssuerKey $null -IsCA $true -Days $CertificateExpiration.Days
    Export-X509CertificateToPemFile -Cert $rootCert -Path $CertInfo.RootCACertificatePath

    # Intermediate CA
    $intKey = New-KeyPair -Path $CertInfo.IntermediateCAPrivateKeyPath
    $intCert = New-Certificate -Subject $CertInfo.IntermediateCASubjectName -Key $intKey -IssuerCert $rootCert -IssuerKey $rootKey -IsCA $true -Days $CertificateExpiration.Days
    Export-X509CertificateToPemFile -Cert $intCert -Path $CertInfo.IntermediateCACertificatePath

    az iot dps certificate create --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --path $CertInfo.RootCACertificatePath | Out-Null
    Stop-OnError -Step "az iot dps certificate create"

    $etag = az iot dps certificate show --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --query etag -o tsv
    Stop-OnError -Step "az iot dps certificate show"

    $DpsVerificationCodeInfo = az iot dps certificate generate-verification-code --dps-name $DpsName --resource-group $ResourceGroup --name $CertInfo.DpsRootCAName --etag $etag | ConvertFrom-Json
    Stop-OnError -Step "az iot dps certificate generate-verification-code"

    # Create verification cert
    $DpsVerificationKeyPath = "$PrivateDir/dps-root-ca-verification.key.pem"
    $DpsVerificationCertificatePath = "$CertDir/dps-root-ca-verification.cert.pem"
    $DpsVerificationKey = New-KeyPair -Path $DpsVerificationKeyPath
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

# Subscription id...
if ($AzureSubscriptionId -eq $null) {
    $AzureAccount = az account show 2>$null | ConvertFrom-Json
    Stop-OnError -Step "Get Azure account information"
    $AzureSubscriptionId = $AzureAccount.id
} else {
    $discard = az account set --subscription "$AzureSubscriptionId" --only-show-errors 2>$null
    Stop-OnError -Step "Set Azure subscription"
}

# Create resource group (if does not exist). 
$AzureResourceGroup = az group show --name "$ResourceGroup" 2>$null | ConvertFrom-Json
if ($AzureResourceGroup -eq $null) {
    Write-Host "Creating Azure resource group ($ResourceGroup)"
    $AzureResourceGroup = az group create --name "$ResourceGroup" --location $AzureLocation --only-show-errors 2>$null | ConvertFrom-json 
    Stop-OnError -Step "Create Azure resource group"
}

if ($EnableCertificateManagement -eq $true) {
    $ResouceGroupScope = "/subscriptions/$AzureSubscriptionId/resourceGroups/$ResourceGroup"
    Write-Host "Creating Azure role assignment for certificate management (scope=$ResouceGroupScope)"
    az role assignment create --assignee 89d10474-74af-4874-99a7-c23c2f643083 --role Contributor --scope "$ResouceGroupScope" --only-show-errors | Out-Null
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
    $AzureIoTHub = az iot hub create --name "$IotHubName" --resource-group "$ResourceGroup" --location "$AzureLocation" | ConvertFrom-Json
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

    if ($NoDps -eq $false) {    
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $DpsSymmKeyEnrollmentId --credential-policy $AzureAdrPolicyName | Out-Null
        Stop-OnError "Create an Azure Device Provisioning symmetric-key enrollment group (certificate management)"

        # Create enrollment group
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $DpsX509EnrollmentId --ap static --cp $DpsCertificates.IntermediateCACertificatePath --provisioning-status enabled --iot-hubs $IotHubFqdn --credential-policy $AzureAdrPolicyName | Out-Null
        Stop-OnError "Create an Azure Device Provisioning x509 enrollment group (certificate management)"
    }
} else {
    if ($NoDps -eq $false) {
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $DpsSymmKeyEnrollmentId | Out-Null
        Stop-OnError "Create an Azure Device Provisioning symmetric-key enrollment group"

        # Create enrollment group
        az iot dps enrollment-group create --dps-name $DpsName --resource-group $ResourceGroup --enrollment-id $DpsX509EnrollmentId --ap static --cp $DpsCertificates.IntermediateCACertificatePath --provisioning-status enabled --iot-hubs $IotHubFqdn | Out-Null
        Stop-OnError "Create an Azure Device Provisioning x509 enrollment group"
    }
}

# File Upload
if ($EnableFileUpload -eq $true) {
    Write-Host "Creating Azure Storage account ($StorageAccountName)"
    $AzureStorageAccount = az storage account create --name "$StorageAccountName" --resource-group "$ResourceGroup" --location "$AzureLocation" --sku Standard_LRS --kind StorageV2 | ConvertFrom-Json
    Stop-OnError "Creating Azure Storage account"

    $AzureStorageContainerName = "iothubuploads"

    Write-Host "Creating Azure Storage container ($AzureStorageContainerName on $StorageAccountName)"
    az storage container create --name $AzureStorageContainerName --account-name "$StorageAccountName" --only-show-errors | Out-Null
    Stop-OnError "Creating Azure Storage container"

    Write-Host "Getting Azure Storage account connection string"
    $AzureStorageConnectionString=$(az storage account show-connection-string --name "$StorageAccountName" --resource-group "$ResourceGroup" --query connectionString -o tsv)
    Stop-OnError "Getting Azure Storage account connection string"

    if ($EnableCertificateManagement -eq $true) {
        Write-Host "Updating Azure IoT Hub file upload settings (certificate management)"
        az iot hub update --name "$IotHubName" --resource-group "$ResourceGroup" --fcs "$AzureStorageConnectionString" --fc $AzureStorageContainerName --fileupload-sas-ttl 1 --ns-identity-id "$($AzureAdrNamespace.identity.principalId)" | Out-Null
        Stop-OnError "Updating Azure IoT Hub file upload settings (certificate management)"
    } else {    
        Write-Host "Updating Azure IoT Hub file upload settings"
        az iot hub update --name "$IotHubName" --resource-group "$ResourceGroup" --fcs "$AzureStorageConnectionString" --fc $AzureStorageContainerName --fileupload-sas-ttl 1 | Out-Null
        Stop-OnError "Updating Azure IoT Hub file upload settings"
    }
}


# Gathering Test Environment settings.
Write-Host "Getting IoT Hub Connection String"
$TestEnvInfo.IotHubConnectionString = $(az iot hub connection-string show -g $ResourceGroup -n $IotHubName --kt primary --pn iothubowner --query connectionString -o tsv)
Stop-OnError -Step "Get IoT Hub Connection String"

Write-Host "Getting IoT Hub's Event Hub Connection String"
$TestEnvInfo.IotHubEventHubConnectionString = $(az iot hub connection-string show -g $ResourceGroup -n $IotHubName --kt primary --pn iothubowner --eh --query connectionString -o tsv)
Stop-OnError -Step "Get IoT Hub's Event Hub Connection String"

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
