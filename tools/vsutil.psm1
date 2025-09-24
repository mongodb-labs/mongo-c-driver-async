Import-Module $PSScriptRoot/util -Force

function Get-VswhereExecutable {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        # Directory where cached files are stored
        [Parameter()]
        [string]
        $CachePath,
        # The version of vshwere to dowload
        [Parameter()]
        [string]
        $Version = "3.0.3",
        # Disable file caching
        [Parameter()]
        [switch]
        $NoCache
    )

    $ErrorActionPreference = "stop"

    return Get-CachedRemoteFile "vswhere/$Version/vswhere.exe" `
        -CachePath:$CachePath `
        -DownloadUri "https://github.com/microsoft/vswhere/releases/download/$Version/vswhere.exe" `
        -NoCache:$NoCache
}

function Get-VsInstallations {
    param (
        # The vshwere executable to be used
        [string]
        $Vswhere
    )

    $ErrorActionPreference = "stop"

    if ([string]::IsNullOrEmpty($Vswhere)) {
        $Vswhere = Get-VswhereExecutable
    }

    $vswhere_json = & $vswhere -utf8 -nologo -format json -all -legacy -prerelease -products * | Out-String
    return $vswhere_json | ConvertFrom-Json -Depth 20
}

<#
.SYNOPSIS
    Obtain the environment variables related to a Visual Studio environment and
    optional Windows SDK

.DESCRIPTION
    This script will load the specified Visual Studio environment with the
    specified options set, and then query the environment variables thereof, and
    return those as a hashtable

    This script makes use of vswhere.exe, which is installed with Visual Studio
    2017 or later, but supports all Visual Studio versions.

    Only the -Version and -TargetArch parameters are required.

.EXAMPLE
    PS C:\> Get-VsEnvironment -Version 14.* -TargetArch amd64

    This will load the Visual Studio 14 environment targetting amd64 processors.
#>
function Get-VsEnvironment {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        # Select a version of Visual Studio to activate. Accepts wildcards.
        #
        # Major versions by year release:
        #
        #   - 14.* => VS 2015
        #   - 15.* => VS 2017
        #   - 16.* => VS 2019
        #   - 17.* => VS 2022
        #
        # Use of a wildcard pattern in scripts is recommended for portability.
        #
        # Supports tab-completion if vswhere.exe is present.
        [Parameter(Mandatory)]
        # xxx: This requires PowerShell 5+, which some build hosts don't have:
        # [ArgumentCompleter({
        #         param($commandName, $paramName, $wordToComplete, $commandAst, $fakeBoundParameters)
        #         $vswhere_found = @(Get-ChildItem -Filter vswhere.exe `
        #                 -Path 'C:\Program Files*\Microsoft Visual Studio\Installer\' `
        #                 -Recurse)[0]
        #         if ($null -eq $vswhere_found) {
        #             Write-Host "No vswhere found"
        #             return $null
        #         }
        #         return & $vswhere_found -utf8 -nologo -format json -all -legacy -prerelease -products * `
        #         | ConvertFrom-Json `
        #         | ForEach-Object { $_.installationVersion } `
        #         | Where-Object { $_ -like "$wordToComplete*" }
        #     })]
        [string]
        $Version,
        # The target architecture for the build
        [Parameter(Mandatory)]
        [ValidateSet("x86", "amd64", "arm", "arm64", IgnoreCase = $false)]
        [string]
        $TargetArch,
        # Select a specific Windows SDK version.
        [string]
        # xxx: This requires PowerShell 5+, which some build hosts don't have:
        # [ArgumentCompleter({
        #         param($commandName, $paramName, $wordToComplete, $commandAst, $fakeBoundParameters)
        #         $found = @()
        #         if (Test-Path "${env:ProgramFiles(x86)}\Windows Kits\10\Include") {
        #             $found += $(
        #                 Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Include"`
        #                 | Where-Object { Test-Path "$($_.FullName)\um\Windows.h" }`
        #                 | ForEach-Object { Split-Path -Leaf $_.FullName }
        #             )
        #         }
        #         if (Test-Path "${env:ProgramFiles(x86)}\Windows Kits\8.1\Include") {
        #             $found += $(
        #                 Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\8.1\Include"`
        #                 | Where-Object { Test-Path "$($_.FullName)\um\Windows.h" }`
        #                 | ForEach-Object { Split-Path -Leaf $_.FullName }
        #             )
        #         }
        #         return $found | Where-Object { $_ -like "$wordToComplete*" }
        #     })]
        $WinSDKVersion,
        # The host architecture to use. Not usually needed. Defaults to x86.
        [ValidateSet("x86", "amd64", IgnoreCase = $false)]
        [string]
        $HostArch = "x86",
        # The Visual C++ toolset to load
        [string]
        $VCToolsetVersion,
        # Prefer Visual C++ libraries with Spectre mitigations
        [switch]
        $UseSpectreMitigationLibraries,
        # The app platform to load. Default is "Desktop"
        [ValidateSet("Desktop", "UWP", IgnoreCase = $false)]
        [string]
        $AppPlatform = "Desktop",
        # The directory to store ephemeral files
        [string]
        $ScratchDir
    )

    $ErrorActionPreference = 'Stop'

    $this_dir = $PSScriptRoot

    if ([string]::IsNullOrEmpty($ScratchDir)) {
        $ScratchDir = Join-Path (Split-Path -Parent $this_dir) "_build"
    }

    New-Item $ScratchDir -ItemType Directory -ErrorAction Ignore

    $vs_versions = Get-VsInstallations

    # Pick the product that matches the pattern
    $selected = @($vs_versions | Where-Object { $_.installationVersion -like $Version })

    if ($selected.Length -eq 0) {
        throw "No Visual Studio was found with a version matching '$Version'"
    }

    $selected = $selected[0]
    Write-Verbose "Selected Visual Studio version $($selected.installationVersion) [$($selected.installationPath)]"

    # Find Windows SDK version
    if (-not [String]::IsNullOrEmpty($WinSDKVersion)) {
        $sdk_avail = @()
        if (Test-Path "${env:ProgramFiles(x86)}\Windows Kits\10\Include") {
            $sdk_avail += $(
                Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Include" `
                | Where-Object { Test-Path "$($_.FullName)\um\Windows.h" } `
                | ForEach-Object { Split-Path -Leaf $_.FullName }
            )
        }
        if (Test-Path "${env:ProgramFiles(x86)}\Windows Kits\8.1\Include") {
            $sdk_avail += $(
                Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\8.1\Include" `
                | Where-Object { Test-Path "$($_.FullName)\um\Windows.h" } `
                | ForEach-Object { Split-Path -Leaf $_.FullName }
            )
        }
        Write-Debug "Detected Windows SDK versions: $sdk_avail"

        $sdk_selected = @($sdk_avail | Where-Object { $_ -like $WinSDKVersion })[0]
        if ($null -eq $sdk_selected) {
            throw "No Windows SDK version was found matching '$WinSDKVersion' (Of $sdk_avail)"
        }
        $WinSDKVersion = $sdk_selected
    }

    # Find the environment-activation script for the chosen VS
    $vsdevcmd_bat = @(Get-ChildItem `
            -Path $selected.installationPath `
            -Filter "VsDevCmd.bat" -Recurse)[0]

    $env_script_content = ""

    # Use batch and the 'set' command to get the required environment variables out.
    if ($null -eq $vsdevcmd_bat) {
        Write-Warning "No VsDevCmd.bat found for the requested VS version. Falling back to vcvarsall.bat"
        Write-Warning "Additional platform selection functionality will be limited"
        $vcvarsall_bat = @(Get-ChildItem -Path $selected.installationPath -Filter "vcvarsall.bat" -Recurse)[0]
        if ($null -eq $vcvarsall_bat) {
            throw "No VsDevCmd.bat nor vcvarsall.bat file found for requested Visual Studio version '$($selected.installationVersion)'"
        }
        Write-Debug "Using vcvarsall: [$($vcvarsall_bat.FullName)]"
        $env_script_content = @"
            @echo off
            call "$($vcvarsall_bat.FullName)" $TargetArch $WinSDKVersion
            set _rc=%ERRORLEVEL%
            set
            exit /b %_rc%
"@
    }
    else {
        # Build up the argument string to load the appropriate environment
        $argstr = "-no_logo -arch=$TargetArch -host_arch=$HostArch -app_platform=$AppPlatform"
        if ($UseSpectreMitigationLibraries) {
            $argstr += " -vcvars_spectre_libs=spectre"
        }
        if ($WinSDKVersion) {
            $argstr += " -winsdk=$WinSDKVersion"
        }
        if ($VCToolsetVersion) {
            $argstr += " -vcvars_ver=$VCToolsetVersion"
        }

        Write-Debug "Using VsDevCmd: [${vsdevcmd_bat.FullName}]"
        $env_script_content = @"
            @echo off
            call "$($vsdevcmd_bat.FullName)" $argstr
            set _rc=%ERRORLEVEL%
            set
            exit /b %_rc%
"@
    }

    # Write the script and then execute it, capturing its output
    Set-Content "$ScratchDir/.env.bat" $env_script_content

    Write-Debug "Loading VS environment with command: [$vsdevcmd_bat $argstr]..."
    $output = & cmd.exe /c "$ScratchDir/.env.bat"
    if ($LASTEXITCODE -ne 0) {
        throw "Loading the environment failed [$LASTEXITCODE]:`n$output"
    }

    return Split-CmdVarsOutput ($output -join "`r`n")
}
