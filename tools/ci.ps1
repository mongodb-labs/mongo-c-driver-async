[CmdletBinding(PositionalBinding = $false)]
param(
    # The MSVS version to be loaded
    [Parameter(Mandatory)]
    [string]$VSVersion,
    # The target architecture
    [Parameter(Mandatory)]
    [string]$TargetArch,
    # The configurations to be built
    [string[]]$Configs = @("Debug"; "RelWithDebInfo"),
    [int]$Jobs,
    [switch]$Fresh,
    [switch]$Clean,
    [switch]$UseVcpkg,
    [switch]$BuildTesting,
    [switch]$WarningsAsErrors,
    [switch]$Test
)

$ErrorActionPreference = "Stop"

Import-Module $PSScriptRoot/vsutil -Force
Import-Module $PSScriptRoot/uv -Force
Import-Module $PSScriptRoot/cmake -Force

$this_dir = $PSScriptRoot

$root = Split-Path -Parent $this_dir

Write-Verbose "Loading uv environment..."
$uv_env = Get-UvEnvironment -ArgumentList "--group=build"
Write-Verbose "Loading MSVS environment..."
$vs_env = Invoke-WithEnvironment $uv_env {
    Get-VsEnvironment -Version:$VSVersion -TargetArch:$TargetArch
}
# Set the CC and CXX env vars to point to MSVC to prevent Ninja generation from
# attempting to use MinGW GCC instead, even if its available on the path
$vs_env["CC"] = "cl.exe"
$vs_env["CXX"] = "cl.exe"

Invoke-WithEnvironment $vs_env {
    $settings = @{
        AMONGOC_USE_PMM                  = $UseVcpkg;
        BUILD_TESTING                    = $Test;
        AMONGOC_COMPILE_WARNING_AS_ERROR = $WarningsAsErrors;
        CMAKE_CROSS_CONFIGS              = $Configs -join ';';
        CMAKE_DEFAULT_CONFIGS            = "all";
    }
    Build-CMakeProject -SourceDir $root -BuildDir $root/_build `
        -Settings $settings `
        -Generator "Ninja Multi-Config" `
        -Fresh:$Fresh -Clean:$Clean `
        -Jobs:$Jobs -Debug:$DebugPreference -Verbose:$VerbosePreference
    if ($Test) {
        Test-CMakeProject -BuildDir $root/_build -Configuration $Configs[0] -Progress -Jobs:$Jobs
    }
}
