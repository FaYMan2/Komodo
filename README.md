# Komodo C++ Prototype

This folder now contains a minimal CMake/PortAudio prototype that mirrors the desired structure. The Python proof-of-concept that lives under `basic/` remains untouched.

## Layout

```
komodo/
├── CMakeLists.txt
├── cmake/vcpkg.cmake            # optional helper if you want to preload the toolchain
├── src/
│   ├── main.cpp
│   ├── audio.cpp/.h
│   └── transcriber.cpp/.h
├── scripts/
│   ├── build.ps1
│   └── dev.ps1
└── README.md
```

## Prerequisites

1. Install [vcpkg](https://github.com/microsoft/vcpkg) outside of the repository, e.g.
   ```powershell
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   ```
2. Set the location once and reopen your shell:
   ```powershell
   setx VCPKG_ROOT "C:\\vcpkg"
   ```
   (Alternatively you can set `VCPKG_INSTALLATION_ROOT` instead of `VCPKG_ROOT`.)
3. Install PortAudio via vcpkg (manifest mode will come later):
   ```powershell
   $env:VCPKG_ROOT\vcpkg install portaudio
   ```
   If you set `VCPKG_INSTALLATION_ROOT` instead, use:
   ```powershell
   $env:VCPKG_INSTALLATION_ROOT\vcpkg install portaudio
   ```
   Or, if `vcpkg` is on your PATH:
   ```powershell
   vcpkg install portaudio
   ```

## Building

Use the scripted workflow so the vcpkg toolchain is supplied at configure time:

```powershell
# from the repo root
.\scripts\build.ps1 -Configuration Release
```

The script checks `VCPKG_ROOT`, configures CMake with the external toolchain and builds into `build/`.

If you previously configured `build/` without a toolchain (or with a different one) and see:

- `Manually-specified variables were not used by the project: CMAKE_TOOLCHAIN_FILE`

Clean the build directory and reconfigure:

```powershell
.\scripts\build.ps1 -Configuration Release -Clean
```

## Developer loop

```powershell
.\scripts\dev.ps1
```

`dev.ps1` calls the build script and then launches the resulting `komodo.exe` (preferring `build/Release` but also supporting single-config generators).

## Optional: preset toolchain file

If you prefer to bake the variable in during configuration, you can pass `-C cmake/vcpkg.cmake` to CMake. The helper verifies `VCPKG_ROOT` and sets `CMAKE_TOOLCHAIN_FILE` for the run.

---

- The repository no longer embeds `vcpkg/`.
- `build/` and `vcpkg/` are ignored via `.gitignore` so accidental folders do not get committed.
- `basic/` is left as-is for the Python experiment.
