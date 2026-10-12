#!/usr/bin/env python3
"""
Populate empty descriptions (col 7) in res/cfg/allcmds.csv from authored
knowledge. Rewrites the canonical file in place (previous copy rotated to
allcmds_bak.csv). Existing descriptions untouched. Run from the repo root.
"""
from csvio import read, write
from desc_part1 import D1
from desc_part2 import D2
from desc_part3 import D3
from desc_part4 import D4
from desc_part5 import D5
from desc_part6 import D6
from desc_part7 import D7
from desc_part8 import D8

AUTHORED = {}
AUTHORED.update(D1); AUTHORED.update(D2); AUTHORED.update(D3); AUTHORED.update(D4)
AUTHORED.update(D5); AUTHORED.update(D6); AUTHORED.update(D7); AUTHORED.update(D8)

# fix typo
AUTHORED.pop('glm_litererefresh_capslock', None)
AUTHORED['glm_literefresh_capslock'] = "Internal GLM debug workaround (caps-lock lite refresh)."

# sixense family: mechanical but accurate (names are self-describing)
def humanize(name):
    n = name[len('sixense_'):]
    return n.replace('_', ' ')

for name, desc in {
    "sixense_aim_1to1_heading_multiplier": "Razer Hydra: heading multiplier for 1:1 aiming.",
    "sixense_aim_1to1_pitch_multiplier": "Razer Hydra: pitch multiplier for 1:1 aiming.",
    "sixense_aim_1to1_ratchet_vertical": "Razer Hydra: vertical ratchet for 1:1 aiming.",
    "sixense_aim_freeaim_accel_band_exponent": "Razer Hydra: free aim acceleration band exponent.",
    "sixense_aim_freeaim_accel_band_size": "Razer Hydra: free aim acceleration band size.",
    "sixense_aim_freeaim_auto_level_rate": "Razer Hydra: rate free aim auto-levels the view.",
    "sixense_aim_freeaim_dead_zone_radius": "Razer Hydra: free aim dead zone radius.",
    "sixense_aim_freeaim_heading_multiplier": "Razer Hydra: free aim heading multiplier.",
    "sixense_aim_freeaim_max_speed": "Razer Hydra: maximum free aim speed.",
    "sixense_aim_freeaim_pitch_multiplier": "Razer Hydra: free aim pitch multiplier.",
    "sixense_aim_freeaim_spin_disabled": "Razer Hydra: disable free aim spin.",
    "sixense_aim_freeaim_switch_blend_time_enter": "Razer Hydra: blend time entering free aim.",
    "sixense_aim_freeaim_switch_blend_time_exit": "Razer Hydra: blend time exiting free aim.",
    "sixense_aim_scope_heading_multiplier": "Razer Hydra: heading multiplier while scoped.",
    "sixense_aim_scope_pitch_multiplier": "Razer Hydra: pitch multiplier while scoped.",
    "sixense_always_draw_crosshair": "Razer Hydra: always draw the crosshair.",
    "sixense_base_offset_x": "Razer Hydra: base station X offset.",
    "sixense_base_offset_y": "Razer Hydra: base station Y offset.",
    "sixense_base_offset_z": "Razer Hydra: base station Z offset.",
    "sixense_controller_angle_mode": "Razer Hydra: controller angle mode.",
    "sixense_crosshair_horiz_multiplier": "Razer Hydra: crosshair horizontal multiplier.",
    "sixense_crosshair_vert_multiplier": "Razer Hydra: crosshair vertical multiplier.",
    "sixense_crouch_sensitivity": "Razer Hydra: crouch gesture sensitivity.",
    "sixense_disable_gestures": "Razer Hydra: disable gestures.",
    "sixense_enabled": "Enable Razer Hydra motion controller support.",
    "sixense_exit_metroid_blend": "Razer Hydra: blend threshold to exit metroid drift mode.",
    "sixense_exit_one_to_one_dot": "Razer Hydra: dot threshold to exit 1:1 aiming.",
    "sixense_features_enabled": "Razer Hydra: feature flags in use.",
    "sixense_feet_angles_offset_stick_spin_exponent": "Razer Hydra: stick spin exponent for feet angles offset.",
    "sixense_feet_angles_offset_stick_spin_horiz_multiplier": "Razer Hydra: horizontal multiplier for feet angle stick spin.",
    "sixense_feet_angles_offset_stick_spin_invert_pitch": "Razer Hydra: invert pitch for feet angle stick spin.",
    "sixense_feet_angles_offset_stick_spin_vert_multiplier": "Razer Hydra: vertical multiplier for feet angle stick spin.",
    "sixense_filter_level": "Razer Hydra: controller signal filter level.",
    "sixense_jump_sensitivity": "Razer Hydra: jump gesture sensitivity.",
    "sixense_left_handed": "Razer Hydra: use left-handed controller mode.",
    "sixense_max_charge_spin": "Razer Hydra: maximum spin from charging gestures.",
    "sixense_melee_pitch_blend_val": "Razer Hydra: pitch blend value for melee swings.",
    "sixense_mode": "Razer Hydra: input mode.",
    "sixense_mouse_enabled": "Razer Hydra: enable virtual mouse control.",
    "sixense_mouse_sensitivity": "Razer Hydra: virtual mouse sensitivity.",
    "sixense_point_gesture_angle_threshold": "Razer Hydra: angle threshold for the point gesture.",
    "sixense_reload_sensitivity": "Razer Hydra: reload gesture sensitivity.",
    "sixense_roll_correct_blend": "Razer Hydra: blend value for roll correction.",
    "sixense_select_grenade": "Razer Hydra: select grenade via gesture.",
    "sixense_select_machinegun": "Razer Hydra: select machine gun via gesture.",
    "sixense_select_melee": "Razer Hydra: select melee weapon via gesture.",
    "sixense_select_pistol": "Razer Hydra: select pistol via gesture.",
    "sixense_sensitivity_level": "Razer Hydra: sensitivity level preset.",
    "sixense_set_base_offset": "Razer Hydra: set the base station offset.",
    "sixense_set_filter_params": "Razer Hydra: set controller filter parameters.",
    "sixense_spring_view_enabled": "Razer Hydra: enable spring view.",
    "sixense_spring_view_max_angle": "Razer Hydra: spring view maximum angle.",
    "sixense_spring_view_max_spring": "Razer Hydra: spring view maximum springiness.",
    "sixense_spring_view_min_angle": "Razer Hydra: spring view minimum angle.",
    "sixense_spring_view_min_spring": "Razer Hydra: spring view minimum springiness.",
    "sixense_teleport_metroid_blend_time": "Razer Hydra: blend time for teleport (metroid style).",
    "sixense_teleport_wait_to_blend_time": "Razer Hydra: wait time before teleport blending.",
    "sixense_tilt_gesture_angle_threshold": "Razer Hydra: angle threshold for the tilt gesture.",
    "sixense_trigger_threshold": "Razer Hydra: trigger squeeze threshold.",
    "sixense_walking_dead_zone_percent": "Razer Hydra: walking dead zone percent.",
    "sixense_walking_exponent": "Razer Hydra: walking movement exponent.",
    "sixense_weapon_select_sensitivity": "Razer Hydra: weapon select gesture sensitivity.",
    "sixense_zoom_momentary_time": "Razer Hydra: momentary zoom time in ms.",
}.items():
    AUTHORED[name] = desc

