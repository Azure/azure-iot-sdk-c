# Introduction
# This guide shows the steps to merge the following submodules into the azure-iot-sdk-c (v1) repository.
# For any other submodule, the steps will have to be executed manually.
#	• c-utility
#	• uamqp
#	• umqtt
#	• deps/uhttp
#	• provisioning_client/deps/utpm
#
# Getting Started
#	1. The script you will run is written for PowerShell. Please enable scripts to run on your PowerShell
#      instance with this command:
#
#	   Set-ExecutionPolicy -ExecutionPolicy Unrestricted
# 2. To automatically create Pull Requests, this script uses the Github CLI found here:
#        https://cli.github.com
#    Please install it prior to running the script.
#
#	2. Clone the azure-iot-sdk-c repo and recursively include all submodules:
#
#	   git clone --recursive https://github.com/azure/azure-iot-sdk-c.git
#
#	3. Enter the azure-iot-sdk-c directory.
#
# Running the Script - Part 1
# The following four submodules must be merged first:
#	• uamqp
#	• umqtt
#	• daps/uhttp
#	• provisioning_client/deps/utpm
#
#	1. Run the script with the following flags:
#
#	   .\Update-AzIoTSdkCSubmodules.ps1 -UpdateSubmodulesFirst -PushOnline
#
#	   This step will update the four submodules to point to the most recent version of the c-utility
#      submodule.  For each of the four submodules, this update will be committed on a new branch labeled
#      subup-<timestamp>.
#
#	2. When the script completes, you will see output similar to the following for each submodule:
#		Switched to branch 'master'
#		Switched to branch 'master'
#		Switched to a new branch 'subup-202008191105'
#		remote:
#		remote: Create a pull request for 'subup-202008191105' on GitHub by visiting:
#		remote:      https://github.com/Azure/azure-utpm-c/pull/new/subup-202008191105
#		remote:
#
#	3. Follow the supplied links and create a pull request for each of the four submodules.
#	   IMPORTANT: Have each PR approved as soon as possible.
#
# Running the Script - Part 2
# When the four prior submodules have been successfully merged, and each PR is closed, you can complete
# the update of the azure-iot-sdk-c repo.
#
#	1. Run the script with the following flag:
#
#	   .\Update-AzIoTSdkCSubmodules.ps1 -PushOnline
#
#	   This step will update the azure-iot-sdk-c repo to point to the most recent version of all five
#      submodules.  The update will be committed on a new branch labeled subup-<timestamp>.
#
#	2. When the script completes, you will see output similar to the following for each submodule:
#		Switched to branch 'master'
#		Switched to branch 'master'
#		Switched to branch 'master'
#		Switched to branch 'master'
#		Switched to branch 'master'
#		Switched to a new branch 'subup-202008191123'
#		remote:
#		remote: Create a pull request for 'subup-202008191123' on GitHub by visiting:
#		remote:      https://github.com/Azure/azure-iot-sdk-c/pull/new/subup-202008191123
#		remote:
#
#	3. Follow the supplied link and create a pull request for the azure-iot-sdk-c repository.
#	   IMPORTANT: Have the PR approved as soon as possible.
#
param([switch]$UpdateSubmodulesFirst, [switch]$PushOnline)

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

        gh pr create --title "Submodule Update" --body "Submodule Update"

        if ($LASTEXITCODE -NE 0)
        {
            Write-Host "Make sure you have the Github CLI installed"
            return $0
        }
    }

    cd $OriginalDir

    return $true
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

        gh pr create --title "Submodule Update" --body "Submodule Update"

        if ($LASTEXITCODE -NE 0)
        {
            Write-Host "Make sure you have the Github CLI installed"
            return $0
        }
    }
}

return $true
