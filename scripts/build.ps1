param(
    [string]$Configuration = "Release"
    ,
    [switch]$Clean
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

if (Test-Path $buildDir) {
    $cachePath = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cachePath) {
        $cacheLine = (Get-Content $cachePath -ErrorAction SilentlyContinue | Select-String -Pattern '^CMAKE_TOOLCHAIN_FILE:FILEPATH=' | Select-Object -First 1).Line
        if ($cacheLine) {
            $cachedToolchain = $cacheLine -replace '^CMAKE_TOOLCHAIN_FILE:FILEPATH=', ''
            if ($cachedToolchain -and ($cachedToolchain -ne $toolchain)) {
                Write-Warning "This build directory was configured with a different toolchain file:"
                Write-Warning "  cached: $cachedToolchain"
                Write-Warning "  desired: $toolchain"
                Write-Warning "CMake only applies toolchains on the first configure. Reconfigure with a clean build dir:"
                Write-Warning "  .\scripts\build.ps1 -Configuration $Configuration -Clean"
            } elseif (-not $cachedToolchain) {
                Write-Warning "This build directory was configured without a toolchain file."
                Write-Warning "CMake only applies toolchains on the first configure; if you see 'CMAKE_TOOLCHAIN_FILE was not used', clean and reconfigure:"
                Write-Warning "  .\scripts\build.ps1 -Configuration $Configuration -Clean"
            }
        }
    }
}

$configureArgs = @(
    "-B", $buildDir,
    "-S", $repoRoot,
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
)

cmake @configureArgs
cmake --build $buildDir --config $Configuration