# sk_* hitgroup damage multipliers
for name, desc in {
    "sk_autoaim_mode": "Autoaim mode (0 = off, 1 = assist).",
    "sk_npc_arm": "Damage multiplier for NPCs hit in the arm.",
    "sk_npc_chest": "Damage multiplier for NPCs hit in the chest.",
    "sk_npc_head": "Damage multiplier for NPCs hit in the head.",
    "sk_npc_leg": "Damage multiplier for NPCs hit in the leg.",
    "sk_npc_stomach": "Damage multiplier for NPCs hit in the stomach.",
    "sk_player_arm": "Damage multiplier for players hit in the arm.",
    "sk_player_chest": "Damage multiplier for players hit in the chest.",
    "sk_player_head": "Damage multiplier for players hit in the head.",
    "sk_player_leg": "Damage multiplier for players hit in the leg.",
    "sk_player_stomach": "Damage multiplier for players hit in the stomach.",
    "sk_plr_dmg_grenade": "Grenade damage dealt by players.",
}.items():
    AUTHORED[name] = desc

# slots
for i in range(13):
    AUTHORED[f"slot{i}"] = f"Select weapon slot {i}."

AUTHORED.update({
    "-attack": "Release the attack input.",
    "-attack2": "Release the secondary attack input.",
    "CreateHairball": "Internal: create a hairball effect (unused/debug).",
    "OpenSteamWorkshopDialog": "Open the Steam Workshop dialog.",
    "tf_fall_damage_disablespread": "Disable fall damage randomness.",
    "tf_player_spell_drop_on_death_rate": "Rate of Halloween spell drops on death.",
    "tf_populator_damage_multiplier": "Damage multiplier for MvM wave populators.",
    "tf_populator_debug": "Debug the MvM wave population system.",
    "tf_populator_health_multiplier": "Health multiplier for MvM wave populators.",
    "tf_powerup_max_charge_time": "Maximum charge time for power-up mode pickups.",
})

rows = read()
filled = 0
fallbacks = []
for r in rows:
    if not r[6]:
        if r[0] in AUTHORED:
            r[6] = AUTHORED[r[0]]
            filled += 1
        else:
            r[6] = "Undocumented engine variable."
            fallbacks.append(r[0])

print(f"Authored: {filled}, fallback: {len(fallbacks)}")
for n in fallbacks:
    print(f"  {n}")

names = [r[0] for r in rows]
assert len(names) == len(set(names)), "duplicate names!"

write(rows)
print(f"\nWrote {len(rows)} rows to res/cfg/allcmds.csv")
ndesc = sum(1 for r in rows if r[6])
print(f"Non-empty descriptions: {ndesc}/{len(rows)}")
