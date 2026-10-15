# argbounds verification ledger

Working list for `res/cfg/argbounds.csv` (design: `docs/DESIGN-arg-bounds.md`).
Rows enter the CSV only when a source states the bound; everything weaker sits here
until it is verified in-game. Sources used: the game's own command help text
(`data/cvarlist_linux.txt`, mirrored into the catalog descriptions), community
documentation (official TF2 wiki, long-standing community scripts), and the Source
SDK's data structures.

The run that settled the open rows happened 2026-10-15 (repo owner, TF2 console;
Microsoft Narrator for the screen-reader half). Its raw answers, including the
proposals each row was tested against, are in `docs/in-game-checklist.md`, and the
`sv_pure` console log is kept in `data/sv_pure.txt`.

## Applied rows (mirrors `res/cfg/argbounds.csv`)

| Command | Arg | Bounds | Source | Confidence |
|---|---|---|---|---|
| `build` | 0 | int 0..2 | verified in-game (2026-10-13): 0 dispenser, 1 teleporter, 2 sentry | highest |
| `build` | 1 | int 0..1 | verified in-game (2026-10-13): teleporter subtype, 0 entrance 1 exit | highest |
| `cc_bot_selectweapon` | 1 | int 0..2 | verified in-game (2026-10-15): slots 0..2; higher values are silently ignored | highest |
| `coach_command` | 0 | int 0..3 | game help text enumerates 0:attack 1:defend 2:look 3:go | high |
| `dbghist_addline` | 0 | int 0..4 | game help text enumerates categories 0..4 | high |
| `dbghist_dump` | 0 | int 0..4 | same list in the dump help | high |
| `demo_setendtick` | 0 | int min 0 | game help text "Set to 0 to disable" | high |
| `demo_timescale` | 0 | flt min 0 | verified in-game (2026-10-15): 0 freezes playback; negative values are invalid | highest |
| `destroy` | 0 | int 0..2 | same mapping as `build` (verified 2026-10-13) | highest |
| `destroy` | 1 | int 0..1 | same mode semantics as `build` (verified 2026-10-13) | highest |
| `disguise` | 0 | int 1..9 | verified in-game (2026-10-13): 1 scout 2 sniper 3 soldier 4 demoman 5 medic 6 heavy 7 pyro 8 spy 9 engineer | highest |
| `disguise` | 1 | int -2..2 | verified in-game (2026-10-13): -2 friendly, -1 enemy, 1 blu, 2 red; 0 undocumented | highest |
| `eureka_teleport` | 0 | int 0..1 | game help text enumerates 0:home 1:teleporter exit | high |
| `fs_warning_level` | 0 | int 0..6 | verified in-game (2026-10-15): the bare command enumerates `FILESYSTEM_WARNING_*` 0..6 | highest |
| `impulse` | 0 | int 0..255 | SDK byte field, plus verified (2026-10-15) that >255 is accepted and a no-op, not an error | high |
| `load_itempreset` | 0 | int 0..3 | verified in-game by repo owner | highest |
| `mat_enable_vrmode` | 0 | int 0..1 | verified in-game (2026-10-15): clamped; takes effect on restart | highest |
| `mat_setvideomode` | 2 | int 0..1 | verified in-game (2026-10-15): 2 and up wrap to 1 | highest |
| `maxplayers` | 0 | int 1..101 | verified in-game (2026-10-15): 101 is the ceiling; 0 is accepted but the server never starts | highest |
| `next_map_vote` | 0 | int 0..2 | game help text `<0 \| 1 \| 2>` | high |
| `playvol` | 1 | flt min 0 | verified in-game (2026-10-15): -1 wraps to 0 | highest |
| `sv_pure` | 0 | int -1..2 | verified in-game (2026-10-15): -1/0/1/2 all set, 3 prints the usage (`data/sv_pure.txt`) | highest |
| `voicemenu` | 0 | int 0..2 | wiki/community: z/x/c = 0/1/2; the 2026-10-15 run left it alone | medium |
| `voicemenu` | 1 | int 0..8 | wiki/community: items 0..8; **menu 2 stops at 7** - see the caveat below | medium |

