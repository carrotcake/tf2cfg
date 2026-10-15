# In-game verification checklist

Everything the app cannot settle on its own: the argument bounds the ledger proposes, the
bind ids missing from the keyboard model, and the screen-reader pass. Filling this in clears
the user-gated queue in `docs/ROADMAP.md` (§2, §4) and `docs/argbounds-verification.md`.

**Run complete 2026-10-15** (repo owner; Microsoft Narrator for §3). The results below are
the record of that run; the outcomes were applied the same day to `res/cfg/argbounds.csv`,
`res/cfg/allcmds.csv`, `docs/argbounds-verification.md`, `docs/ROADMAP.md` and
`docs/accessibility.md`. The `sv_pure` console log is kept in `data/sv_pure.txt`.

Nothing gets applied to the app from a proposal alone - only in-game evidence moves a row
into `res/cfg/argbounds.csv` or an id into `src/ui/keytable.cpp`.

## How to run it

1. **Console**: Options -> Advanced -> Enable developer console, then `~` to open it.
2. **Local server** for the server-side rows: `sv_cheats 1`, then `map itemtest` (or
   Create Server from the main menu). Client rows work from any console.
3. **Cheapest trick first**: type a command with no arguments. The console usually prints
   its usage line (`Usage: sv_pure <0|1|2>`), which answers the question by itself. Copy
   that line instead of guessing.

## How to report

Fill in the Result column, or paste the rows back. Use these words:

| Word | Meaning |
|---|---|
| `0..2` | every value in that span is accepted and sticks (check with the bare command afterwards) |
| `clamped to N` | a far value silently became N |
| `rejected` | the console printed an error / usage line for that value |
| `wrap 256->0` | a value behaved like a different one |
| `no-op` | accepted but nothing happened |
| `n/a` | could not test, with the reason (no demo, no controller, not settable here) |

A rejection is a real answer and worth as much as an acceptance - write it down rather than
moving on.

## 1. Argument bounds (ledger: 21 proposals, none applied)

Order follows `docs/argbounds-verification.md` (most likely to be hit by a config author
first). "Proposed" is what the app would enforce for the editor only - the engine never
clamps command arguments.

### 1a. Client console, no server needed

