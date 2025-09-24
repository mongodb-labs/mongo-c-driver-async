Import-Module $PSScriptRoot/util -Force

<#
.SYNOPSIS
    Configure and/or build a CMake project
.DESCRIPTION
    This command will configure, build, and optionally install a CMake project
#>
function Build-CMakeProject {
    [CmdletBinding(PositionalBinding = $false, DefaultParameterSetName = "configure-and-build")]
    param(
        # If set, only configure the project without building
        [Parameter(ParameterSetName = "configure")]
        [switch]
        $OnlyConfigure,
        # If set, only build without configuring
        [Parameter(ParameterSetName = "build")]
        [switch]
        $OnlyBuild,
        # The build directory for the project
        [Parameter(Mandatory)]
        [string]
        $BuildDir,
        # The source directory for the project
        [Parameter(ParameterSetName = "configure", Mandatory)]
        [Parameter(ParameterSetName = "configure-and-build", Mandatory)]
        [string]
        $SourceDir,
        # The CMake generator to use
        [Parameter(ParameterSetName = "configure")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [string]
        $Generator,
        # If set, then CMake will perform a fresh configure
        [Parameter(ParameterSetName = "configure")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [switch]
        $Fresh,
        # If set, the project will be installed to the given prefix
        [Parameter(ParameterSetName = "build")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [string]
        $InstallPath,
        # A mapping of CMake settings that will be used during configuration
        [Parameter(ParameterSetName = "configure")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [hashtable]
        $Settings = @{},
        # If set, clean before building
        [Parameter(ParameterSetName = "build")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [switch]
        $Clean,
        # Number of parallel jobs to run
        [Parameter(ParameterSetName = "build")]
        [Parameter(ParameterSetName = "configure-and-build")]
        [int]
        $Jobs,
        # The CMake executable to be used
        [string]
        $CMakeExecutable = "cmake"
    )
    $ErrorActionPreference = "stop"

    if ([string]::IsNullOrEmpty($BuildDir)) {
        $BuildDir = Join-Path $PWD "_build"
    }

    if (-not $OnlyBuild) {
        $command = @($CMakeExecutable)
        if (-not [string]::IsNullOrEmpty($SourceDir)) {
            $command += "-S$SourceDir"
        }
        $command += "-B$BuildDir"
        if (-not [string]::IsNullOrEmpty($Generator)) {
            $command += "-G$Generator"
        }
        if ($Fresh) {
            $command += "--fresh"
        }
        foreach ($key in $Settings.Keys) {
            $command += "-D$key=$(Format-CMakeValue $Settings[$key])"
        }
        Join-CommandLineArgv $command | Invoke-Expression
        if ($LASTEXITCODE) {
            throw "CMake configuration failed [$LASTEXITCODE]"
        }
    }

    if (-not $OnlyConfigure) {
        $command = @($CMakeExecutable)
        $command += "--build", $BuildDir
        if ($Clean) {
            $command += "--clean-first"
        }
        if ($Jobs) {
            $command += "--parallel", "$Jobs"
        }
        Write-Debug "Executing CMake command: $command"
        Join-CommandLineArgv $command | Invoke-Expression
        if ($LASTEXITCODE) {
            throw "CMake build failed [$LASTEXITCODE]"
        }
    }
}

function Test-CMakeProject {
    [CmdletBinding(PositionalBinding = $false)]
    param(
        # The directory containing the build results of the project
        [Parameter(Mandatory)]
        [string]
        $BuildDir,
        # Number of parallel jobs to run for the test
        [int]$Jobs,
        # The CTest configuration to run
        [string]$Configuration,
        # The CTest executable to use
        [string]$CTestExecutable = "ctest",
        # Stop immediately on the first test failure
        [switch]
        $StopOnFailure,
        # Print test output on failure
        [switch]
        $OutputOnFailure,
        # Use concise progress reporting
        [switch]
        $Progress,
        # Only execute tests matching the given regular expression
        [switch]
        $OnlyMatching,
        # Remaining arguments that are forwarded to CTest directly
        [Parameter(ValueFromRemainingArguments)]
        [string[]]
        $ArgumentList = @()
    )
    $ErrorActionPreference = "stop"

    $ctest_command = @($CTestExecutable)
    if (-not [string]::IsNullOrEmpty($Configuration)) {
        $ctest_command += "--build-config", $Configuration
    }
    if ($Verbose) {
        $ctest_command += "--verbose"
    }
    if ($StopOnFailure) {
        $ctest_command += "--stop-on-failure"
    }
    if ($Jobs) {
        $ctest_command += "--parallel", "$Jobs"
    }
    if ($OutputOnFailure) {
        $ctest_command += "--output-on-failure"
    }
    if ($Progress) {
        $ctest_command += "--progress"
    }
    if ($OnlyMatching) {
        $ctest_command += "--tests-regex", $OnlyMatching
    }
    $ctest_command += $ArgumentList

    try {
        [void](Push-Location $BuildDir)
        Join-CommandLineArgv $ctest_command | Invoke-Expression
        if ($LASTEXITCODE) {
            throw "CTest test execution failed [$LASTEXITCODE]"
        }
    }
    finally {
        [void](Pop-Location)
    }
}

function Format-CMakeValue($Value) {
    if ($Value -is [System.Boolean]) {
        if ($Value) { "TRUE" } else { "FALSE" }
    }
    elseif ($Value -is [array]) {
        return $Value | ForEach-Object { Format-CMakeValue $_ } | Join-String -Separator ';'
    }
    else {
        return "$Value"
    }
}
