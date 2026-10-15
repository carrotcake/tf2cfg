# In-game verification checklist

Everything the app cannot settle on its own: the argument bounds the ledger proposes, the
bind ids missing from the keyboard model, and the screen-reader pass. Filling this in clears
the user-gated queue in `docs/ROADMAP.md` (§2, §4) and `docs/argbounds-verification.md`.

Nothing here is applied to the app until it comes back checked - only in-game evidence moves
a row into `res/cfg/argbounds.csv` or an id into `src/ui/keytable.cpp`.

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
| 1 | `impulse` | `impulse 256`, then `impulse 300`; does 256 act like 0? | int 0..255, wraps mod 256 | |
| 2 | `mat_setvideomode` | `mat_setvideomode 1280 720 0`, then `... 1`, then `... 2` | windowed arg 0..1 | |
| 3 | `mat_enable_vrmode` | `mat_enable_vrmode 0`, `1`, `2` | 0..1 | |
| 4 | `grab_window` | `grab_window 1`, `grab_window 0` | 0..1 | |
| 5 | `fs_warning_level` | `fs_warning_level 0` .. `4`; which stick, which are rejected? | small enum, 0..3? | |
| 6 | `wait` | `wait 0` | `wait 0` legal (no-op)? | |
| 7 | `benchframe` | `benchframe 1`, `benchframe 0` | >= 1 | |
| 8 | `playvol` | `playvol <a sound path you know works> 0`, then `-1` | volume min 0 | |
| 9 | `bug` | `bug` bare first; then `bug 0`, `bug 1` | semantics unknown | |
| 10 | `Test_ProxyToggle_SetValue` | `Test_ProxyToggle_SetValue 0`, `1` | 0..1? | |
| 11 | `Test_SpawnRandomEntities` | bare `Test_SpawnRandomEntities`; then `0`, `-1` | counts >= 0? | |

### 1b. Needs the local server (`sv_cheats 1` + a map)

| # | Command | How to test | Proposed | Result |
|---|---|---|---|---|
| 12 | `sv_pure` | `sv_pure 0`, `1`, `2`, `-1`, `3` - check `sv_pure` after each | 0..2, is -1 a query state? | |
| 13 | `maxplayers` | `maxplayers 0`, `1`, `101`, `256` - check `maxplayers` after each | engine clamp (1..100? 1..255?) | |
| 14 | `tf_mvm_tank_health` | `tf_mvm_tank_health 0`, `1` | minimum 1? | |
| 15 | `cl_view` (also `cl_find_ent_index`, `cl_pred_track`) | `cl_view 0`, `2047`, `2048`, `-1` on a map with entities | 0..MAX_EDICTS-1 | |
| 16 | `cc_bot_selectweapon` | bare first, then each slot value it mentions; needs a bot on the server | 0..2? (primary/secondary/melee) | |
| 17 | `cc_spawn_merasmus_at_level` | bare first, then `0`, `1`, a large value | level range 1..N? | |
| 18 | `voicemenu` | `voicemenu 2 8` (arg 0 = 2 is the C menu in the zero-based z/x/c mapping); does item 8 exist on that menu, given its published list stops at 7? | menu 2 item 8 valid? | |
| 19 | `vote` | needs an active vote to mean anything - skip unless one is running | arg 0/1 ranges | |

### 1c. Needs a demo file (optional, low value)

| # | Command | How to test | Proposed | Result |
|---|---|---|---|---|
| 20 | `demo_gototick` | `playdemo <a demo you have>`, then `demo_gototick -5` | tick >= 0 assumed | |
| 21 | `demo_timescale` | same demo, then `demo_timescale 0`, `0.01`, `-1` | minimum (0 rejected? floor 0.01?) | |

## 2. Bind ids missing from the keyboard model

Fixtures and evidence: `docs/legacy/missing-input-ids.md`. Any id that checks out becomes a
UI-only change (the parser already handles quoted keys with spaces).

| # | Id | How to test | Result |
|---|---|---|---|
| 1 | `joy11` .. `joy32` | Without a controller: `bind joy11 +attack` - does the console report an unknown key name, or accept it silently? With a pad that has more than ten buttons: bind `joy11` .. `joy20` to different actions and press the extra buttons - which id fires on which button? | |
| 2 | `r axis pos` / `r axis neg` | `bind "r axis pos" "+attack"` (quotes matter), then twist the stick that has a roll/twist axis. Does it fire, and does the other direction need `r axis neg`? | |

Optional appendix - do **not** add these to the UI unless one checks out here. Same test
(unknown key name?) plus a press where the hardware exists: `break`, `capslocktoggle`,
`numlocktoggle`, `scrolllocktoggle`, and the Novint Falcon / Steam Controller codes listed
in the legacy doc.

## 3. Screen-reader pass (accessibility.md section 4)

Needs NVDA or Narrator (Windows) / Orca (Linux). The keyboard steps are already covered by
`tests/qml/`; what only a real reader can answer is what it *announces*.

| # | Step | What to check | Result |
|---|---|---|---|
| 1 | Start screen, press Tab repeatedly | Every control takes focus, the ring is visible, Enter/Space activates, and the reader names each control | |
| 2 | Load a config, then the editor tabs | Left/Right changes tab; the reader announces the tabs as tabs and says which is selected | |
| 3 | Keyboard panel: Tab to a cap, press Enter | The reader announces the cap as a button with its key label; the bind dialog opens | |
| 4 | In the bind dialog, Tab through it | Every combo, checkbox, radio, field and the Ok/Cancel pair takes focus with a ring and is announced | |
| 5 | A bound cap (orange) | The reader reads the bind as the description after the name | |

## After the run

Paste the filled tables back. Then the results land as: verified rows into
`res/cfg/argbounds.csv` (with a test pinning each), verified ids into `src/ui/keytable.cpp`
(+ parity test), the ledger and `docs/ROADMAP.md` updated, and anything that came back
`rejected` or `n/a` recorded as such rather than applied.
