<#!
.SYNOPSIS
    Installs and validates the toolchain prerequisites via winget.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Add-PathEntry {
    param(
        [Parameter(Mandatory)] [string] $PathEntry
    )

    $expanded = [Environment]::ExpandEnvironmentVariables($PathEntry)
    if(-not (Test-Path $expanded)) {
        return
    }

    $systemPath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $systemParts = @()
    if($systemPath) {
        $systemParts = $systemPath -split ';' | Where-Object { $_ }
    }

    $existsInSystem = $false
    foreach($part in $systemParts) {
        if($part.Trim() -ieq $expanded.Trim()) {
            $existsInSystem = $true
            break
        }
    }

    if(-not $existsInSystem) {
        try {
            $updated = ($systemParts + $expanded) -join ';'
            [Environment]::SetEnvironmentVariable('Path', $updated, 'Machine')
            Write-Host "Added ${expanded} to system PATH"
        } catch {
            Write-Warning "Unable to update system PATH with ${expanded}: $($PSItem.Exception.Message)"
        }
    }

    $sessionParts = $env:Path -split ';'
    $existsInSession = $false
    foreach($part in $sessionParts) {
        if($part.Trim() -ieq $expanded.Trim()) {
            $existsInSession = $true
            break
        }
    }

    if(-not $existsInSession) {
        $env:Path = "$env:Path;$expanded"
    }
}

function Test-Executable {
    param(
        [Parameter(Mandatory)] [string] $Command
    )

    return [bool](Get-Command $Command -ErrorAction SilentlyContinue)
}

function Install-WingetPackage {
    param(
        [Parameter(Mandatory)] [string] $Id,
        [Parameter(Mandatory)] [string] $DisplayName,
        [Parameter(Mandatory)] [string] $Command,
        [Parameter()] [string[]] $CandidatePaths = @()
    )

    if(Test-Executable $Command) {
        Write-Host "$DisplayName already available"
    } else {
        Write-Host "Installing $DisplayName via winget..."
        $installArgs = @('install', '--exact', '--id', $Id, '--silent', '--accept-package-agreements', '--accept-source-agreements')
        $process = Start-Process -FilePath 'winget' -ArgumentList $installArgs -Wait -PassThru
        if($process.ExitCode -ne 0) {
            throw "winget failed to install $DisplayName (exit code $($process.ExitCode))"
        }
    }

    $commandInfo = Get-Command $Command -ErrorAction SilentlyContinue
    if($commandInfo) {
        Add-PathEntry -PathEntry (Split-Path -Parent $commandInfo.Source)
        return
    }

    foreach($candidate in $CandidatePaths) {
        Add-PathEntry -PathEntry $candidate
    }

    if(-not (Test-Executable $Command)) {
        throw "$DisplayName is not accessible via PATH. Restart the shell or verify the installation."
    }
}

function Assert-Winget {
    if(-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw 'winget CLI is required. Install winget (App Installer) from the Microsoft Store first.'
    }
}

function Get-VulkanSdkPath {
    $candidates = @(
        [Environment]::GetEnvironmentVariable('VULKAN_SDK', 'Process'),
        [Environment]::GetEnvironmentVariable('VULKAN_SDK', 'User'),
        [Environment]::GetEnvironmentVariable('VULKAN_SDK', 'Machine')
    ) | Where-Object { $_ }

    foreach($candidate in $candidates) {
        if(Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Assert-VulkanSdk {
    $sdkRoot = Get-VulkanSdkPath
    if(-not $sdkRoot) {
        Write-Warning 'VULKAN_SDK is not set. Install the Vulkan SDK from https://vulkan.lunarg.com/ and set VULKAN_SDK.'
        return
    }

    $relativeExecutables = @(
        'Bin\vulkaninfo.exe',
        'Bin64\vulkaninfo.exe',
        'Bin\vulkaninfoSDK.exe',
        'Bin64\vulkaninfoSDK.exe',
        'Bin32\vulkaninfo.exe'
    )

    foreach($relative in $relativeExecutables) {
        $candidatePath = Join-Path $sdkRoot $relative
        if(Test-Path $candidatePath) {
            Write-Host "Vulkan SDK detected at $sdkRoot (found ${relative})"
            return
        }
    }

    Write-Warning "Vulkan SDK path '$sdkRoot' does not contain a vulkaninfo executable. Checked: $([string]::Join(', ', $relativeExecutables))."
}

Assert-Winget

if(-not (Test-IsAdmin)) {
    Write-Warning 'Running without administrator rights. You may be prompted during winget installs.'
}

$packages = @(
    @{ Id = 'LLVM.LLVM'; Name = 'LLVM/Clang'; Command = 'clang'; Paths = @('C:\\Program Files\\LLVM\\bin') },
    @{ Id = 'Kitware.CMake'; Name = 'CMake'; Command = 'cmake'; Paths = @('C:\\Program Files\\CMake\\bin') },
    @{ Id = 'Ninja-build.Ninja'; Name = 'Ninja'; Command = 'ninja'; Paths = @('C:\\Program Files\\Ninja', 'C:\\Program Files\\Ninja\\bin') }
)

foreach($package in $packages) {
    Install-WingetPackage -Id $package.Id -DisplayName $package.Name -Command $package.Command -CandidatePaths $package.Paths
}

Assert-VulkanSdk
Write-Host 'Dependency installation and validation complete.'
