<#
.SYNOPSIS
    Obtain a local copy of a file, with caching behavior
.DESCRIPTION
    This function obtains a file according to the parameters and stores a cached
    copy in a directory on the local system. If the file already exists, then the
    file will not be obtained and the filepath will be returned immediately.

    The -FileSuffix specifies the expected destination of the file within the
    caching directory. The file suffix may contain directory components. The
    file suffix should uniquely identify the resource that is being obtained.

    The -DownloadUri parameter will download the file directly. More complex
    logic can be performed with the -ObtainCommand, which will be invoked
    with a single argument specifying the goal filepath where the function
    expects the obtaining command to write the actual file.
#>
function Get-CachedRemoteFile {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        # The name of the file that is being downloaded
        [Parameter(Mandatory, Position = 1)]
        [string]
        $FileSuffix,
        # The URI of item to be downloaded
        [Parameter(Mandatory, ParameterSetName = "auto-download")]
        [string]
        $DownloadUri,
        # An invocable block that will be used to obtain the file
        [Parameter(Mandatory, Position = 2, ParameterSetName = "command-download")]
        [scriptblock]
        $ObtainCommand,
        # The directory that will hold cached objects
        [Parameter()]
        [string]
        $CachePath,
        # Disable caching. Try to obtain the file, event if it already exists
        [Parameter()]
        [switch]
        $NoCache
    )

    $ErrorActionPreference = "stop"

    if (-not $null -eq $DownloadUri) {
        $ObtainCommand = {
            # Take the destination as our sole parameter:
            param($target)
            # Pin the TLS version to ones that we know work across many hosts:
            [Net.ServicePointManager]::SecurityProtocol = 'tls12, tls11'
            # Suppress download process output:
            $ProgressPreference = "SilentlyContinue"
            Write-Debug "Downloading remote file [$DownloadUri] to [$target]"
            Invoke-WebRequest -UseBasicParsing -Uri $DownloadUri -OutFile $target
        }
    }

    # If they did not specify a caching directory, cache to a ~/AppData/Local
    if ([string]::IsNullOrEmpty($CachePath)) {
        $CachePath = "$env:LOCALAPPDATA/mongoc-cache"
    }

    # The actual local path to the file that we are looking for:
    $target = Join-Path $CachePath $FileSuffix
    # Ensure the parent directory exists before we proceed:
    $parent = Split-Path -parent $target
    [void](New-Item -ItemType Directory $parent -Force)

    if (-not (Test-Path $target) -or $NoCache) {
        Write-Debug "Need to obtain file $target"
        & $ObtainCommand $target
    }

    if (-not (Test-Path $target)) {
        throw "Obtaining the cached file did not result in the file being present. Check your -ObtainCommand"
    }

    return $target
}

<#
.SYNOPSIS
    Obtain the current environment variables for the process as a hashtable
    of strings.
#>
function Get-CurrentEnvironment {
    param()
    $env = [ordered]@{}
    foreach ($item in Get-ChildItem Env:) {
        $env[$item.Name] = $item.Value
    }
    return $env
}

<#
.SYNOPSIS
    Update the environment variables in the calling process with the contents of the given table
.DESCRIPTION
    This command will modify the environmnent variables of the current process to match the
    environment variables in the given hashtable.

    If `-RemoveUnset` is specified, then all variables in the current environment that are
    not present in the `-Environment` table will be removed from the process's environment.
.NOTES
    If you wish to run a single command/scriptblock with a modified environment, use
    `Invoke-WithEnvironment` instead.
#>
function Update-CurrentEnvironment {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        # A hash table of environment variables that will be set in the calling process.
        [Parameter(Position = 1)]
        [System.Collections.Hashtable]
        $Environment,
        # If set, the environment variables that are not present in the given environment
        # mapping will be removed from the process's environment
        [Parameter()]
        [switch]
        $RemoveUnset
    )
    $ErrorActionPreference = "Stop"
    foreach ($key in $Environment.Keys) {
        Write-Debug "Set environment variable `"$key`" => `"$($Environment[$key])`""
        [System.Environment]::SetEnvironmentVariable($key, $Environment[$key])
    }
    if ($RemoveUnset) {
        foreach ($item in (Get-CurrentEnvironment).Keys) {
            if (-not ($Environment.Contains($item))) {
                Write-Debug "Removing environment variable '$item'"
                Remove-Item env:$item
            }
        }
    }
}

<#
.SYNOPSIS
    Invoke a subcommand/code block with modified environment variables.
.DESCRIPTION
    The environment will be modified according to the -Environment parameter, which will
    specify the new environment variables to be used when invoking the command.
.NOTES
    This temporarily modifies the environment in the current process, and restores to
    environment when the subcommand returns.
#>
function Invoke-WithEnvironment {
    [CmdletBinding(PositionalBinding = $false)]
    param (
        [Parameter(Mandatory, Position = 1)]
        [System.Collections.Hashtable]
        $Environment,
        # The command to be executed
        [Parameter(Mandatory, Position = 2)]
        $Command,
        # If set, then environment variables not present in $Environment will be unset for
        # the invocation
        [Parameter()]
        [switch]
        $RemoveUnset
    )
    $ErrorActionPreference = "Stop"
    # Save the current environment to later restore it
    $prior_env = Get-CurrentEnvironment
    # Update the env and invoke the subcommand
    try {
        Update-CurrentEnvironment -Environment:$Environment -RemoveUnset:$RemoveUnset
        Invoke-Command $Command
    }
    finally {
        Update-CurrentEnvironment $prior_env -RemoveUnset
    }
}

<#
.SYNOPSIS
    Split a list of lines of key-value pairs, based on the output of the cmd.exe `set` command.
    Returns a mapping of the key/value pairs.
#>
function Split-CmdVarsOutput {
    [CmdletBinding()]
    param (
        # The lines of output from the cmd.exe "set" command
        [Parameter(Mandatory)]
        [string]
        $EnvironmentLines
    )
    $env = @{}
    foreach ($line in $EnvironmentLines.Split("`r`n")) {
        if ($line -match "(\w+)=(.+)") {
            $varname = $Matches[1]
            $value = $Matches[2]
            $env[$varname] = $value
        }
    }
    return $env
}

<#
.SYNOPSIS
    Enquote a string such that the MSVC CRT will reconstruct it correctly when
    passed as a command-line argument in a subprocess command string.

    This is imperfect, and misses an edge case related to leaning toothpicks
    that precede double-quotes
#>
function Format-QuotedString($arg) {
    if ($arg -match "^[\w=\.:@~\\/-]+$") {
        return $arg
    }
    else {
        $arg = $arg.replace('"', '\"')
        return "`"$arg`""
    }
}

function Join-CommandLineArgv($cmd) {
    $cmd | ForEach-Object { Format-QuotedString $_ } | Join-String -Separator ' '
}
