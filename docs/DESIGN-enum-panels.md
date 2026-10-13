# Design: Enumerated-Choice Panels (second wave of custom panels)

Status: design, pending review. Extends `docs/DESIGN-custom-panels.md`: after
`voicemenu`, a census of the catalog shows a family of commands whose single argument
is a small named enumeration. They all fit one generic panel component driven by
per-command data, which is the cheapest way to add them and the template for future
waves.

## 1. Census (2026-10-13, client-controls commands with enumerable arguments)

| Command | Signature | Values | Source | Confidence |
|---|---|---|---|---|
| `jointeam` | `[str]` | `red`, `blue`, `auto`, `spectate` | game usage string in allcmds.csv | high |
| `joinclass`, `join_class` | `[str]` | `scout`, `soldier`, `pyro`, `demoman`, `heavyweapons`, `engineer`, `medic`, `sniper`, `spy`, `random`, `auto` | game usage string + class list | high |
| `build`, `destroy` | `[int] <int>` | 0 Dispenser, 1 Teleporter entrance, 2 Sentry gun, 3 Teleporter exit; optional `<mode>` stays out of the panel | community scripts | medium - verify |
| `eureka_teleport` | `[int]` | 0 home, 1 teleporter exit | game help text | high |
| `coach_command` | `[int]` | 0 attack, 1 defend, 2 look, 3 go | game help text | high |
| `next_map_vote` | `[int]` | 0, 1, 2 (map option) | game help text | high |
| `loot_response` | `[str]` | `common`, `rare`, `ultra_rare` | game help text | high |
| `load_itempreset` | `[int]` | 0..3, labeled **A / B / C / D** (the in-game preset names are hardcoded) | verified in-game by repo owner | high |
| `pda_click` | `[int] <int>` | argument semantics unknown | - | deferred, see §7 |
| `voicemenu` | `[int] [int]` | bespoke panel already shipped | - | done |

User decisions (2026-10-13): panel **all** enumerable commands; `build`/`destroy` get
a **rebuild compound** (`destroy N; build N` in one bind); `load_itempreset` included
with the A/B/C/D labels; string values are emitted as **bare tokens**
(`jointeam red`), matching wiki-script style.

## 2. `EnumChoicePanel.qml` (new, generic)

One component serves every row of the census table. It is the same three-part
contract as `VoiceMenuPanel` (see `docs/DESIGN-custom-panels.md` §3), with the
per-command data supplied by the registry:

- `property string commandName` - set by the dialog from the selected command, so one
  component can serve `build` and `destroy`.
- `property string valueType` - `"int"` or `"str"`; only affects prefill matching.
- `property var options` - `[{ value, label }]`, ordered as authored.
- `property string rebuildPartner` - optional; when set (only on `build`), the panel
  shows a "Rebuild (destroy + build)" checkbox and authors
  `destroy N; build N` instead of the single command.

Behavior:

- **UI**: one `Tf2Button` per option (checkable, selected = accent, like the voicemenu
  panel), in a Flow. No "open menu" concept - there is exactly one authored form.
- **Authoring**: clicking an option emits `authored(text)` where `text` is
  `commandName + " " + valueToken` (bare token, no quotes) or, with the rebuild box
  checked, `destroy N; build N` (`rebuildPartner` first, then the command).
- **prefill(parts)**: tokens after the command name, with a trailing `;` stripped:
  - plain `[value]` or `[value, ignored]`: select the option whose value matches
    (numeric for int, case-insensitive for str); unknown values keep the state and
    emit nothing (voicemenu's safety rule).
  - compound `partner N; command N` (4 tokens): selects the option and checks the
    rebuild box, so a key already bound to `destroy 2; build 2` re-opens correctly.
- The dialog extension to the panel contract: on load it also sets `commandName` and
  the registry entry's data fields. `VoiceMenuPanel` ignores the extra properties.

## 3. Registry changes (`BindEditorDialog`)

Registry entries grow from `Component` to a small object; the dialog unwraps it:

    property var customPanels: ({
        "voicemenu":  { component: voicemenuPanel },
        "jointeam":   { component: enumPanel, valueType: "str", options: [...] },
        "joinclass":  { component: enumPanel, valueType: "str", options: classOptions },
        "join_class": { component: enumPanel, valueType: "str", options: classOptions },
        "build":      { component: enumPanel, valueType: "int", options: buildingOptions,
                        rebuildPartner: "destroy" },
        "destroy":    { component: enumPanel, valueType: "int", options: buildingOptions },
        "eureka_teleport":  { component: enumPanel, valueType: "int", options: [...] },
        "coach_command":    { component: enumPanel, valueType: "int", options: [...] },
        "next_map_vote":    { component: enumPanel, valueType: "int", options: [...] },
        "loot_response":    { component: enumPanel, valueType: "str", options: [...] },
        "load_itempreset":  { component: enumPanel, valueType: "int",
                              options: [{value:0,label:"Preset A"}, ...] },
    })

Adding a future enumerated command is one object literal. The dialog code that
instantiates the panel sets `commandName`/`valueType`/`options`/`rebuildPartner`
before `initialArgs` so prefill can match immediately.

## 4. Label data (generic, per D-b of the first design)

- Classes: display names capitalized; values lowercase `heavyweapons` for Heavy.
- Teams: `RED`, `BLU`, `Auto-assign`, `Spectate`; values as the usage string spells
  them (`red`, `blue`, `auto`, `spectate`).
- Buildings: `Dispenser`, `Teleporter entrance`, `Sentry gun`, `Teleporter exit`.
- `next_map_vote`: `Option 1/2/3` - the actual maps are server-dependent.
- `load_itempreset`: `Preset A/B/C/D`.

## 5. Interaction with existing machinery

- The catalog `argbounds.csv` rows for these commands stay: they are the numeric
  safety net if a panel is ever removed, and the generic editors (still shown for
  every non-panel command) keep using bounds.
- The bind/bindtoggle toggle, Custom Command field and OK flow are untouched; a
  compound `destroy 2; build 2` lands in Custom Command exactly like any other text
  and `setBind` stores it as one command string (the game runs both on one press).
- `load_itempreset` keeps its verified `argbounds` row 0..3.

## 6. Tests and verification

QML is outside the engine test suite; the suite stays green (no C++ changes). Manual
verification per AGENTS.md:

- Each new command shows its option grid with the right labels/counts.
- A key bound to `joinclass heavyweapons` re-opens with Heavy selected.
- A key bound to `destroy 2; build 2` re-opens with Sentry + rebuild checked, and
  re-saving keeps the compound.
- Unknown bound values (e.g. a future class name) keep the Custom Command text and
  do not silently rewrite it.

## 7. Open questions (needs in-game verification)

- `build`/`destroy` building order 0..3 (community mapping) and what the optional
  `<mode>` argument does - until verified, the panel omits it.
- `jointeam`: does the console accept `blu`/`red` spellings other than the usage
  string's (`blue`)? The panel emits the usage-string spellings.
- `pda_click [int] <int>`: what the two arguments mean; deferred until known - it
  stays on the generic editors (now with no bounds, free-form).
- `disguise` (spy): usage says `none`, but the console may take team/class arguments.
  If verified, it is a natural next enum panel (team + class two-level, like
  voicemenu's structure).

## 8. Rollout

1. `EnumChoicePanel.qml` + registry refactor + all entries in one commit (inert until
   a command is selected; generic editors remain the fallback everywhere).
2. Manual verification; adjust labels per feedback.
