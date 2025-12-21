## Prebuilt whisper.cpp binaries

This project links to **prebuilt** `whisper.cpp` binaries from this folder.

### Expected layout

```
libs/whisper/
  bin/
    Release/whisper.dll
    Release/ggml.dll
    Release/ggml-cpu.dll
    Release/ggml-base.dll
    Debug/...
  lib/
    Release/whisper.lib
    Debug/...
```

### Build + populate

Run:

```powershell
.\scripts\build_whisper.ps1 -Configuration Release
```

Then build/run Komodo:

```powershell
.\scripts\dev.ps1 -Configuration Release
```


