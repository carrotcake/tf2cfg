# allcmds_v5.csv schema

10 comma-separated fields, quoted, headerless, sorted by command name:

| # | Field | Example | Notes |
|---|-------|---------|-------|
| 1 | Name | `cl_interp` | Exact console name (`+`/`-` prefix = hold/release pair) |
| 2 | Type | `cmd` / `ncmd` / empty | `cmd` = declared command, `ncmd` = undeclared (intercepted by game code), empty = console variable |
| 3 | Default | `0.1` | Default value (cvars only) |
| 4 | Min | `0` | Engine-enforced lower bound (cvars only; rare) |
| 5 | Max | `0.5` | Engine-enforced upper bound (cvars only; rare) |
| 6 | Flags | `client archive userinfo` | FCVAR tags — see [flags.md](flags.md) |
| 7 | Description | `Sets the interpolation amount...` | Tooltip-style help text |
| 8 | Input type | `flt` / `[str] <str>` | See below |
| 9 | Platform | `universal` | See below |
| 10 | Context | `client settings` | See below |

## Input type (field 8)

For cvars — the value type the variable accepts:

- `bool` — 0/1 toggle
- `int` — integer
- `flt` — floating point
- `str` — string
- `flt flt flt` — 3-vector (`fog_color`)

Derived mechanically from the default value; a decimal point in min or max forces
`flt` (e.g. `sensitivity` defaults to integer-looking `3` but is a float).
Descriptions listing multiple numbered modes force `int` (e.g. `cl_showfps`).
Edge cases are listed in `INPUT_OVERRIDES` in `field_dicts.py`.

For commands — `none` if the command takes no parameters, otherwise the parameter
list, space separated: `[type]` = mandatory parameter, `<type>` = optional parameter.

- `bind [str] [str]` — key and command, both mandatory
- `addcond [int] <int>` — condition id, optional value
- `bot_teleport [str] [flt] [flt] [flt] [flt] [flt] [flt]` — name + position + angles

Signatures are authored per command in `SIGS` (field_dicts.py), informed by the
command's documented usage string. ~860 commands take no parameters; commands
whose parameter lists are not publicly documented default to `none` — verify
against the in-game console before relying on a signature.

## Platform (field 9)

- `universal` — works on all platforms (3,762 entries)
- `linux` — ToGL/Linux renderer internals (`gl_*`, `glm_*`; 48 entries)
- `windows` — D3D9Ex/Windows-specific (`dxa_*`, `windows_speaker_config`)
- `xbox` — Xbox 360 port only, inert on PC (13 entries)

Peripheral-gated features that ship on PC (`sixense_*` Razer Hydra, `hap_*`
Novint Falcon, `vr_*`) are `universal` — they register and run everywhere.

## Context (field 10)

One primary in-game usage context:

- `client controls` — in-game action commands (move, fire, taunt, vote, join, ...)
- `client settings` — client-side configuration (input, audio, HUD, network prefs, demo prefs)
- `client render` — rendering pipeline (mat_\*, r_\*, gl_\*, fog, camera/FOV, particles, ragdolls)
- `server commands` — operate the server itself (bots, kicks/bans, changelevel, ent_\*, nav)
- `server settings` — server-side cvars (sv_\*, mp_\*, gameplay tuning)
- `engine` — engine tooling: config/file system, demo playback, profiling, memory, crash reporting, debug overlays

`cheat` does not affect classification. Classification rule order: explicit
overrides → flags (`game` ⇒ server side) → name prefixes/keywords → fallback.
Player-action commands that execute server-side (`voicemenu`, `jointeam`,
`build`, `callvote`, ...) are `client controls` — they are usable from any
client on any server, which is what separates them from `server commands`.
