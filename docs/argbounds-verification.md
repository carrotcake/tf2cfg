# argbounds verification ledger

Working list for `res/cfg/argbounds.csv` (design: `docs/DESIGN-arg-bounds.md`).
Rows enter the CSV only when a source states the bound; everything weaker sits here
until it is verified in-game. Sources used: the game's own command help text
(`data/cvarlist_linux.txt`, mirrored into the catalog descriptions), community
documentation (official TF2 wiki, long-standing community scripts), and the Source
SDK's data structures.

## Applied rows (2026-10-13)

| Command | Arg | Bounds | Source | Confidence |
|---|---|---|---|---|
| `load_itempreset` | 0 | int 0..3 | verified in-game by repo owner | highest |
| `coach_command` | 0 | int 0..3 | game help text enumerates 0:attack 1:defend 2:look 3:go | high |
| `eureka_teleport` | 0 | int 0..1 | game help text enumerates 0:home 1:teleporter exit | high |
| `next_map_vote` | 0 | int 0..2 | game help text `<0 \| 1 \| 2>` | high |
| `dbghist_addline` | 0 | int 0..4 | game help text enumerates categories 0..4 | high |
| `dbghist_dump` | 0 | int 0..4 | same list in the dump help | high |
| `demo_setendtick` | 0 | int min 0 | game help text "Set to 0 to disable" | high |
| `build` | 0 | int 0..3 | community scripts: 0 dispenser, 1 teleporter entrance, 2 sentry, 3 exit | medium |
| `destroy` | 0 | int 0..3 | same mapping as `build` | medium |
| `voicemenu` | 0 | int 0..2 | wiki/community: menus z/x/c = 0/1/2 | medium |
| `voicemenu` | 1 | int 0..8 | wiki/community: 9 items per menu (0..8, "Pass To Me!" = 8) | medium |
| `impulse` | 0 | int 0..255 | SDK: `CUserCmd::impulse` is a byte, so >255 wraps | medium |

## Needs in-game verification (proposed bounds, not yet applied)

Ordered by how likely a config author hits them.

- `sv_pure` `<int>`: Valve docs say 0/1/2; some sources mention -1 as a query state. Propose 0..2, confirm whether -1 is accepted.
- `voicemenu` item: is 8 valid on menu 2 (2 8)? Menu 2's published list stops at 7.
- `build`/`destroy` optional `<mode>` (arg 1): what values does the game accept (0 = normal, 1 = instant?).
- `impulse` >255: confirm the value wraps modulo 256 rather than being rejected.
- `mat_setvideomode` windowed (arg 2): 0/1, or does any nonzero act as windowed?
- `mat_enable_vrmode`: 0/1?
- `grab_window`: 0 = ungrab, 1 = grab?
- `fs_warning_level`: SDK `FileWarningLevel_t` suggests a small enum (0..3?); count the levels the console accepts.
- `maxplayers`: what does the engine clamp to (1..100? 1..255)?
- `wait`: is `wait 0` legal (no-op) or disabled?
- `demo_gototick`: tick ≥ 0 assumed, confirm negatives are rejected.
- `demo_timescale`: minimum (0 rejected? floor like 0.01?).
- `playvol` volume (arg 1): minimum 0?
- `tf_mvm_tank_health`: minimum 1?
- `cc_bot_selectweapon` slot (arg 1): 0..2 (primary/secondary/melee) or more?
- `cc_spawn_merasmus_at_level`: difficulty range (1..N?).
- `pda_click` arg 0/1: slot and index ranges.
- `vote` arg 0/1: vote index and option ranges (depends on the vote type).
- `bug` `<int>`: semantics of the argument at all.
- `Test_ProxyToggle_SetValue`: 0/1?
- `Test_SpawnRandomEntities` args: counts ≥ 0?
- `cl_view` / `cl_find_ent_index` / `cl_pred_track` entindex: 0..MAX_EDICTS-1 (engine-dependent)?
- `benchframe`: frame ≥ 1?

## Deliberately left unbounded

The engine accepts these values without a stable, knowable limit; do not bound them
without new evidence:

- `addcond` / `removecond` condition IDs - the list grows every patch.
- `banid` minutes, `kickid`, `namelockid` - player/session IDs, unbounded.
- `bot_hurt` damage, `ent_fire` value/delay, `ai_set_move_height_epsilon`,
  `air_density`, `gl_persistent_buffer_max_offset` - free numeric inputs.
- Coordinates and angles: `drawcross`, `drawline`, `bot_teleport`, `spec_goto`.
- `mm_select_session`, `mm_session_info` - session IDs.
