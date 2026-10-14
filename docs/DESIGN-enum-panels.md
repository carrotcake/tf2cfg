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
| `build`, `destroy` | `[int] <int>` | **verified 2026-10-13**: `<building> <mode>` with 0 Dispenser, 1 Teleporter (mode 0 entrance, 1 exit), 2 Sentry gun; mode exists only for teleporter | in-game, repo owner | highest |
| `eureka_teleport` | `[int]` | 0 home, 1 teleporter exit | game help text | high |
| `coach_command` | `[int]` | 0 attack, 1 defend, 2 look, 3 go | game help text | high |
| `next_map_vote` | `[int]` | 0, 1, 2 (map option) | game help text | high |
| `loot_response` | `[str]` | `common`, `rare`, `ultra_rare` | game help text | high |
| `load_itempreset` | `[int]` | 0..3, labeled **A / B / C / D** (the in-game preset names are hardcoded) | verified in-game by repo owner | high |
| `disguise` | `[int] <int>` | **verified 2026-10-13**: `disguise <class> <team>` (class first); class 1 scout 2 sniper 3 soldier 4 demoman 5 medic 6 heavy 7 pyro 8 spy 9 engineer; team -2 friendly, -1 enemy, 1 blu, 2 red. Shipped as the bespoke `DisguisePanel`. | in-game, repo owner | highest |
| `pda_click` | - | **resolved 2026-10-13**: takes no arguments in game (or any args are no-ops); catalog row is now `none`, stays on generic editors | in-game, repo owner | highest |
| `voicemenu` | `[int] [int]` | bespoke panel already shipped | - | done |

User decisions (2026-10-13): panel **all** enumerable commands; `build`/`destroy` get
a **rebuild compound** (`destroy N; build N` in one bind); `load_itempreset` included
with the A/B/C/D labels; string values are emitted as **bare tokens**
(`jointeam red`), matching wiki-script style. Verified facts (same day, in-game):
`build`/`destroy` use the two-argument modern convention (§1), `jointeam` emits `blu`
(the console accepts `blue` too), `pda_click` is a no-argument command, and `disguise`
is class-first two-argument - shipped as its own panel.

## 2. `EnumChoicePanel.qml` (new, generic)

One component serves every row of the census table. It is the same three-part
contract as `VoiceMenuPanel` (see `docs/DESIGN-custom-panels.md` §3), with the
per-command data supplied by the registry:

- `property string commandName` - set by the dialog from the selected command, so one
  component can serve `build` and `destroy`.
- `property string valueType` - `"int"` or `"str"`; only affects prefill matching.
- `property var options` - `[{ value, label }]`, ordered as authored.
- `property string compoundTemplate` - optional; `"destroy {v}; build {v}"` shows a
  Rebuild checkbox (each `{v}` is filled with the selected value) and authors the
  template instead of the single command. The word in front of `{v}` is the partner
  command name the compound prefill looks for.

Behavior:

- **UI**: one `Tf2Button` per option (checkable, selected = accent, like the voicemenu
  panel), in a Flow. No "open menu" concept - there is exactly one authored form.
- **Authoring**: clicking an option emits `authored(text)` where `text` is
  `commandName + " " + value` (bare token(s), no quotes) or, with the rebuild box
  checked, the filled compound template (`destroy 1 1; build 1 1` for the teleporter
  exit).
