param(
    [string]$Configuration = "Release"
    ,
    [switch]$Clean
    ,
    [switch]$WithWhisper = $true
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir = Join-Path $repoRoot "build"

if ($Clean -and (Test-Path $buildDir)) {
    Remove-Item -Recurse -Force $buildDir
}

function Resolve-VcpkgToolchain {
    $candidates = New-Object System.Collections.Generic.List[string]
    $checked = New-Object System.Collections.Generic.List[string]

    if ($env:VCPKG_ROOT) { $candidates.Add($env:VCPKG_ROOT) }
    if ($env:VCPKG_INSTALLATION_ROOT) { $candidates.Add($env:VCPKG_INSTALLATION_ROOT) }

    # If vcpkg is on PATH, infer root from its location.
    $cmd = Get-Command vcpkg -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Path) {
        $candidates.Add((Split-Path -Parent $cmd.Path))
    }

    # Common locations + a couple of repo-local conventions (if user cloned it nearby).
    $candidates.Add((Join-Path $repoRoot "vcpkg"))
    # Repo vendors vcpkg under dev/vcpkg in this project.
    $candidates.Add((Join-Path $repoRoot "dev/vcpkg"))
    $candidates.Add((Join-Path $repoRoot "external/vcpkg"))
    $candidates.Add("C:\vcpkg")
    $candidates.Add("C:\dev\vcpkg")
    $candidates.Add("C:\src\vcpkg")

    foreach ($root in $candidates) {
        if (-not $root) { continue }
        $checked.Add($root)
        $toolchain = Join-Path $root "scripts/buildsystems/vcpkg.cmake"
        if (Test-Path $toolchain) {
            return @{
                Toolchain = $toolchain
                CheckedRoots = $checked
            }
        }
    }

    return @{
        Toolchain = $null
        CheckedRoots = $checked
    }
}

function Normalize-ToolchainPath {
    param([string]$p)
    if (-not $p) { return $null }
    try {
        # Resolve-Path normalizes separators and casing where possible.
        return (Resolve-Path $p -ErrorAction Stop).Path
    } catch {
        # Fall back to a best-effort normalization.
        return ([System.IO.Path]::GetFullPath($p)) -replace '/', '\'
    }
}

$probe = Resolve-VcpkgToolchain
$toolchain = $probe.Toolchain
if (-not $toolchain) {
    $checkedList = if ($probe.CheckedRoots.Count -gt 0) { ($probe.CheckedRoots | Sort-Object -Unique) -join "`n  - " } else { "(none)" }
    $msg = @"
Could not find vcpkg toolchain file (scripts/buildsystems/vcpkg.cmake).

Checked:
  - $checkedList

Fix:
  - Install vcpkg (example):
      git clone https://github.com/microsoft/vcpkg C:\vcpkg
      C:\vcpkg\bootstrap-vcpkg.bat
  - Set VCPKG_ROOT (or VCPKG_INSTALLATION_ROOT) and open a new shell:
      setx VCPKG_ROOT "C:\vcpkg"

If you already have vcpkg installed, point VCPKG_ROOT at its folder (the one that contains scripts\buildsystems\).
"@
    Write-Error $msg
    exit 1
}

# If the binary is currently running, MSVC's linker can't overwrite it (LNK1104).
$running = Get-Process -Name "komodo" -ErrorAction SilentlyContinue
if ($running) {
    try { $running | Stop-Process -Force -ErrorAction SilentlyContinue } catch {}
}

# Optional: ensure prebuilt whisper binaries exist under libs/whisper (one-time-ish).
# We do this BEFORE configuring, because CMakeLists validates the prebuilt files at configure time.
if ($WithWhisper) {
    $whisperRoot = Join-Path $repoRoot "external/whisper.cpp"
    $whisperLib = Join-Path $repoRoot "libs/whisper/lib/$Configuration/whisper.lib"
    $whisperBin = Join-Path $repoRoot "libs/whisper/bin/$Configuration"
    $requiredDlls = @("whisper.dll", "ggml.dll", "ggml-cpu.dll", "ggml-base.dll")
    $missingDll = $requiredDlls | Where-Object { -not (Test-Path (Join-Path $whisperBin $_)) } | Select-Object -First 1

    if ((Test-Path $whisperRoot) -and ((-not (Test-Path $whisperLib)) -or $missingDll)) {
        if ($missingDll) {
            Write-Host "whisper.cpp runtime DLL missing from libs/whisper ($missingDll); building it now..."
        } else {
            Write-Host "whisper.cpp prebuilt not found in libs/whisper; building it now..."
        }
        & "$PSScriptRoot/build_whisper.ps1" -Configuration $Configuration
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

$useToolchainArg = $true
if (Test-Path $buildDir) {
    $cachePath = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cachePath) {
        $cacheLine = (Get-Content $cachePath -ErrorAction SilentlyContinue | Select-String -Pattern '^CMAKE_TOOLCHAIN_FILE:FILEPATH=' | Select-Object -First 1).Line
        if ($cacheLine) {
            $cachedToolchain = $cacheLine -replace '^CMAKE_TOOLCHAIN_FILE:FILEPATH=', ''
            $cachedNorm = Normalize-ToolchainPath $cachedToolchain
            $desiredNorm = Normalize-ToolchainPath $toolchain
            if ($cachedNorm -and $desiredNorm -and ($cachedNorm -ne $desiredNorm)) {
                Write-Warning "This build directory was configured with a different toolchain file:"
                Write-Warning "  cached: $cachedToolchain"
                Write-Warning "  desired: $toolchain"
                Write-Warning "CMake only applies toolchains on the first configure. Auto-cleaning build dir to apply the correct toolchain."
                if (-not $Clean) {
                    Remove-Item -Recurse -Force $buildDir
                }
            } elseif (-not $cachedToolchain) {
                Write-Warning "This build directory was configured without a toolchain file."
                Write-Warning "CMake only applies toolchains on the first configure. Auto-cleaning build dir to apply the vcpkg toolchain."
                if (-not $Clean) {
                    Remove-Item -Recurse -Force $buildDir
                }
            } else {
                # Toolchain already applied in this build dir; don't pass it again (avoids "not used" warnings).
                $useToolchainArg = $false
            }
        }
    }
}

$configureArgs = @(
    "-B", $buildDir,
    "-S", $repoRoot
)
if ($useToolchainArg) {
    $configureArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
}
$configureArgs += "-DKOMODO_WHISPER_CONFIG=$Configuration"

cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $buildDir --config $Configuration
