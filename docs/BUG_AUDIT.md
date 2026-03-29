# MSR Code Audit — Bug Report

**Date:** 2026-03-15
**Scope:** MSCScripts (2883 .script files), MasterSwordRebirth C++ source (client.dll / ms.dll)
**Methodology:** Manual review of ~60 scripts across all major categories + ~20 key C++ source files

---

## Table of Contents

1. [Scripts — Non-Functional Systems (Safe Fixes)](#1-scripts--non-functional-systems-safe-fixes)
2. [Scripts — Combat & Balance Bugs](#2-scripts--combat--balance-bugs)
3. [Scripts — Multiplayer Race Conditions & Exploits](#3-scripts--multiplayer-race-conditions--exploits)
4. [Scripts — Minor / Cosmetic](#4-scripts--minor--cosmetic)
5. [C++ — Crash Bugs](#5-c--crash-bugs)
6. [C++ — Security Vulnerabilities](#6-c--security-vulnerabilities)
7. [C++ — Buffer Overflows](#7-c--buffer-overflows)
8. [C++ — Design Issues](#8-c--design-issues)

---

## 1. Scripts — Non-Functional Systems (Safe Fixes)

These are bugs where an entire system is broken due to typos or wrong variable references. Fixing them **restores intended functionality** and does not change balance or cause multiplayer incompatibility. All are server-side NPC/menu interactions.

---

### S-01 — Banker NPC Menu Completely Broken

**File:** `scripts/NPCs/base_banker.script`
**Event:** `menuitem_checkacct_succes` (line 76)
**Severity:** CRITICAL

The callback event for existing-account detection is misspelled. The engine (verified in `src/game/server/monsters/npcscript.cpp` line 1429) calls `Callback + "_success"` (double `s`), meaning it looks for `menuitem_checkacct_success`. The script defines `menuitem_checkacct_succes` (single `s`).

**Impact:** Players who already have a bank account can never access their bank through the NPC menu. The "Bank" menu item is never registered.

**Fix:**
```
// Change:
{ menuitem_checkacct_succes
// To:
{ menuitem_checkacct_success
```

**MP Impact:** None — server-side menu registration only.

---

### S-02 — Fletcher "Bundle Bolts" Checks Wrong Array

**File:** `scripts/NPCs/base_fletcher.script`
**Event:** `check_bolts`
**Severity:** HIGH

The `check_bolts` event iterates `ARROW_STACK_TYPES` instead of `BOLT_STACK_TYPES`:

```
local CUR_ITEM $get_token(ARROW_STACK_TYPES,LOOP_COUNT)
```

**Impact:** The "Bundle Bolts" menu option appears when the player has *arrows*, not bolts. Actual bolt bundling either fails or produces wrong results.

**Fix:** Replace `ARROW_STACK_TYPES` with `BOLT_STACK_TYPES` in the `check_bolts` event.

**MP Impact:** None — server-side menu/inventory operation.

---

### S-03 — Guards Never Fight Back

**File:** `scripts/NPCs/guard.script`
**Event:** `npc_struck`
**Severity:** HIGH

When struck, the guard sets `CAN_STTACK` (double T typo) instead of `CAN_ATTACK`. The guard AI checks `CAN_ATTACK` to decide whether to retaliate, but since it's never set to `1`, guards remain passive.

**Fix:** Change `CAN_STTACK` to `CAN_ATTACK`.

**MP Impact:** None — NPC AI behavior, server-side only.

---

### S-04 — Magic Shop Scroll Buy-Back Never Works

**File:** `scripts/shops/base_magic.script`
**Event:** `bs_supplement`
**Severity:** HIGH

All scroll buy-back entries are gated behind:
```
if OVERCHARGE isnot 'OVERCHARGE'
```

But `OVERCHARGE` is never initialized in this script — only `B_OVERCHARGE` (const 200) exists. An uninitialized variable equals its own name string in MSC scripting, so the condition always evaluates to false.

**Impact:** Players cannot sell scrolls to any magic shop. Economy impact.

**Fix:** Change `OVERCHARGE` to `B_OVERCHARGE`, or add `setvard OVERCHARGE B_OVERCHARGE` before the checks.

**MP Impact:** None — shop menu, server-side.

---

### S-05 — Arrow Storage System Entirely Non-Functional

**File:** `scripts/NPCs/base_arrow_storage.script`
**Event:** `check_for_item`
**Severity:** HIGH

Two bugs in one event:
1. `$item_exists(PARAM1,SEARCH_ITEM)` uses `PARAM1` (the item name string) as the entity to search — should be `CUSTOMER_ID`.
2. `$item_exists(CUSTOMER_ID,SEARCH_NAME,name)` references `SEARCH_NAME` which doesn't exist — should be `SEARCH_ITEM`.

Additionally, `STORAGE_FEE` is set to `PARAM` (no number suffix) instead of `PARAM2`.

**Impact:** Arrow/bolt storage menu items never appear. The entire arrow storage NPC feature is broken.

**Fix:** Correct variable names: `PARAM1` → `CUSTOMER_ID`, `SEARCH_NAME` → `SEARCH_ITEM`, `PARAM` → `PARAM2`.

**MP Impact:** None — NPC menu, server-side.

---

### S-06 — Tutorial NPC Auto-Open Menu Typo

**File:** `scripts/NPCs/tutorial_prisoner.script`
**Event:** spawn block
**Severity:** MEDIUM

`menu.autopen 1` is misspelled — should be `menu.autoopen 1`. The auto-open flag is never set, so new players in the tutorial must figure out how to interact with Kyra manually.

**Fix:** Change `menu.autopen` to `menu.autoopen`.

**MP Impact:** None — client UX.

---

### S-07 — `base_scan_area.script` Is an Empty Stub

**File:** `scripts/items/base_scan_area.script`
**Event:** `bscan_start_scan`
**Severity:** MEDIUM

The entire scanning implementation is missing (commented-out WIP). Any spell or effect that `#include`s this file and relies on `BSCAN_EVENT` callbacks will silently do nothing.

**Impact:** Area-of-effect spells using this system have no target acquisition.

**Fix:** Implement the scanning logic, or mark dependent scripts as non-functional.

**MP Impact:** None — nothing happens either way currently.

---

### S-08 — `player_conartist.script` References Undefined Variables

**File:** `scripts/player/player_conartist.script`
**Event:** `[client] game_prerender`
**Severity:** MEDIUM

`SKEL_ID` and `SKEL_LIGHT_ID` are never set anywhere in this script — copy-paste artifact from a skeleton/undead effect script. Fires every client prerender frame, potentially creating stray light effects at world origin.

**Fix:** Remove or gate the undefined variable references.

**MP Impact:** None — client-side visual only.

---

## 2. Scripts — Combat & Balance Bugs

These bugs affect damage calculations, healing, or combat mechanics. Fixing them changes gameplay behavior. **Safe to fix on your own server**, but playing on a vanilla server with these fixes won't matter (combat scripts run server-side).

---

### S-09 — Charged Melee: Dead Code After `return` — Charge Never Resets

**File:** `scripts/items/base_melee.script`
**Event:** `melee_damaged_other` (line 239)
**Severity:** HIGH

The `return L_CHARGE_RATIO` on line 239 exits the event, making all subsequent code unreachable:

```
setdmg dmg NEW_DMG
return L_CHARGE_RATIO          ← exits here
dbg Adjusted dmg x L_CHARGE_RATIO   ← dead
setvard BWEAPON_CHARGE_PERCENT 0     ← dead — never resets!
local CUR_DRAIN MELEE_ENERGY         ← dead
multiply CUR_DRAIN BWEAPON_CHARGE_PERCENT  ← dead
drainstamina ent_owner CUR_DRAIN     ← dead
```

**Impact (three issues):**
1. `BWEAPON_CHARGE_PERCENT` is never reset to 0 — stale charge values persist across attacks.
2. Stamina drain for charged attacks never fires.
3. Even if reachable, `setvard BWEAPON_CHARGE_PERCENT 0` runs *before* `multiply CUR_DRAIN BWEAPON_CHARGE_PERCENT`, so drain would always be zero. The order is wrong.

**Fix:** Move `setvard BWEAPON_CHARGE_PERCENT 0` and `drainstamina` lines before the `return`. Fix the ordering so the drain calculation uses the charge value before resetting it.

**MP Impact:** Changes melee damage scaling and stamina economy for ALL melee weapons. Only affects the server you play on.

---

### S-10 — Vampire Potion Heals 2x Per Hit

**File:** `scripts/player/externals.script` (event `game_damaged_other`)
**Related:** `scripts/items/base_vampire.script`
**Severity:** HIGH

In the player's `game_damaged_other`, vampire potion code calls `givehp` unconditionally AND then calls `try_vampire_target` which does a second `givehp` on valid targets:

1. `givehp DMG_INFLICTED` — first heal (unconditional, fires on ALL targets including undead/immune)
2. `try_vampire_target` → `givehp PARAM1 PARAM3` — second heal (only on valid targets)

**Impact:** Players heal for 2x intended amount against valid targets, and still heal once against invalid targets (undead/immune) where they should heal 0. Compare with the Blood Drinker sword, which correctly only uses `try_vampire_target` (single heal).

**Fix:** Remove the unconditional `givehp` in `game_damaged_other`. Only heal through `try_vampire_target`.

**MP Impact:** Halves vampire potion healing effectiveness. Significant balance change. Server-side only.

---

### S-11 — `nub_loop` Permanently Reduces Weapon Damage After Level-Up

**File:** `scripts/items/base_melee.script`
**Event:** `nub_loop`
**Severity:** MEDIUM

When `NOOB_LOOP` is 1, `BITEM_UNDERSKILLED` is set to 1 unconditionally. If the player levels up their skill to meet `BASE_LEVEL_REQ` between loop cycles, the loop stops repeating — but `BITEM_UNDERSKILLED` is never set back to 0. The weapon stays at 5% damage (`setdmg dmg 0.05`) until manually re-equipped.

**Fix:** After the skill check, add an `else` branch: `setvard BITEM_UNDERSKILLED 0` and `setvard NOOB_LOOP 0`.

**MP Impact:** Fixes unintended damage penalty. Server-side, beneficial fix.

---

### S-12 — Two-Handed Sword Riposte: Entity May Not Exist

**File:** `scripts/items/swords_base_twohanded.script`
**Event:** `play_parry`
**Severity:** MEDIUM

`ent_laststruck` is used without checking `$get(ent_laststruck,isalive)` or `$get(ent_laststruck,exists)`. The parry event is called with a 0.1s delay (`callevent 0.1 play_parry`), and the attacker could die or be removed in that window. `xdodamage` on an invalid entity reference may silently fail or hit the wrong target.

**Fix:** Add `if $get(ent_laststruck,isalive)` before the riposte block.

**MP Impact:** Minor — prevents an edge case crash/misbehavior. Server-side.

---

### S-13 — Two-Handed Sword Over-Filters "magic" Damage Type for Parry

**File:** `scripts/items/swords_base_twohanded.script`
**Event:** `game_takedamage`
**Severity:** LOW

```
if ( PARAM4 contains 'effect' ) local EXIT_SUB 1
if ( PARAM4 contains 'target' ) local EXIT_SUB 1
if ( PARAM4 contains 'magic' ) local EXIT_SUB 1
```

The `contains 'magic'` check prevents parrying any damage type containing the substring "magic", even if it should be blockable. The `'effect'` check already catches `magic_effect`, making `'magic'` either redundant or overly broad.

**MP Impact:** Minor balance change if fixed. Server-side.

---

### S-14 — Spider Protection Potion Does Not Reduce Damage

**File:** `scripts/player/player_main.script`
**Event:** `game_damaged`
**Severity:** MEDIUM (if no engine-side handler exists)

The spider damage reduction code is entirely commented out:
```
//this doesn't wanna work here :\
//  if ( $get(ent_me,scriptvar,'PLR_SPIDER_PROT') )
//  { ... setdmg dmg OUT_DMG ... }
```

The spider protection potion sets `PLR_SPIDER_PROT` and `PLR_SPIDER_AMT` variables, but nothing reads them for actual damage reduction. The potion displays a message but has no mechanical effect.

**Fix:** Uncomment and fix the damage reduction logic, or implement it via a scriptflag check.

**MP Impact:** Would make spider protection potions actually functional. Balance change. Server-side.

---

### S-15 — Stamina Regen Effect May Never Expire

**File:** `scripts/effects/effect_stamina_regen.script`
**Event:** `game_activate`
**Severity:** MEDIUM

The script overrides `game_activate` from `base_effect` but only calls `callevent stamina_loop` — it never invokes the parent's activation logic that sets up `EFFECT_DURATION` and schedules `effect_duration_ended`. The stamina regen loop may continue indefinitely.

**Fix:** Call parent activation or manually schedule `effect_duration_ended`.

**MP Impact:** Infinite stamina regen is a balance issue. Server-side.

---

### S-16 — `effect_slow` May Become Permanent

**File:** `scripts/effects/effect_slow.script`
**Event:** `game_activate`
**Severity:** MEDIUM

Same pattern as S-15. The `[server]` scope tag on `game_activate` and `allowduplicate` keyword in the `#include` may prevent `base_effect`'s duration timer from being set up. A permanent slow debuff would persist until death.

**Fix:** Verify duration timer is set up correctly, or manually schedule expiration.

**MP Impact:** Debuff on players. Server-side.

---

### S-17 — Boss Restoration Comment/Value Mismatch

**File:** `scripts/monsters/base_monster_shared.script`
**Severity:** LOW

Comment says `[0.75]` but the constant is `0.5`:
```
//NPC_BOSS_RESTORATION  [0.75] ratio of health to restore when boss slays all players
const NPC_BOSS_RESTORATION 0.5
```

One or the other is wrong. Bosses either restore 50% or 75% health after a TPK — intent unclear.

**MP Impact:** If changed, affects boss difficulty. Server-side.

---

### S-18 — Volcano Sword `SECONDARY_DMG` Unused

**File:** `scripts/items/swords_volcano.script`
**Severity:** LOW

```
const SECONDARY_DMG 500 //not used in this script?
```

Dead code. The script's own comment acknowledges it. Charge attacks use a different formula.

**MP Impact:** None — dead code.

---

## 3. Scripts — Multiplayer Race Conditions & Exploits

These bugs only manifest with 2+ players and can cause duplication, lockouts, or incorrect state.

---

### S-19 — Chest Withdrawal Lock: Permanent Lockout After Disconnect

**File:** `scripts/chests/bank1/withdraw.script`
**Event:** `withdraw_items`
**Severity:** HIGH

The mutex uses `PLAYER_WITHDRAWING` to prevent concurrent access. If a player disconnects mid-withdrawal (crash, timeout), `PLAYER_WITHDRAWING` is never cleared. The chest becomes **permanently locked** for all players until the map restarts.

**Fix:** Add a timeout mechanism, or clear the lock when the locking player disconnects (e.g., via `game_playerleave` or a timed watchdog).

**MP Impact:** Denial of service fix. No balance change.

---

### S-20 — Gold Bag Double-Pickup

**File:** `scripts/chests/bag_o_gold_base.script`
**Event:** `game_playerused`
**Severity:** MEDIUM

Two players pressing E on the same gold bag in the same server frame can both enter the event before `WAS_USED` is set. Both receive the gold.

**Fix:** Set `WAS_USED` as the very first operation, before any other logic.

**MP Impact:** Fixes gold duplication exploit.

---

### S-21 — Crystal Keeper Shard Race Condition

**File:** `scripts/NPCs/crystal_keeper.script`
**Event:** `gave_shard` / `give_key`
**Severity:** MEDIUM

`SHARDS_RECIEVED` and `QUEST_WINNER` are NPC-level variables. Simultaneous shard submissions from two players can corrupt the winner assignment, giving the key to the wrong player or producing duplicate keys.

**Fix:** Use per-player tracking or immediately lock submissions after the count is reached.

**MP Impact:** Fixes quest-item duplication/misattribution.

---

### S-22 — Game Master Fade Uses Shared Variables

**File:** `scripts/game_master.script`
**Event:** `gm_fade` / `gm_fade_loop`
**Severity:** MEDIUM

Concurrent fade operations overwrite each other's `FD_LOOP_TARG` and `FD_LOOP_AMT`, leaving partially faded corpses/NPCs on screen.

**Fix:** Use unique per-target fade variables or a queue system.

**MP Impact:** Visual glitch fix only.

---

### S-23 — Game Master NPC Creation Slot Overwrite

**File:** `scripts/game_master.script`
**Event:** `gm_createnpc` through `gm_createnpc4`
**Severity:** MEDIUM

Only 4 delayed NPC creation slots (`CNPCA_*` through `CNPCD_*`). If a slot is reused before its delay fires (e.g., AoE kills a group), the wrong NPC spawns (wrong loot bags, wrong split creatures).

**Fix:** Use a dynamic queue instead of fixed slots.

**MP Impact:** Fixes incorrect monster/loot spawning under heavy load.

---

### S-24 — Storage Ticket System Can Lose Items

**File:** `scripts/NPCs/base_storage.script`
**Event:** `return_ticket` / `redeem_ticket`
**Severity:** HIGH

In both directions, the source item is removed *before* verifying the destination can receive it. If the player's inventory is full:
- `return_ticket`: Item removed, ticket offer fails → item permanently lost.
- `redeem_ticket`: Ticket consumed, item offer fails → ticket gone, no item received.

**Fix:** Check inventory space before removing the source item.

**MP Impact:** Prevents permanent item loss. No balance change.

---

## 4. Scripts — Minor / Cosmetic

---

### S-25 — `float_to_percent` Dead Code Branch

**File:** `scripts/player/externals.script`
**Event:** `float_to_percent`
**Severity:** LOW

```
if ( PARAM1 == 0 ) setvard FLOAT_RETURN '100%'
if ( PARAM1 == 1 ) setvard FLOAT_RETURN '100%'  ← dead code
if PARAM1 != 0
```

The `PARAM1 == 1` line sets `FLOAT_RETURN` to `'100%'`, but the subsequent `if PARAM1 != 0` block overwrites it. Output is accidentally correct but the code is misleading.

**MP Impact:** None.

---

### S-26 — Fire Aura Removal Called Twice on Death

**File:** `scripts/player/player_main.script`
**Event:** `game_death`
**Severity:** LOW

`callexternal ent_me ext_fire_aura_remove` appears twice in the same event. Poison aura removal appears once. The second fire aura call is redundant.

**MP Impact:** None — harmless redundancy.

---

### S-27 — Division by Zero Risk in Health/Mana Display

**File:** `scripts/player/player_main.script`
**Events:** `display_health`, `mana_drain`
**Severity:** LOW

```
divide PERCENT MY_MAXHP   // if MY_MAXHP == 0
divide PERCENT MY_MAXMP   // if MY_MAXMP == 0
```

Normal gameplay should never produce zero max HP/MP, but corrupted character data could trigger this.

**MP Impact:** None.

---

### S-28 — Storage NPC Rumor Speech Typo

**File:** `scripts/NPCs/storage.script`
**Event:** `heard_rurmor` (should be `heard_rumor`)
**Severity:** LOW

`catchspeech` routes "rumor" keywords to `heard_rumor`, but the event is defined as `heard_rurmor` (letters transposed). The NPC never responds to rumor inquiries.

**MP Impact:** None — dialogue only.

---

### S-29 — NPC `NPCATK_TARGET` Set to Sentinel Before Validation

**File:** `scripts/monsters/base_npc_attack.script`
**Event:** `hunting_mode_go`
**Severity:** LOW

`NPCATK_TARGET` is set to `HUNT_LASTTARGET` unconditionally, including when it's `−NONE−`. External scripts reading `NPCATK_TARGET` may misinterpret the sentinel value as a real entity.

**MP Impact:** None — NPC AI internal.

---

### S-30 — Fire Wall Trap Applies Burn to Dead Entities

**File:** `scripts/traps/fire_wall.script`
**Event:** `game_dodamage`
**Severity:** LOW

Applies `dot_fire` effect to dead players and doesn't distinguish allies from enemies.

**MP Impact:** None — effects on corpses are harmless.

---

### S-31 — Soup `ext_resetsoup` Doesn't Extend Duration

**File:** `scripts/effects/soup.script`
**Event:** `ext_resetsoup`
**Severity:** LOW

Resets regen rates and recalculates `FX_END_TIME`, but the actual expiration is governed by a scheduled `soup_duration_end` that is never re-issued. Duration is not actually extended despite the recalculation.

**MP Impact:** None — possible design intent.

---

### S-32 — Feldagor Accepts Any Gold Amount Via Direct Offer

**File:** `scripts/NPCs/feldagor.script`
**Event:** `recvoffer_gold`
**Severity:** LOW

Unconditionally accepts any gold offer (`recvoffer accept`), bypassing the intended `HIRE_PRICE` requirement if the player uses the old offer-gold mechanic.

**MP Impact:** Minor economy bypass.

---

## 5. C++ — Crash Bugs

---

### C-01 — Null Pointer Dereference in `CreateChar`

**File:** `src/game/server/sv_character.cpp` (line 85-87)
**Function:** `CBasePlayer::CreateChar`
**Severity:** CRITICAL

```cpp
CGenericItem *pStartingItem = NewGenericItem(CharData.Weapon);
if (FBitSet(pStartingItem->MSProperties(), ITEM_SPELL))  // CRASH if NULL
```

`NewGenericItem()` can return `NULL` if the weapon script is invalid. The very next line dereferences it without a null check. Compare with lines 59-61 in the same function, where the same pattern correctly uses `if (!pStartingItem) continue;`.

**Fix:**
```cpp
CGenericItem *pStartingItem = NewGenericItem(CharData.Weapon);
if (!pStartingItem) return;
if (FBitSet(pStartingItem->MSProperties(), ITEM_SPELL))
```

---

### C-02 — `exit(-1)` in Item Spawn Kills Entire Server

**File:** `src/game/shared/weapons/genericitem.cpp` (line 113)
**Function:** `CGenericItemMgr::GetGlobalGenericItemByName`
**Severity:** MEDIUM

On a double-exception during item spawn, the code calls `exit(-1)`, terminating the server process for all connected players. A single bad item definition in `scripts.pak` takes down the entire server.

**Fix:** Log the error, skip the item, and continue execution instead of terminating.

---

## 6. C++ — Security Vulnerabilities

These are defense-in-depth issues. In practice, exploitation requires injecting modified scripts into the server, which is difficult when using `scripts.pak`. However, they represent design-level flaws.

---

### C-03 — Path Traversal in `erasefile` Script Command

**File:** `src/game/shared/ms/scriptcmds.cpp` (line 3083-3110)
**Function:** `CScript::ScriptCmd_EraseFile`
**Severity:** HIGH

The filename parameter is concatenated to the game directory and passed to `std::remove()` with no path traversal validation. A script containing `erasefile "../../important_file"` could delete files outside the game directory.

```cpp
_snprintf(cFileName, sizeof(cFileName), "%s/%s", EngineFunc::GetGameDir(), fname.c_str());
std::remove(cFileName);
```

**Mitigating factor:** Only runs server-side (`#ifdef VALVE_DLL`), scripts are packed in `scripts.pak`.

**Fix:** Reject filenames containing `..` or absolute paths. Validate the resolved path is within the game directory.

---

### C-04 — Path Traversal in `writeline` Script Command

**File:** `src/game/shared/ms/scriptcmds.cpp` (line 7275-7325)
**Function:** `CScript::ScriptCmd_WriteLine`
**Severity:** HIGH

Same pattern as C-03, but for file writing. Additionally, the `#ifdef VALVE_DLL` guard was removed per a code comment, so `writeline` now runs on both client and server.

**Fix:** Re-restrict to server-only, validate filenames, limit to allowed directories.

---

### C-05 — Command Injection via `servercmd` Script Command

**File:** `src/game/shared/ms/scriptcmds.cpp` (line 5461-5482)
**Function:** `CScript::ScriptCmd_ServerCmd`
**Severity:** HIGH

Script parameters are concatenated verbatim and passed to `SERVER_COMMAND()` with no sanitization. Injection characters (`;`, `\n`) in parameters could chain arbitrary server console commands.

**Mitigating factor:** Scripts are packed; only exploitable with modified script files.

**Fix:** Whitelist allowed commands or sanitize against `;`, `\n`, `"`.

---

### C-06 — Command Injection via `clientcmd` Script Command

**File:** `src/game/shared/ms/scriptcmds.cpp` (line 2501-2548)
**Function:** `CScript::ScriptCmd_ClientCmd`
**Severity:** HIGH

Same pattern as C-05 for client commands. Additionally, the entity retrieved at `Params[0]` is dereferenced without a NULL check:

```cpp
CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity(Params[0]) : NULL;
if (pEntity->IsPlayer())  // CRASH if pEntity is NULL
```

**Fix:** Add `if (pEntity && pEntity->IsPlayer())` and sanitize command strings.

---

## 7. C++ — Buffer Overflows

Legacy C-style string handling throughout the codebase. Low practical risk since most data originates from trusted sources (pak files, engine), but corrupted save files or malformed data could trigger these.

---

### C-07 — `vsprintf` in `ErrorPrint`

**File:** `src/game/shared/ms/sharedutil.cpp` (line 254)
**Function:** `ErrorPrint`
**Severity:** HIGH

```cpp
static char string[1024];
vsprintf(string, szFmt, argptr);  // No bounds check
```

Any error message exceeding 1024 bytes overflows the stack buffer.

**Fix:** `vsnprintf(string, sizeof(string), szFmt, argptr)`.

---

### C-08 — `CMemFile::Write` Has No Overflow Check

**File:** `src/game/shared/ms/msfileio.cpp` (line 140-145)
**Function:** `CMemFile::Write`
**Severity:** HIGH

```cpp
void CMemFile::Write(void* pvData, size_t Size) {
    memcpy(&m_Buffer[m_WriteOffset], pvData, Size);
    m_WriteOffset += Size;
}
```

The character save buffer is allocated at 64KB (`1 << 16`). If character data exceeds this, a heap buffer overflow occurs.

**Fix:** Add bounds check: `if (m_WriteOffset + Size > m_BufferSize) { /* handle error */ return; }`.

---

### C-09 — Character Deserialization: Unbounded Loop Counts

**File:** `src/game/server/sv_character.cpp` (lines 148-390)
**Functions:** `ReadMaps1`, `ReadSkills1`, `ReadItems1`, `ReadStorageItems1`, etc.
**Severity:** HIGH

All `Read*` functions read a count from the save file and loop that many times without upper-bound validation:

```cpp
m_File.ReadInt(Maps);  // Could be 2 billion from corrupted file
for (int m = 0; m < Maps; m++) {
    m_File.ReadString(cTemp, MSSTRING_SIZE);
    m_VisitedMaps.add(cTemp);
}
```

**Impact:** A corrupted `.char` file causes OOM, excessive allocation, or reading past the buffer.

**Fix:** Validate counts against reasonable maximums (e.g., `MAX_SKILLS = 64`, `MAX_ITEMS = 256`, `MAX_MAPS = 1024`) and against remaining file buffer size.

---

### C-10 — `strcpy`/`strcat` Without Bounds in Entity Parser

**File:** `src/game/server/entities.cpp` (lines 104-127)
**Function:** `CEntity::FLoadEntity`
**Severity:** HIGH

```cpp
strcpy(szKey, com_token);          // no bounds check
strcpy(ent.classname, com_token);  // no bounds check
```

A malformed `.ent` file could overflow these stack buffers.

**Fix:** `strncpy(szKey, com_token, sizeof(szKey) - 1)`.

---

### C-11 — `strcat` Overflow in Player Pickup Menu

**File:** `src/game/server/player/player.cpp` (lines 5229-5302)
**Functions:** `PickupAnyItems`, `StealAnyItems`
**Severity:** MEDIUM

Multiple `strcat(cItemList, cTemp)` calls into a fixed-size buffer without tracking remaining space. Many items on the ground can cause overflow.

**Fix:** Use `_snprintf` or track remaining buffer capacity.

---

### C-12 — `sprintf` Overflow in Weight Check

**File:** `src/game/server/player/playershared.cpp` (line 565)
**Function:** `CBasePlayer::AddItem`
**Severity:** MEDIUM

```cpp
sprintf(cErrorString, "The %s is too big for you to carry.", pItem->DisplayName());
```

A long `DisplayName()` from script data overflows `cErrorString`.

**Fix:** `_snprintf(cErrorString, sizeof(cErrorString), ...)`.

---

### C-13 — `strcat` Overflow in NPC Speech

**File:** `src/game/server/monsters/msmonsterserver.cpp` (lines 1625-1648)
**Function:** `CMSMonster::Speak`
**Severity:** MEDIUM

Multiple `strcat(FinalSentence, ...)` calls with no bounds tracking. Long player/NPC names could overflow.

---

### C-14 — `strcpy` Overflow in Map Transition Strings

**File:** `src/game/shared/ms/scriptcmds.cpp` (lines 6523-6525)
**Function:** Script transition handling
**Severity:** MEDIUM

```cpp
strcpy(pPlayer->m_OldTransition, STRING(sName));
strcpy(pPlayer->m_NextMap, STRING(sDestMap));
strcpy(pPlayer->m_NextTransition, STRING(sDestTrans));
```

These member buffers are likely 32 bytes (matching `strncpy(..., 32)` in `SaveChar`). Script data exceeding that overflows.

**Fix:** `strncpy` with proper size.

---

### C-15 — Undersized Temp Buffer in `ReadItem1`

**File:** `src/game/server/sv_character.cpp` (line 391-400)
**Function:** `chardata_t::ReadItem1`
**Severity:** LOW

Uses `char cTemp[128]` for reading item names, but `MSSTRING_SIZE` is 256. A 128+ byte item name overflows.

**Fix:** Use `char cTemp[MSSTRING_SIZE]`.

---

## 8. C++ — Design Issues

---

### C-16 — Static Return Buffers Cause Use-After-Return

**File:** `src/game/shared/ms/mscharacter.cpp` (lines 43-77)
**Function:** `GetSaveFileName`
**Severity:** MEDIUM

Returns a pointer to `static char cFileName[MAX_PATH]`. Any subsequent call overwrites the previous result. Same pattern exists in `GetConst()` in `script.cpp` with `static msstring ReturnString`.

Currently safe under GoldSrc's single-threaded model, but fragile and bug-prone if code is refactored.

**Fix:** Accept an output buffer parameter, or return `std::string`.

---

### C-17 — `mslist::operator[]` Has No Bounds Checking

**File:** `src/game/shared/ms/stackstring.h` (lines 85-88)
**Severity:** MEDIUM

```cpp
itemtype_y &operator[](const int idx) const {
    return m_First[idx];  // no bounds check
}
```

Used pervasively (`m_Stats[i]`, `Gear[i]`, `m_Scripts[i]`). file-sourced indices from corrupted saves are not validated before use.

**Fix:** Add debug-mode bounds assertions. Validate file-sourced indices before use.

---

### C-18 — Global Static `cTemp` Shared Across Functions

**File:** `src/game/server/sv_character.cpp` (line 144)
**Severity:** LOW

`static char cTemp[MSSTRING_SIZE]` at file scope is shared by all `Read*` functions. Safe under current single-threaded execution, but a maintenance hazard.

---

### C-19 — Memory Leak on Exception Path in Item Spawning

**File:** `src/game/shared/weapons/genericitem.cpp` (lines 97-115)
**Function:** `CGenericItemMgr::GetGlobalGenericItemByName`
**Severity:** LOW

When `bTempPev = true` and the first `catch(...)` block fires, `GlobalItem.pItem` is replaced but the originally allocated `entvars_t` is leaked — cleanup at line 115 only runs if `bTempPev` is still true, but the catch block modifies that state.

---

## Summary

| Category | Critical | High | Medium | Low | Total |
|----------|----------|------|--------|-----|-------|
| Scripts — Broken Systems | 1 | 4 | 3 | 0 | **8** |
| Scripts — Combat/Balance | 0 | 2 | 5 | 1 | **8** |
| Scripts — MP Race Conditions | 0 | 2 | 3 | 0 | **5** |
| Scripts — Minor/Cosmetic | 0 | 0 | 0 | 8 | **8** |
| C++ — Crashes | 1 | 0 | 1 | 0 | **2** |
| C++ — Security | 0 | 4 | 0 | 0 | **4** |
| C++ — Buffer Overflows | 0 | 4 | 4 | 1 | **9** |
| C++ — Design | 0 | 0 | 2 | 3 | **5** |
| **Total** | **2** | **16** | **18** | **13** | **49** |

### Priority Fix Order (recommended)

1. **S-01** (Banker typo) — one character fix, restores entire banking system
2. **C-01** (CreateChar null deref) — one line fix, prevents server crash
3. **S-02 through S-05** (Fletcher, Guard, Magic Shop, Arrow Storage) — typo/variable fixes, restore broken features
4. **S-19** (Chest lockout) — prevents permanent lockout in multiplayer
5. **S-24** (Storage ticket item loss) — prevents permanent item loss
6. **S-09** (Melee charge dead code) — move lines before return, fixes charge mechanics
7. **C-07** (vsprintf → vsnprintf) — one-word fix, prevents buffer overflow
8. **C-08, C-09** (Save/load overflow protection) — prevents corrupt-save crashes