| # | Command | How to test | Proposed | Result |
|---|---|---|---|---|
| 1 | `impulse` | `impulse 256`, then `impulse 300`; does 256 act like 0? | int 0..255, wraps mod 256 | all values silently accepted but most values a no-op. see [valve dev wiki](https://developer.valvesoftware.com/wiki/Impulse) |
| 2 | `mat_setvideomode` | `mat_setvideomode 1280 720 0`, then `... 1`, then `... 2` | windowed arg 0..1 | windowed arg 0..1 *(higher values wrap 2+ -> 1)* |
| 3 | `mat_enable_vrmode` | `mat_enable_vrmode 0`, `1`, `2` | 0..1 | 0..1 *(clamped, takes effect on restart)* |
| 4 | `grab_window` | `grab_window 1`, `grab_window 0` | 0..1 | **bare output:** Unknown command "grab_window" -> row removed from the catalog (pinned by `catalogRowsFixedFromTheInGameRun`) |
| 5 | `fs_warning_level` | `fs_warning_level 0` .. `4`; which stick, which are rejected? | small enum, 0..3? |**bare output:** "fs_warning_level n" where n is one of: 0:	FILESYSTEM_WARNING_QUIET 1:	FILESYSTEM_WARNING_REPORTUNCLOSED 2:	FILESYSTEM_WARNING_REPORTUSAGE 3:	FILESYSTEM_WARNING_REPORTALLACCESSES 4:	FILESYSTEM_WARNING_REPORTALLACCESSES_READ 5:	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE 6:	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC |
| 6 | `wait` | `wait 0` | `wait 0` legal (no-op)? | no-op *for all values*|
| 7 | `benchframe` | `benchframe 1`, `benchframe 0` | >= 1 | **bare output:** benchframe \<demoname\> \<frame\> \<tgafilename\>: takes a snapshot of a particular frame in a demo|
| 8 | `playvol` | `playvol <a sound path you know works> 0`, then `-1` | volume min 0 | wrap -1 -> 0|
| 9 | `bug` | `bug` bare first; then `bug 0`, `bug 1` | semantics unknown | arg ignored - 'bug' 'bug 0' 'bug 1' all open identical bug reporter dialog |
| 10 | `Test_ProxyToggle_SetValue` | `Test_ProxyToggle_SetValue 0`, `1` | 0..1? | no-op *from my perspective - no console output* |
| 11 | `Test_SpawnRandomEntities` | bare `Test_SpawnRandomEntities`; then `0`, `-1` | counts >= 0? | no-op *from my perspective - no console output* |

### 1b. Needs the local server (`sv_cheats 1` + a map)

| # | Command | How to test | Proposed | Result |
|---|---|---|---|---|
| 12 | `sv_pure` | `sv_pure 0`, `1`, `2`, `-1`, `3` - check `sv_pure` after each | 0..2, is -1 a query state? | -1..2 **w/ extensive usage info** -- *full console log in* data/sv_pure.txt |
| 13 | `maxplayers` | `maxplayers 0`, `1`, `101`, `256` - check `maxplayers` after each | engine clamp (1..100? 1..255?) | *while running server:* maxplayers -> "maxplayers" is "24", maxplayers 0 -> Cannot change maxplayers while the server is running. **Then set before the map loads:** 101 is accepted and is the ceiling; 0 is accepted but the server never starts -> applied as **1..101** |
| 14 | `tf_mvm_tank_health` | `tf_mvm_tank_health 0`, `1` | minimum 1? | Usage: tf_mvm_tank_health \<health to set all active tanks to\> - *silently accepts 0, no current val displayed*|
| 15 | `cl_view` (also `cl_find_ent_index`, `cl_pred_track`) | `cl_view 0`, `2047`, `2048`, `-1` on a map with entities | 0..MAX_EDICTS-1 | cl_view accepts any value silently; cl_find_ent_index 1 -> 'class C_TFPlayer' (entindex 1), cl_find_ent_index 2 -> Found no entity at 2. , etc.; *usage:* cl_pred_track \<entindex\> \<fieldname\>, cl_pred_track 1 1 -> No field '1' in datamap_t for entity 1/class C_TFPlayer |
| 16 | `cc_bot_selectweapon` | bare first, then each slot value it mentions; needs a bot on the server | 0..2? (primary/secondary/melee) | **0..2** (Usage: bot_selectweapon <bot name> <weapon slot>) - *primary-secondary-melee; higher values silently no-op. The "0..1" in this row's first draft was a typo* |
| 17 | `cc_spawn_merasmus_at_level` | bare first, then `0`, `1`, a large value | level range 1..N? | any value silently accepted |
| 18 | `voicemenu` | `voicemenu 2 8` (arg 0 = 2 is the C menu in the zero-based z/x/c mapping); does item 8 exist on that menu, given its published list stops at 7? | menu 2 item 8 valid? | voicemenu 2 8 -> no-op *(**incorrect assumption in our code:** voicemenu 0/1/2 with no second arg is also a no-op, UI voicemenus use voice_menu_\<N\> command)* |
| 19 | `vote` | needs an active vote to mean anything - skip unless one is running | arg 0/1 ranges | *range dependent on current vote - more info at* [alliedmods wiki](https://wiki.alliedmods.net/Tf2_voting#callvote) |

### 1c. Needs a demo file (optional, low value)

| # | Command | How to test | Proposed | Result |
|---|---|---|---|---|
| 20 | `demo_gototick` | `playdemo <a demo you have>`, then `demo_gototick -5` | tick >= 0 assumed | *for demo with 1000 ticks:* tick values < 1 silently no-op; tick values > 1000 skip to end of demo and exit to menu as if demo were fully played|
| 21 | `demo_timescale` | same demo, then `demo_timescale 0`, `0.01`, `-1` | minimum (0 rejected? floor 0.01?) | floor 0: *timescale 0 results in frozen playback as logically expected* |

## 2. Bind ids missing from the keyboard model

Fixtures and evidence: `docs/legacy/missing-input-ids.md`. Any id that checks out becomes a
UI-only change (the parser already handles quoted keys with spaces).

| # | Id | How to test | Result |
|---|---|---|---|
| 1 | `joy11` .. `joy32` | Without a controller: `bind joy11 +attack` - does the console report an unknown key name, or accept it silently? With a pad that has more than ten buttons: bind `joy11` .. `joy20` to different actions and press the extra buttons - which id fires on which button? | binds are silently accepted (the engine does not validate key names) but testing with real hardware didn't fire any commands -> **not surfaced in the keyboard model**; ROADMAP §4 closed |
| 2 | `r axis pos` / `r axis neg` | `bind "r axis pos" "+attack"` (quotes matter), then twist the stick that has a roll/twist axis. Does it fire, and does the other direction need `r axis neg`? | same as joy11 .. joy32; tested with a USB flightstick controller -> same decision |

Optional appendix - do **not** add these to the UI unless one checks out here. Same test
(unknown key name?) plus a press where the hardware exists: `break`, `capslocktoggle`,
`numlocktoggle`, `scrolllocktoggle`, and the Novint Falcon / Steam Controller codes listed
in the legacy doc.

## 3. Screen-reader pass (accessibility.md section 4)

Needs NVDA or Narrator (Windows) / Orca (Linux). The keyboard steps are already covered by
`tests/qml/`; what only a real reader can answer is what it *announces*.

| # | Step | What to check | Result |
|---|---|---|---|
| 1 | Start screen, press Tab repeatedly | Every control takes focus, the ring is visible, Enter/Space activates, and the reader names each control | OK |
| 2 | Load a config, then the editor tabs | Left/Right changes tab; the reader announces the tabs as tabs and says which is selected | **Round 1:** names never announced (the strip had no role). **Round 2** after `Accessible.PageTabList`: "Tab contains 3 items" - list recognised, names still not, because the strip itself held the focus. **Fix:** `Tf2TabBar` is a `FocusScope` whose current tab takes the focus (Qt's TabBar structure). **Round 3: confirmed** - the reader names each tab ("Command Overview, tab, 1 of 3, selected") |
| 3 | Keyboard panel: Tab to a cap, press Enter | The reader announces the cap as a button with its key label; the bind dialog opens | OK |
| 4 | In the bind dialog, Tab through it | Every combo, checkbox, radio, field and the Ok/Cancel pair takes focus with a ring and is announced | All but tabs OK *(same cause - the dialog has its own Tf2TabBar; fixed with it)* |
| 5 | A bound cap (orange) | The reader reads the bind as the description after the name | OK |

## What the run changed

Applied 2026-10-15 (details in `docs/argbounds-verification.md`):

- `res/cfg/argbounds.csv` gained eight rows: `sv_pure` (-1..2), `fs_warning_level` (0..6),
  `maxplayers` (1..101), `mat_setvideomode` arg 2 (0..1), `mat_enable_vrmode` (0..1),
  `cc_bot_selectweapon` arg 1 (0..2), `playvol` arg 1 (min 0), `demo_timescale` (min 0).
- `res/cfg/allcmds.csv`: `benchframe` signature corrected to `[str] [int] [str]`, `bug`
  reduced to `none`, `sv_pure` description replaced, `grab_window` row removed.
- `docs/ROADMAP.md` §4 closed (both id groups), §2 records the Narrator pass and the tab
  fix; §7 sizes the catalog-fidelity work the `grab_window` finding exposed.
- `docs/accessibility.md` §4 records the Narrator results: the tab strips needed two fixes
  (the `PageTabList` role, then focus moving to the current tab). Round 3 the same day
  confirmed the second one.
- Pinned by `EngineTest::argBoundsFromTheInGameRun` and
  `EngineTest::catalogRowsFixedFromTheInGameRun` (both fail if the data drifts) and by
  `test_tabBarAccessibleRoles` + `test_tabBarFocusFollowsTheCurrentTab`.

Left open: the catalog reconciliation in `docs/ROADMAP.md` section 7, which needs a fresh
in-game dump rather than the stale one in `data/`.

## 4. Next run (queue as of 2026-10-15)

**The dump.** From the in-game console, one line per command:

```
con_logfile cvarlist.txt
cvarlist
con_logfile ""
```

Then send `cvarlist.txt` (it lands next to the game's `cfg` folder). It replaces
`data/cvarlist_linux.txt`, which predates most of the catalog, so it is the only way to see
which descriptions and rows have drifted (`tools/catalog_diff.py`).

**Five cheat markers.** The catalog tags these cvars `cheat` and the old dump disagrees. A
cheat cvar refuses to change with `sv_cheats 0` and prints so, which settles each one in a
few seconds - on a listen server, with `sv_cheats 0` set first.

| # | cvar | How to test | Result |
|---|---|---|---|
| 1 | `r_shadows` | `sv_cheats 0`, then `r_shadows 1` - does the console refuse it? | |
| 2 | `snd_disable_mixer_duck` | same, `snd_disable_mixer_duck 1` | |
| 3 | `snd_mix_async` | same, `snd_mix_async 1` | |
| 4 | `snd_showclassname` | same, `snd_showclassname 1` | |
| 5 | `sv_memlimit` | `sv_memlimit 1000` | |

A refusal means the catalog's `cheat` marker is right and the old dump is wrong; a plain
change means the marker is wrong and the row loses the tag.
