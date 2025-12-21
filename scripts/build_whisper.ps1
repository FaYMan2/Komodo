param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$whisperRoot = Join-Path $repoRoot "external/whisper.cpp"
if (-not (Test-Path $whisperRoot)) {
    Write-Error "external/whisper.cpp not found at: $whisperRoot"
    exit 1
}

$whisperBuild = Join-Path $whisperRoot "build"
$outRoot = Join-Path $repoRoot "libs/whisper"
$outBin = Join-Path $outRoot "bin/$Configuration"
$outLib = Join-Path $outRoot "lib/$Configuration"

New-Item -ItemType Directory -Force -Path $outBin | Out-Null
New-Item -ItemType Directory -Force -Path $outLib | Out-Null

cmake -S $whisperRoot -B $whisperBuild
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $whisperBuild --config $Configuration
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$builtBin = Join-Path $whisperBuild "bin/$Configuration"

$dlls = @("whisper.dll", "ggml.dll", "ggml-cpu.dll", "ggml-base.dll")
foreach ($dll in $dlls) {
    $src = Join-Path $builtBin $dll
    if (-not (Test-Path $src)) {
        Write-Error "Expected DLL not found: $src"
        exit 1
    }
    Copy-Item -Force $src $outBin
}

# Import library (MSVC): location can vary (often build/src/<config>/whisper.lib).
$implibCandidates = @(
    (Join-Path $builtBin "whisper.lib"),
    (Join-Path $whisperBuild "src/$Configuration/whisper.lib"),
    (Join-Path $whisperBuild "src/Release/whisper.lib"),
    (Join-Path $whisperBuild "src/Debug/whisper.lib")
)

$implib = $implibCandidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if (-not $implib) {
    Write-Error "Expected import library not found. Checked:`n  - $($implibCandidates -join "`n  - ")"
    exit 1
}
Copy-Item -Force $implib $outLib

# Copy PDBs if present (useful for debugging).
Get-ChildItem -Path $builtBin -Filter "*.pdb" -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Force $_.FullName $outBin
}

Write-Host "whisper.cpp built and copied to:"
Write-Host "  $outBin"
Write-Host "  $outLib"


