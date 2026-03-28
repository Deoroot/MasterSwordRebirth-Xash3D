# Master Sword Rebirth

The continuation of Master Sword Continued (MSC), a total Half-Life 1 conversion mod.
Based on [MSRevive/MasterSwordRebirth](https://github.com/MSRevive/MasterSwordRebirth).

This fork produces **client.dll** and **ms.dll** for both **GoldSrc** (Half-Life / ReHLDS) and **Xash3D FWGS**.

> **IMPORTANT:** Building a playable game requires **two additional repositories** besides this one:
> - [MSCScripts](https://github.com/MSRevive/MSCScripts) -- game scripts (needed to generate `scripts.pak`)
> - [assets](https://github.com/MSRevive/assets) -- maps, models, sounds, textures
>
> Without these, the DLLs will compile but the game will not run. See [Building a Playable Game](#building-a-playable-game) below.

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

### Xash3D FWGS engine binaries

The mod DLLs alone are not enough -- you need the **Xash3D FWGS engine** to run them. There are two ways to obtain it:

#### Option A: Download a pre-built release (recommended)

Download the latest **win32** release from [FWGS/xash3d-fwgs](https://github.com/FWGS/xash3d-fwgs/releases). Extract it and you'll get the base engine. Then copy the mod files into a `msr/` subdirectory.

#### Option B: Use the standalone bundle from `build_and_deploy_xash.bat`

The deploy script builds everything from source and assembles a standalone bundle. It requires a local Xash3D FWGS engine build (set `ENGINE_SRC` in the bat) to copy the engine binaries from.

#### Standalone bundle structure

Either way, the final standalone directory should look like this:

```
MSR/                              <-- engine root
  msr.exe                          Launcher (XASH_GAMEDIR=msr)
  xash.dll                         Engine core (modified version in bins/dependencies/ for server browser)
  ref_gl.dll                       OpenGL renderer
  ref_soft.dll                     Software renderer
  filesystem_stdio.dll             Filesystem module
  SDL2.dll                         SDL2 runtime
  menu.dll                         Main UI menu
  vgui.dll                         VGUI library
  vgui_support.dll                 VGUI support module
  fmod.dll                         FMOD audio (32-bit)
  discord-rpc.dll                  Discord Rich Presence
  steam_broker.exe                 Steam auth bridge (optional)
  steam_api.dll                    Valve's Steam SDK DLL, 32-bit (optional, for Steam auth)
  steam_appid.txt                  Contains "70" (optional, for Steam auth)
  steam_browse_appid.txt           Contains AppID for server browser, e.g. "1961680" (optional)
  xashcomm.lst                     Master server config: "mastergs 127.0.0.1:27420" (optional)
  Launch Master Sword Rebirth.bat  "start msr.exe -console"
  msr/                             <-- game directory
    gameinfo.txt                   Xash3D game configuration (basedir "msr", standalone)
    cl_dlls/
      client.dll                   Client mod DLL (Xash3D build)
    dlls/
      ms.dll                       Server mod DLL (Xash3D build)
    scripts.pak                    Packed game scripts
    extras.pk3                     Engine fonts/UI resources
    gfx.wad                        Engine graphics
    fonts.wad                      Engine fonts
    game.ico                       Game icon
    ... (maps, models, sounds, textures from assets repo)
```

#### Where each file comes from

| File | Source |
|------|--------|
| `msr.exe` | Compiled from `xash3d-fwgs-sdk/game_launch` with `-DXASH_GAMEDIR=msr` |
| `xash.dll` | Pre-built in `bins/dependencies/` (modified for Steam server browser) or from Xash3D FWGS release |
| `ref_gl.dll`, `ref_soft.dll`, `filesystem_stdio.dll`, `SDL2.dll` | [Xash3D FWGS release](https://github.com/FWGS/xash3d-fwgs/releases) or local engine build |
| `menu.dll` | Compiled from `xash3d-fwgs-sdk/3rdparty/mainui` |
| `vgui.dll`, `vgui_support.dll` | [Xash3D FWGS release](https://github.com/FWGS/xash3d-fwgs/releases) or local engine build |
| `fmod.dll` (32-bit) | [FMOD Studio API](https://www.fmod.com/download) -> `api/core/lib/x86/fmod.dll` |
| `discord-rpc.dll` | [discord-rpc v3.4.0](https://github.com/discord/discord-rpc/releases/tag/v3.4.0) |
| `steam_broker.exe` | [Steam-Broker-Xash3D-MSR](https://github.com/Deoroot/Steam-Broker-Xash3D-MSR) |
| `steam_api.dll` | Valve's real 32-bit DLL (from Steam SDK or Half-Life install) |
| `extras.pk3`, `gfx.wad`, `fonts.wad` | [Xash3D FWGS release](https://github.com/FWGS/xash3d-fwgs/releases) `valve/` directory or local engine build |
| `client.dll`, `ms.dll` | Compiled from this repo with `-DXASH_BUILD=ON` |
| `scripts.pak` | Generated by `scriptpack` from [MSCScripts](https://github.com/MSRevive/MSCScripts) |
| Maps, models, sounds, textures | [assets](https://github.com/MSRevive/assets) repo (`msr/` subdirectory) |
| `gameinfo.txt`, `game.ico` | Included in [assets](https://github.com/MSRevive/assets) repo |

> **Note:** The Steam-related files (`steam_broker.exe`, `steam_api.dll`, `steam_appid.txt`, `steam_browse_appid.txt`, `xashcomm.lst`) are only needed if you want Steam authentication and the Steam server browser. The game runs without them, but you won't be able to connect to Steam-protected servers or browse servers by AppID.

---

## Client-Server Connection and Steam AppID

For a Xash3D client to successfully connect to a GoldSrc/ReHLDS server with Steam authentication, both sides must agree on the same **Steam AppID**. Here's how each component uses it:

### Server side (MSRevive's modified ReHLDS)

> **Note:** This section refers to the [MSRevive fork of ReHLDS](https://github.com/MSRevive/MSR-ReHLDS), **not** the vanilla [dreamstalker/rehlds](https://github.com/dreamstalker/rehlds).

The MSRevive-modified ReHLDS engine has the AppID **hardcoded to `1961680`** (MS Rebirth) in `sv_main.cpp`:

```cpp
#define MSS_APPID 1961680
int GetGameAppID(void) { return MSS_APPID; }
```

The original (vanilla) ReHLDS implementation looks up the AppID dynamically from a table (`g_GameToAppIDMap`) based on the active game directory (`com_gamedir`). The MSRevive fork replaced that logic with a hardcoded return value. This means:

- The server **always** registers with Steam as AppID 1961680, regardless of game directory
- **Changing the AppID requires recompiling** the engine (`sv_main.cpp`, lines ~8577-8581)
- On startup, the dedicated server writes this value to `steam_appid.txt` and registers with Steam master servers under AppID 1961680

#### How to change the AppID and recompile MSRevive's ReHLDS

**Prerequisites:** Visual Studio 2022 (or Build Tools) with C++ Desktop workload, Win32 (x86) target.

**Step 1 -- Modify the AppID** in `src/engine/sv_main.cpp`:

```cpp
// ~line 8574: update the table entry
{ 0x1DEED0, "msrebirth" }    // 0x1DEED0 = 1961680. Change to desired AppID/gamedir

// ~line 8577-8581: update the hardcoded return
#define MSS_APPID 1961680    // <-- change this value to the desired AppID
int GetGameAppID(void)
{
    return MSS_APPID;
}
```

**Step 2 -- Recompile** using MSBuild (the CMakeLists.txt does not support Windows):

```bat
msbuild ReHLDS.sln /p:Configuration=Release /p:Platform=Win32
```

Or with full path to MSBuild if not in a Developer Command Prompt:

```bat
"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" ReHLDS.sln /p:Configuration=Release /p:Platform=Win32
```

**Step 3 -- Deploy** the compiled binaries from `bins/Release/`:

| File | Description |
|------|-------------|
| `swds.dll` | Server engine DLL (the main output) |
| `hlds.exe` | Dedicated server launcher |
| `filesystem_stdio.dll` | Filesystem module |

### Server side (vanilla ReHLDS)

The vanilla [dreamstalker/rehlds](https://github.com/dreamstalker/rehlds) **does support different AppIDs** per game directory. Unlike the MSRevive fork, its `GetGameAppID()` dynamically looks up the AppID from a table based on the active game directory (`com_gamedir`):

```cpp
GameToAppIDMapItem_t g_GameToAppIDMap[] = {
    { 10,  "cstrike" },
    { 20,  "tfc" },
    { 30,  "dod" },
    { 40,  "dmc" },
    { 50,  "gearbox" },
    { 60,  "ricochet" },
    { 70,  "valve" },
    { 80,  "czero" },
    { 100, "czeror" },
    { 130, "bshift" },
    { 150, "cstrike_beta" },
};

int GetGameAppID(void)
{
    // iterates g_GameToAppIDMap comparing pGameDir with com_gamedir
    // returns matching AppID, or 70 (Half-Life) as fallback
}
```

To add MSR support to vanilla ReHLDS, modify **one file** (`rehlds/engine/sv_main.cpp`):

1. Add an entry to `g_GameToAppIDMap`: `{ 1961680, "msr" }` (or your game directory name)
2. Increase the array size accordingly
3. Recompile with the same MSBuild command above

The vanilla implementation will then automatically use AppID 1961680 when the server runs with `-game msr`. No other files need modification.

### Client side (Xash3D + Steam Broker)

The client needs two things for Steam authentication:

| File | Value | Purpose |
|------|-------|---------|
| `steam_appid.txt` | `70` | Initializes Steam SDK (must be Half-Life's AppID so Steam recognizes the game) |
| `steam_browse_appid.txt` | `1961680` | Tells the broker which AppID to query for the server browser |

The [Steam Broker](https://github.com/Deoroot/Steam-Broker-Xash3D-MSR) handles the translation: it initializes with AppID 70 (Half-Life, required by Steam) but generates auth tickets that are valid for connecting to servers running under AppID 1961680.

### How connection works

```
Xash3D client                    Steam Broker (local)              ReHLDS server
     |                                |                                  |
     |-- sb_connect ip:port --------->|                                  |
     |                                |-- InitiateGameConnection() -->   |
     |                                |   (Steam SDK, AppID 70)          |
     |<-- auth ticket ----------------|                                  |
     |                                                                   |
     |-- connect + ticket ------------------------------------------>    |
     |                                                   validates ticket|
     |<-- connection accepted -------------------------------------------|
```

### Server browser

For the server browser to list MSR servers, the client needs:
1. A modified `xash.dll` (included in `bins/dependencies/`) with GoldSrc master server support
2. `xashcomm.lst` with `mastergs 127.0.0.1:27420`
3. Steam Broker running with `steam_browse_appid.txt` set to `1961680`

### GoldSrc client (standard Half-Life)

GoldSrc clients connecting through Steam (hl.exe) use AppID 70 natively. The ReHLDS server accepts these connections because Steam's authentication system handles the cross-AppID validation. No broker is needed for GoldSrc clients.

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
