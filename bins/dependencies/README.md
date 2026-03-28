# Pre-built Dependencies

Binaries needed at runtime that are not compiled from this repository.

---

## xash.dll (Modified Xash3D FWGS Engine)

**Modified build of the Xash3D FWGS engine core** with GoldSrc server browser support via Steam.

### What it does

The stock Xash3D FWGS engine can only browse Xash3D-native servers. This modified `xash.dll` adds support for querying **GoldSrc/Steam master servers**, allowing the in-game server browser to list Steam game servers filtered by AppID.

It works together with [Steam Broker](https://github.com/Deoroot/Steam-Broker-Xash3D-MSR):

1. The engine reads `xashcomm.lst` which points a `mastergs` entry to `127.0.0.1:27420` (the broker's address)
2. When the player opens the server browser, the engine sends a GoldSrc master server query to that address
3. Steam Broker receives the query, calls `ISteamMatchmakingServers` to get the real server list from Steam (filtered by the AppID in `steam_browse_appid.txt`)
4. The broker responds using the GoldSrc `M2A_SERVERSLIST` protocol format
5. The engine populates the server browser with the results

### Why it's needed

Without this modification, Xash3D players would not be able to see or connect to MSC/MSR servers running on GoldSrc (Half-Life via Steam). The standard Xash3D engine only knows about Xash3D master servers, not Valve/Steam master servers.

### Configuration

For the server browser to work, add this line to `xashcomm.lst` in the game directory:

```
mastergs 127.0.0.1:27420
```

And ensure [Steam Broker](https://github.com/Deoroot/Steam-Broker-Xash3D-MSR) is running with:
- `steam_appid.txt` containing `70` (Half-Life)
- `steam_browse_appid.txt` containing the AppID to browse (e.g. `1961680` for MS Rebirth, or `70` for all Half-Life servers)

### Where to place

Copy `xash.dll` to the **root directory** of your Xash3D installation, replacing the stock engine DLL.

### Source

Built from [Xash3D FWGS](https://github.com/nicholascioli/xash3d-fwgs) with the `cl_ticket_generator` / `cl_steam_broker_addr` support enabled. The engine's `cl_steam.c` implements the client-side broker protocol, and `masterlist.c` handles the `mastergs` server type.
