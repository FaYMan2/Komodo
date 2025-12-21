param(
    [string]$Configuration = "Release"
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

& "$PSScriptRoot/build.ps1" -Configuration $Configuration
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$binaryCandidates = @(
    Join-Path $repoRoot "build/$Configuration/komodo.exe"
    Join-Path $repoRoot "build/komodo.exe"
)

foreach ($candidate in $binaryCandidates) {
    if (Test-Path $candidate) {
        & $candidate
        exit $LASTEXITCODE
    }
}

Write-Error "Unable to locate komodo executable in build output."
exit 1
