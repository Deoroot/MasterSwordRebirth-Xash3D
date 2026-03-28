# Master Sword Rebirth

The continuation of Master Sword Continued (MSC), a total Half-Life 1 conversion mod.
Based on [MSRevive/MasterSwordRebirth](https://github.com/MSRevive/MasterSwordRebirth).

This fork produces **client.dll** and **ms.dll** for both **GoldSrc** (Half-Life / ReHLDS) and **Xash3D FWGS**.

---

## Prerequisites

| Requirement | Notes |
|-------------|-------|
| [Visual Studio 2022](https://visualstudio.microsoft.com/vs/community/) (or Build Tools) | Desktop C++ workload, **x86 (Win32)** target |
| [CMake](https://cmake.org/download/) >= 3.24 | Bundled with VS 2022 works fine |
| Git | To clone this and the companion repos |

> The project targets **C++14** and compiles as **32-bit (Win32)** on Windows.

---

## Quick Build

### GoldSrc (Half-Life / ReHLDS)

```bat
cmake -S . -B ./build -A Win32 -G "Visual Studio 17 2022"
cmake --build ./build --config Release
```

Output: `bins/release/goldsrc/client.dll` and `bins/release/goldsrc/ms.dll`

### Xash3D FWGS

```bat
cmake -S . -B ./build_xash -A Win32 -G "Visual Studio 17 2022" -DXASH_BUILD=ON
cmake --build ./build_xash --config Release
```

Output: `bins/release/xash/client.dll` and `bins/release/xash/ms.dll`

### Linux (GoldSrc)

```bash
sudo dpkg --add-architecture i386
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y g++-11-multilib libgcc-s1:i386 libstdc++6:i386 libgl1-mesa-dev:i386
./build-linux.sh
```

### scriptpack utility

The `scriptpack` tool packs script files into `scripts.pak`. It lives under `utils/`:

```bat
cd utils
cmake -S . -B ./build -A Win32 -G "Visual Studio 17 2022"
cmake --build ./build --config Release --target scriptpack
```

---

## Building a Playable Game

To run MSC you need **three** repositories plus the engine. The workspace layout should look like:

```
MSC/                          <-- workspace root
  MasterSwordRebirth-MAR2026a/   (this repo)
  MSCScripts/                    (game scripts)
  assets/                        (maps, models, sounds, textures)
```

### 1. Clone companion repos

```bash
git clone https://github.com/MSRevive/MSCScripts
git clone https://github.com/MSRevive/assets
```

### 2. Build the DLLs

Follow the "Quick Build" section above for your target engine (GoldSrc or Xash3D).

### 3. Generate scripts.pak

Build `scriptpack` (see above), then from the `MSCScripts/` directory:

```bat
path\to\scriptpack.exe -f
```

This creates `MSCScripts/scripts.pak`.

### 4. Deploy

Use the included batch scripts from the workspace root:

| Script | Target engine | Deploy directory |
|--------|--------------|-----------------|
| `build_and_deploy.bat` | GoldSrc (Half-Life on Steam) | `Half-Life/msr/` |
| `build_and_deploy_xash.bat` | Xash3D FWGS (standalone) | Standalone bundle |

Both scripts handle the full pipeline: CMake configure, compile, scriptpack, and copy everything to the game directory.

> **Edit the paths** at the top of each `.bat` to match your machine (Half-Life install path, Xash3D engine path, etc.).

### What gets deployed

| File | Destination | Purpose |
|------|-------------|---------|
| `client.dll` | `msr/cl_dlls/` | Client-side mod DLL |
| `ms.dll` | `msr/dlls/` | Server-side mod DLL |
| `scripts.pak` | `msr/` | Packed game scripts |
| Assets (maps, models, etc.) | `msr/` | Game content from `assets/msr/` |

### Runtime dependencies

These DLLs must be in the engine root directory (next to `hl.exe` or `xash3d.exe`):

| DLL | Source |
|-----|--------|
| `fmod.dll` (32-bit) | [FMOD Studio API](https://www.fmod.com/download) -> `api/core/lib/x86/fmod.dll` |
| `discord-rpc.dll` | [discord-rpc v3.4.0](https://github.com/discord/discord-rpc/releases/tag/v3.4.0) |

### Xash3D FWGS additional components

The Xash3D build also compiles and deploys:

- **menu.dll** -- Main UI menu (from `xash3d-fwgs-sdk/3rdparty/mainui`)
- **msr.exe** -- Custom launcher with `XASH_GAMEDIR=msr` (from `xash3d-fwgs-sdk/game_launch`)
- **steam_broker.exe** -- Steam authentication bridge (optional, for connecting to GoldSrc servers)
- Engine binaries: `xash.dll`, `ref_gl.dll`, `ref_soft.dll`, `filesystem_stdio.dll`, `SDL2.dll`, `vgui.dll`, `vgui_support.dll`
- Engine resources: `extras.pk3`, `gfx.wad`, `fonts.wad`

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `XASH_BUILD` | `OFF` | Build for Xash3D FWGS instead of GoldSrc/ReHLDS |
| `INSTALL_DIR` | `""` | Directory to copy built binaries to |
| `VS_DEBUG_WORK_DIR` | `""` | Working directory for VS debugger (MSVC only) |
| `VS_DEBUG_CMD_ARGS` | `""` | Debug launch arguments (MSVC only) |

---

## Project Structure

```
src/game/client/   -- client.dll source
src/game/server/   -- ms.dll (server) source
utils/scriptpack/  -- scriptpack utility
bins/release/      -- compiled output (goldsrc/ and xash/ subdirs)
cmake/             -- CMake modules
thirdparty/        -- third-party dependencies
```

---

## Contributing

See open issues at [MSRevive/MasterSwordRebirth](https://github.com/MSRevive/MasterSwordRebirth/issues) or join the [Discord](https://discord.gg/nwJB9EhAN6).

## License

Half-Life 1 SDK License (Valve Corporation). See [LICENSE](LICENSE).
