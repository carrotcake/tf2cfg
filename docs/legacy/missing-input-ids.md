# Bind keys missing from the Edit Binds model

The Edit Binds panel (qml/keyboard/KeyTable.qml) models a standard keyboard plus
`mouse1`–`mouse5` and `mwheelup`/`mwheeldown`, and an Xbox 360-style controller
block with the first ten joystick buttons, the four POV directions, and ten
axis-direction buttons. This file lists every further Source-engine input id
that TF2 accepts for `bind` but that the model does not cover, with the
hardware each id maps to.

Sources, in order of authority:

- `ButtonCode.h` and `InputEnums.h` from the TF2 branch of the Source SDK
  (`alliedmodders/hl2sdk`, branch `tf2`) — the engine's full input enumeration.
- `tf/cfg/360controller.cfg`, shipped with TF2 — Valve's own controller binds,
  proving the joystick/POV/axis names are live and accepted by `bind`
  (`bind "JOY1" "+jump"`, `bind "POV_UP" "slot1"`, `bind "Z AXIS POS" "+attack2"`).
- The TF2 wiki Scripting page, "List of key names" — confirms Menu and PrtScn
  "Cannot be bound", matching the model's two un-bindable caps.

## Now present in the model (added 2026-10-12)

The controller block in `KeyTable.qml` now includes:

| id | 360 pad hardware | Notes |
|---|---|---|
| `joy1`–`joy4` | A / B / X / Y | |
| `joy5`–`joy6` | Left / Right bumper | |
| `joy7`–`joy8` | Back / Start | |
| `joy9` | Left stick click (L3) | |
| `joy10` | Right stick click (R3) | |
| `pov_up` / `pov_right` / `pov_down` / `pov_left` | D-pad / hat switch | |
| `x axis neg` / `x axis pos` | Left stick left / right | |
| `y axis neg` / `y axis pos` | Left stick up / down | |
| `z axis pos` / `z axis neg` | Left trigger / Right trigger | |
| `u axis neg` / `u axis pos` | Right stick left / right | |
| `v axis neg` / `v axis pos` | Right stick up / down | |

These map 1:1 to Valve's shipped `360controller.cfg` plus the axis directions
the engine exposes as binary "buttons" (each analog direction is a separate
bindable id, e.g. `bind "x axis pos" "+moveleft"`).

## Confirmed bindable, still missing from the model

### Joystick / gamepad buttons: `joy11` .. `joy32`

Binary buttons of a joystick or gamepad (joystick 0 only; TF2's
`MAX_JOYSTICKS = 1`). On Valve's shipped 360-controller layout the first ten
map to the Xbox 360 pad (see table above).

`joy11`–`joy32` exist in the engine enumeration
(`JOYSTICK_MAX_BUTTON_COUNT = 32`) and are reachable on generic joysticks and
gamepads with more than ten buttons; Valve's shipped config never uses them,
so treat them as valid-but-unproven before relying on one.

### Roll axis directions: `r axis pos`, `r axis neg`

The roll (twist) axis of a joystick, exposed as two binary directions. Present
in `InputEnums.h`; axis order is X, Y, Z, R, U, V on Windows and X, Y, Z, U,
R, V on Linux, which only matters for which hardware `r` lands on.

## Engine-enumerated but not proven bindable in TF2

Present in the SDK's `ButtonCode_t`, absent from every TF2 community/shipped
key list. Do not surface in the UI without an in-game check:

- `break` — the Break/SysRq key, absent from modern keyboards.
- `capslocktoggle`, `numlocktoggle`, `scrolllocktoggle` — the toggle-state
  twin events of Caps/Num/Scroll Lock.
- Novint Falcon buttons (`NOVINT_LOGO/TRIANGLE/BOLT/PLUS` × device 0/1) — the
  Novint Falcon haptic controller, which TF2 briefly supported (2008); the
  public SDK does not show the string names TF2 assigns them.
- Steam Controller codes — enumerated (`STEAMCONTROLLER_*`) but bound through
  Steam Input, not through cfg `bind` lines in practice.

## Already correct in the model

- `menu` (engine `KEY_APP`) and `prtsc`: "Cannot be bound" per the TF2 wiki —
  the model's two token-less caps match exactly.
- Every other keyboard name on the wiki list, including `lwin`/`rwin` and all
  `kp_*` names, is present and bindable.

## Parser compatibility

Quoted keys with spaces (`bind "z axis pos" "+attack2"`) flow through the
existing pipeline unchanged: the config lexer keeps quotes on arguments,
`CommandListModel::binds()`/`setBind()`/`bindRows()` compare the unquoted,
lowercased key, and the bind map keys are plain strings — so any future
joystick/axis support in the Edit Binds UI needs UI work only, not parser
work. Note `CommandListModel::setBind()` lowercases commands too; the space
inside an axis key name is safe because the whole key is one quoted argument.