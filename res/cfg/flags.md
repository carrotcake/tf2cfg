# Console Command Tags (FCVAR flags)

The `Flags` column of `allcmds.csv` holds the Source-engine FCVAR flags each
console command or variable was registered with. Sources render the same flags
under different names; this dataset normalizes all of them to one canonical
vocabulary (left column).

## Canonical tags

| Tag | Seen as | FCVAR | Meaning | Applies to |
|-----|---------|-------|---------|------------|
| `archive` | `a` | FCVAR_ARCHIVE | Value is saved to `config.cfg` | cvar |
| `archivexbox` | — | FCVAR_ARCHIVE_XBOX | Value is saved on Xbox 360 builds | cvar |
| `client` | `cl` | FCVAR_CLIENTDLL | Defined by the client DLL (client-side behavior) | both |
| `game` | `sv` | FCVAR_GAMEDLL | Defined by the server game DLL (server-side behavior) | both |
| `replicated` | `rep` | FCVAR_REPLICATED | Server value is replicated to clients on join/change | cvar |
| `userinfo` | `user` | FCVAR_USERINFO | Value is part of the user info string sent to the server | cvar |
| `notify` | `nf` | FCVAR_NOTIFY | Changing the value announces it to all players | cvar |
| `cheat` | — | FCVAR_CHEAT | Requires `sv_cheats 1` (or a singleplayer server) | both |
| `norecord` | `nr` | FCVAR_DONTRECORD | Not recorded into demo files | both |
| `demo` | — | FCVAR_DEMO | Value is recorded into demo files | cvar |
| `hidden` | — | FCVAR_HIDDEN | Hidden from `find` and console autocomplete | both |
| `protected` | `prot` | FCVAR_PROTECTED | Value is a secret (password); masked when queried | cvar |
| `singleplayer` | `sp` | FCVAR_SPONLY | Cannot be changed by clients connected to a multiplayer server | cvar |
| `notconnected` | — | FCVAR_NOT_CONNECTED | Cannot be changed while connected to a server | cvar |
| `svcanexec` | `server_can_execute` | FCVAR_SERVER_CAN_EXECUTE | Server is allowed to execute this client command | command |
| `clcmdcanexec` | `clientcmd_can_execute` | FCVAR_CLIENTCMD_CAN_EXECUTE | May be executed as a client command (e.g. from the server's client scripts) | both |
| `svcannotquery` | — | FCVAR_SERVER_CANNOT_QUERY | Clients cannot query the value (e.g. `password`, `rcon_password`) | cvar |
| `printonly` | `print` | FCVAR_PRINTABLEONLY | Value must contain printable characters only | cvar |
| `numeric` | `neverprint` | (numeric constraint) | Value must be numeric; the wiki renders this flag as `neverprint` | cvar |
| `matsysthread` | `matsys` | (TF2-branch flag) | Settable only from the material-system thread (64-bit branch) | cvar |

Empty flags = `FCVAR_NONE` (registered with no flags). This is a real value, not
missing data — it has been verified against all three dump sources.

## Evidence sources (in order of authority)

1. **TF2 wiki** (wiki CSV) — first-party TF2 rendering, includes platform/thread flags the Linux dump lacks (`archivexbox`, `notconnected`, `matsysthread`, `svcannotquery`).
2. **TF2 Linux cvarlist dump** (rufio-tf2 gist) — first-party dump; uses short tokens (`a`, `cl`, `sv`, `rep`, `nf`, `prot`, `sp`, `print`). Records **no flags for commands**.
3. **CS2 1.39.6.5 dump** (Oct 2023, SuGolYolLom gist) — explicit `FCVAR_*` names for commands too. Used **only to fill empty flags**, restricted to flags with a TF2 equivalent. `FCVAR_DEVELOPMENTONLY`, `FCVAR_RELEASE`, `FCVAR_MISSING*`, and CS2 UI flags are ignored (build artifacts, present on ~2,000 entries).

Merge rules:

- Cvars: union of wiki + Linux dump tokens (normalized), preserving wiki order.
- Empty flags: filled from CS2 evidence when the same name exists there (35 entries: e.g. `exec`/`kick`/`kickid`/`load`/`toggle` = `norecord`, `gameui_*` = `hidden`, `restart` = `cheat`, `autosave`/`save_finish_async`/`tv_msg` = `game`, `buildcubemaps` = `client`).
- Conflicts: TF2 sources win over CS2. Cross-check of all non-empty rows with CS2 evidence: 498 agree, 175 differ — differences are engine-version drift (CS2 stripped/added flags on shared cvars between 2016 and 2023), not errors.

## Coverage

- 3,825 entries; 3,207 carry at least one tag; 618 empty (`FCVAR_NONE`).
- Remaining empties cluster in: `adsp_*` (sound-propagation thresholds), `gl_*` (ToGL internals), `ai_*` NPC tuning, `sixense_*` Hydra, `vr_*`, `vprof_*` debug tools, and plain engine commands (`quit`, `maps`, `status`, ...). All three sources agree these have no flags.