- **prefill(parts)**: tokens after the command name, with a trailing `;` stripped.
  Values may be multi-token (`build 1 1` arrives as `["1","1"]`), so matching joins
  the token stream:
  - plain: the joined tokens select the option whose value matches (case-insensitive
    for str, numeric for int); unknown values keep the state and emit nothing
    (voicemenu's safety rule).
  - compound: the partner command name is located inside the token stream; the tokens
    before and after it must match the same option, e.g. a bound
    `destroy 1 1; build 1 1` reached the build/destroy panel as
    `["1","1","build","1","1"]` - selects the option and checks the rebuild box, so
    the compound re-opens correctly.
- The dialog extension to the panel contract: on load it also sets `commandName` and
  the registry entry's data fields. `VoiceMenuPanel` ignores the extra properties.

## 3. Registry changes (`BindEditorDialog`)

Registry entries grow from `Component` to a small object; the dialog unwraps it:

    property var customPanels: ({
        "voicemenu":  { component: voicemenuPanel },
        "jointeam":   { component: enumPanel, valueType: "str", options: [...] },
        "joinclass":  { component: enumPanel, valueType: "str", options: classOptions },
        "join_class": { component: enumPanel, valueType: "str", options: classOptions },
        "build":      { component: enumPanel, valueType: "str", options: buildingOptions,
                        compoundTemplate: "destroy {v}; build {v}" },
        "destroy":    { component: enumPanel, valueType: "str", options: buildingOptions,
                        compoundTemplate: "destroy {v}; build {v}" },
        "eureka_teleport":  { component: enumPanel, valueType: "int", options: [...] },
        "coach_command":    { component: enumPanel, valueType: "int", options: [...] },
        "next_map_vote":    { component: enumPanel, valueType: "int", options: [...] },
        "loot_response":    { component: enumPanel, valueType: "str", options: [...] },
        "load_itempreset":  { component: enumPanel, valueType: "int",
                              options: [{value:0,label:"Preset A"}, ...] },
        "disguise":   { component: disguisePanel },   // bespoke two-row panel
    })

`buildingOptions` carries two-token values in the verified convention:
`{value:"0 0",label:"Dispenser"}, {value:"1 0",label:"Teleporter entrance"},
{value:"1 1",label:"Teleporter exit"}, {value:"2 0",label:"Sentry gun"}`. The
`disguise` panel is bespoke (`DisguisePanel.qml`): a class row (ids 1..9) and a team
row (-2 friendly, -1 enemy, 1 blu, 2 red), emitting `disguise <class> <team>`; it
defaults to enemy Scout and follows the voicemenu safety rule for malformed bindings.

Adding a future enumerated command is one object literal. The dialog code that
instantiates the panel sets `commandName`/`valueType`/`options`/`rebuildPartner`
before `initialArgs` so prefill can match immediately.

## 4. Label data (generic, per D-b of the first design)

- Classes: display names capitalized; values lowercase `heavyweapons` for Heavy.
- Teams: `RED`, `BLU`, `Auto-assign`, `Spectate`; values `red`, `blu`, `auto`,
  `spectate` - `blu` verified in-game (the console also accepts `blue`, we emit `blu`).
- Buildings: values in the verified two-argument convention (see §3); labels
  `Dispenser`, `Teleporter entrance`, `Teleporter exit`, `Sentry gun`.
- `disguise`: class labels Scout..Engineer, team labels
  `Friendly`/`Enemy`/`BLU`/`RED` with the verified ids in the panel file header.
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
- A key bound to `destroy 1 1; build 1 1` re-opens with Teleporter exit + rebuild
  checked, and re-saving keeps the compound.
- Unknown bound values (e.g. a future class name) keep the Custom Command text and
  do not silently rewrite it.

## 7. Open questions → settled (2026-10-13, in-game by repo owner)

- `build`/`destroy`: the "modern" convention is `build <building> <mode>`; 0 dispenser,
  1 teleporter (mode 0 = entrance, 1 = exit), 2 sentry. The older community mapping
  (0..3, exit = 3) is wrong and was replaced. Panel options now carry two-token values.
  Old one-argument binds migrate on dialog open: `modernizedCommand()` in
  `BindEditorDialog` rewrites `build|destroy 0..3` (quote-aware, per `;` part) to the
  modern form in the Custom Command field before the user edits; OK commits the
  modernized text, Cancel keeps the key's bound command untouched.
- `jointeam`: the console accepts both `blue` and `blu`; owned lines and UI emit `blu`.
- `pda_click`: accepts no args in game (or args are no-ops). Catalog row is `none`;
  no panel, stays on the generic (now argument-less) path.
- `disguise`: takes `<class> <team>` (class first, ids in §1); shipped as
  `DisguisePanel.qml`. Bare `disguise` (no args) semantics remain unverified and are
  not authored by the panel.

Details and the bounds rows: `docs/argbounds-verification.md` (Applied/Resolved).

## 8. Rollout

1. `EnumChoicePanel.qml` + registry refactor + all entries in one commit (inert until
   a command is selected; generic editors remain the fallback everywhere).
2. Manual verification; adjust labels per feedback.
