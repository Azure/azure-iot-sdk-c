param([switch]$CloneRepo, [switch]$UpdateSubmodulesFirst, [switch]$PushOnline) 
  
$TimeStamp = (Get-Date).ToString("yyyyMMddHHmm") 
$SubUpBranch = "subup-$TimeStamp" 
  
function Update-MasterBranch 
{ 
    git checkout master 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed checking out master branch" 
        return $false 
    } 
  
    git pull 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed updating master branch" 
        return $false 
    } 
  
    return $true 
} 
  
function Update-CSharedUtility() { 
    param($CSharedUtilityDir) 
  
    $OriginalDir=(Get-Location).Path 
  
    cd $CSharedUtilityDir 
  
    if (-NOT (Update-MasterBranch)) 
    { 
        Write-Host "Failed updating master branch on $CSharedUtilityDir" 
        return $false 
    }     
  
    cd $OriginalDir 
  
    return $true 
} 
  
function Add-GitChanges() { 
    git checkout -b $SubUpBranch 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed creating branch for submodule update" 
        return $false 
    } 
  
    git add . 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed adding changes" 
        return $false 
    } 
  
    git commit -m "Update submodules" 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed commiting changes" 
        return $false 
    } 
  
    return $true 
} 
  
function Update-Submodule() { 
    param($SubmoduleDir, $CSharedUtilityDir) 
  
    $OriginalDir=(Get-Location).Path 
  
    cd $SubmoduleDir 
  
    if (-NOT (Update-MasterBranch)) 
    { 
        Write-Host "Failed updating master on $SubmoduleDir submodule" 
        return $false 
    } 
  
    if (-NOT (Update-CSharedUtility -CSharedUtilityDir $CSharedUtilityDir)) 
    { 
        Write-Host "Failed updating c-utility on $SubmoduleDir submodule" 
        return $false 
    } 
  
    if (-NOT (Add-GitChanges)) 
    { 
        Write-Host "Failed adding changes on $SubmoduleDir submodule" 
        return $false 
    } 
  
    if ($PushOnline) 
    { 
        git push origin $SubUpBranch 
  
        if ($LASTEXITCODE -NE 0) 
        { 
            Write-Host "Failed pushing changes online for $SubmoduleDir" 
            return $0 
        } 
    } 
  
    cd $OriginalDir 
  
    return $true 
} 
  
if ($CloneRepo) 
{ 
    #Clonning main repo 
    git clone --recursive https://github.com/azure/azure-iot-sdk-c.git 
  
    if ($LASTEXITCODE -NE 0) 
    { 
        Write-Host "Failed clonning main azure-iot-sdk-c repo" 
        return $0 
    } 
  
    cd azure-iot-sdk-c 
} 
  
$Dirs = @( 
    @("uamqp", "deps/azure-c-shared-utility"), 
    @("umqtt", "deps/c-utility"), 
    @("deps/uhttp", "deps/c-utility"), 
    @("provisioning_client/deps/utpm", "deps/c-utility") 
) 
  
if ($UpdateSubmodulesFirst) 
{ 
    for ($i = 0; $i -LT $Dirs.Length; $i++) 
    { 
        if (-NOT (Update-Submodule -SubmoduleDir $Dirs[$i][0] -CSharedUtilityDir $Dirs[$i][1])) 
        { 
            Write-Host "Failed updating $($Dirs[$i][0]) submodule" 
            return $false 
        } 
    } 
} 
else 
{ 
    for ($i = 0; $i -LT $Dirs.Length; $i++) 
    { 
        $OriginalDir=(Get-Location).Path 
  
        cd $($Dirs[$i][0]) 
  
        if (-NOT (Update-MasterBranch)) 
        { 
            Write-Host "Failed updating utmp-c submodule" 
            return $false 
        } 
  
        cd $OriginalDir 
    } 
  
    # c-utility update 
    if (-NOT (Update-CSharedUtility "c-utility")) 
    { 
        Write-Host "Failed updating master on c-utility" 
        return $false 
    } 
  
    # Adding changes on main repo 
    if (-NOT (Add-GitChanges)) 
    { 
        Write-Host "Failed adding changes on main repo" 
        return $false 
    } 
  
    if ($PushOnline) 
    { 
        git push origin $SubUpBranch 
  
        if ($LASTEXITCODE -NE 0) 
        { 
            Write-Host "Failed pushing changes online for $SubmoduleDir" 
            return $0 
        } 
    } 
} 
  
return $true 
