Import-Module $PSScriptRoot/util -Force

<#
.SYNOPSIS
    Obtain a `uv` executable from the internet automatically with a cache
.DESCRIPTION
    This cmdlet will download an executable `uv.exe` from the web, and save it
    locally on the system, and then return the absolute path to the downloaded
    executable file. The executable is stored in a cache directory to prevent
    redundant downloads.
#>
function Get-UvExecutable {
    [CmdletBinding()]
    param (
        # The version of uv that we will attempt to obtain
        [Parameter()]
        [string]
        $Version = "0.8.18",
        # The directory where the cached executables will be stored
        [Parameter()]
        [string]
        $CachePath,
        # Disable caching: Always download a new executable
        [Parameter()]
        [switch]
        $NoCache
    )
    $ErrorActionPreference = "stop"

    return Get-CachedRemoteFile "uv/$Version/uv.exe" -NoCache:$NoCache -CachePath:$CachePath -ObtainCommand {
        param($target)
        $dir = Split-Path -Parent $target
        $uv_zip = Get-CachedRemoteFile "uv/$Version.zip" -DownloadUri "https://github.com/astral-sh/uv/releases/download/$Version/uv-i686-pc-windows-msvc.zip"
        Expand-Archive $uv_zip -DestinationPath $dir -Force
    }
}

<#
.SYNOPSIS
    Get the environment variables associated with a uv virtual environment
#>
function Get-UvEnvironment {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        # Path to the uv executable to use. If not specified, one will be downloaded
        # automatically using `Get-UvExecutable`
        [Parameter()]
        [string]
        $UvExecutable,
        # Arguments to passs to `uv run` when creating the environment
        [Parameter(ValueFromRemainingArguments)]
        [string[]]
        $ArgumentList = @()
    )
    $ErrorActionPreference = "Stop"
    if ([string]::IsNullOrEmpty($UvExecutable)) {
        $UvExecutable = Get-UvExecutable
    }
    # Basic arguments:
    $uv_cmd = @($UvExecutable, "run")
    # Splice the arguments that we pass, like, `--group`, `--with`, etc.
    $uv_cmd += $ArgumentList
    # The command to be executed:
    $uv_cmd += @("--", "cmd.exe", "/c", "set")
    $uv_cmd = Join-CommandLineArgv $uv_cmd
    Write-Debug "Using uv command to obtain environment: $uv_cmd"
    $res = Invoke-Expression -Command $uv_cmd
    if ($LASTEXITCODE) {
        throw "uv subprocess failed [$LASTEXITCODE]"
    }
    return Split-CmdVarsOutput ($res -join "`r`n")
}


function Invoke-WithUvEnvironment {
    [CmdletBinding(PositionalBinding = $false)]
    param(
        # Path to the uv executable to be used. If not specified, one will be downloaded
        # automatically with `Get-UvExecutable`
        [Parameter()]
        [string]
        $UvExecutable,
        # The command to be executed
        [Parameter(Mandatory)]
        $Command,
        # Additional argument to pass to uv to set up the environment
        [Parameter(ValueFromRemainingArguments)]
        [string[]]
        $ArgumentList = @()
    )

    $env = Get-UvEnvironment -UvExecutable:$UvExecutable -ArgumentList:$ArgumentList
    Invoke-WithEnvironment -Environment:$env -Command:$Command -RemoveUnset
}