## Settled in-game (2026-10-15)

Applied from this run: `sv_pure`, `fs_warning_level`, `maxplayers`,
`mat_setvideomode` (arg 2), `mat_enable_vrmode`, `cc_bot_selectweapon` (arg 1),
`playvol` (arg 1) and `demo_timescale` - the eight rows in the table above.

Catalog rows this run changed (see the test that pins them):

- `benchframe` takes `<demoname> <frame> <tgafilename>` - `[str] [int] [str]`, not the
  single `[int]` the dump suggested.
- `bug` ignores its argument, so the row is `none`, the same treatment `pda_click` got.
- `sv_pure`'s catalog text was another command's ("Show user data.") and now carries the
  pure-mode rule list from the in-game help.
- `grab_window` answers **Unknown command** in the live client, so the row was removed
  rather than kept as a pickable entry that cannot work.

No bound to apply:

- `wait` is accepted for every value and does nothing - `sv_allow_wait_command` defaults
  off. The catalog description already says so.
- The engine does not clamp these, so bounding them would be invention:
  `tf_mvm_tank_health` (0 accepted), `cc_spawn_merasmus_at_level`, `cl_view`,
  `cl_find_ent_index`, `cl_pred_track`, `demo_gototick` (tick < 1 is a no-op; past the
  end skips to the end and exits to the menu), `Test_ProxyToggle_SetValue`,
  `Test_SpawnRandomEntities` (neither shows any effect).
- `vote`'s ranges depend on the vote that is running; there is no static range.

Open caveat: `voicemenu` item 8 exists on menus 0 and 1 but not on menu 2.
`argbounds.csv` carries one range per argument, so the union `0..8` stays and the guided
editor can offer item 8 on menu 2, where it does nothing. A per-menu table would be a new
file format; leave it until someone actually trips over it.

## Resolved: calling conventions and argument semantics

2026-10-13 (verified in-game by repo owner):

- `build`/`destroy`: the "modern" calling convention is `build <building> <mode>` with
  0 dispenser, 1 teleporter (mode 0 = entrance, 1 = exit), 2 sentry; the older
  community mapping (0..3) is wrong. Mode exists only for teleporter.
- `pda_click` takes no arguments in game (or any args are no-ops); the catalog row is
  now `none`, and it stays on the free-form generic path.
- `jointeam`: the console accepts both `blue` and `blu`; owned lines and UI emit `blu`.
- `disguise`: takes `<class> <team>` (class first) with the mappings in the applied
  rows above; bare `disguise` semantics (menu open?) remain unverified and unused.

2026-10-15: typing a command bare prints its real usage, and that text beats any
third-party list - which is how `benchframe`, `bug` and `fs_warning_level` above were
settled. `impulse` was in the same family: >255 is accepted and ignored, never rejected.

## Deliberately left unbounded

The engine accepts these values without a stable, knowable limit; do not bound them
without new evidence:

- The no-clamp group the 2026-10-15 run confirmed: `tf_mvm_tank_health`,
  `cc_spawn_merasmus_at_level`, `cl_view`, `cl_find_ent_index`, `cl_pred_track`,
  `demo_gototick`, `Test_ProxyToggle_SetValue`, `Test_SpawnRandomEntities`, `vote`.
- `addcond` / `removecond` condition IDs - the list grows every patch.
- `banid` minutes, `kickid`, `namelockid` - player/session IDs, unbounded.
- `bot_hurt` damage, `ent_fire` value/delay, `ai_set_move_height_epsilon`,
  `air_density`, `gl_persistent_buffer_max_offset` - free numeric inputs.
- Coordinates and angles: `drawcross`, `drawline`, `bot_teleport`, `spec_goto`.
- `mm_select_session`, `mm_session_info` - session IDs.
