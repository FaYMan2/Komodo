param(
    [string]$Configuration = "Release"
    ,
    [switch]$WithWhisper = $true
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

# If the binary is currently running, MSVC's linker can't overwrite it (LNK1104).
# Kill it so iterative `dev.ps1` runs work reliably.
$running = Get-Process -Name "komodo" -ErrorAction SilentlyContinue
if ($running) {
    try {
        $running | Stop-Process -Force -ErrorAction SilentlyContinue
    } catch {
        # Ignore failures; build will emit a clear LNK1104 if it can't overwrite the exe.
    }
}

& "$PSScriptRoot/build.ps1" -Configuration $Configuration -WithWhisper:$WithWhisper
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
