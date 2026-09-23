[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Leaf })]
    [string] $ArtifactZip,

    [Parameter(Mandatory = $true)]
    [string] $RuntimeDirectory
)

$ErrorActionPreference = 'Stop'

$artifactPath = (Resolve-Path -LiteralPath $ArtifactZip).Path
$runtimePath = [IO.Path]::GetFullPath($RuntimeDirectory)
$runtimeRoot = [IO.Path]::GetPathRoot($runtimePath)
if ([StringComparer]::OrdinalIgnoreCase.Equals($runtimePath.TrimEnd('\'), $runtimeRoot.TrimEnd('\'))) {
    throw 'The runtime directory must not be a drive root.'
}

$temporaryDirectory = Join-Path ([IO.Path]::GetTempPath()) ("mumain-artifact-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null

try {
    Expand-Archive -LiteralPath $artifactPath -DestinationPath $temporaryDirectory

    $mainExecutables = @(
        Get-ChildItem -LiteralPath $temporaryDirectory -Recurse -File -Filter 'Main.exe'
    )
    if ($mainExecutables.Count -ne 1) {
        throw "Expected one Main.exe in the artifact; found $($mainExecutables.Count)."
    }

    $artifactDirectory = $mainExecutables[0].Directory.FullName
    $networkLibrary = Join-Path $artifactDirectory 'MUnique.Client.Library.dll'
    if (-not (Test-Path -LiteralPath $networkLibrary -PathType Leaf)) {
        throw 'The artifact does not contain MUnique.Client.Library.dll beside Main.exe.'
    }

    New-Item -ItemType Directory -Force -Path $runtimePath | Out-Null
    $preserveConfig = Test-Path -LiteralPath (Join-Path $runtimePath 'config.ini') -PathType Leaf

    foreach ($item in Get-ChildItem -LiteralPath $artifactDirectory -Force) {
        if ($item.Name -in @('Data', 'fonts')) {
            continue
        }
        if ($preserveConfig -and $item.Name -eq 'config.ini') {
            continue
        }

        Copy-Item -LiteralPath $item.FullName -Destination $runtimePath -Recurse -Force
    }

    foreach ($dataDirectory in @('Data', 'fonts')) {
        $dataPath = Join-Path $runtimePath $dataDirectory
        if (-not (Test-Path -LiteralPath $dataPath -PathType Container)) {
            Write-Warning "Missing $dataDirectory in $runtimePath. Copy compatible game data before launching."
        }
    }

    Write-Host "Installed MuMain developer build to $runtimePath"
    Write-Host "Client executable: $(Join-Path $runtimePath 'Main.exe')"
    Write-Host 'Launcher host: 127.127.127.127'
    Write-Host 'Launcher port: 44406'
}
finally {
    if (Test-Path -LiteralPath $temporaryDirectory) {
        Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
    }
}
