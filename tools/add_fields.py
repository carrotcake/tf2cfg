#!/usr/bin/env python3
"""
Recompute the three derived fields (cols 8-10) of res/cfg/allcmds.csv in place
(previous copy rotated to allcmds_bak.csv):
  col 8: input type   (bool/int/flt/str for cvars; none or [mandatory]/<optional> types for commands)
  col 9: platform     (universal/xbox/linux/windows)
  col 10: in-game context (client controls/client settings/client render/server commands/server settings/engine)
Run from the repo root.
"""
import re
from collections import Counter

from csvio import read, write
from field_dicts import SIGS, SERVER_CMDS, PLAYER_ACTIONS, INPUT_OVERRIDES, CONTEXT_OVERRIDES

rows = read()

# ---------------- input type ----------------
FLOAT_RE = re.compile(r'^-?(\d+\.\d*|\.\d+)(f?)$|^-\d+f$')
INT_RE = re.compile(r'^-?\d+$')
VEC3_RE = re.compile(r'^-?\d+(\.\d+)? -?\d+(\.\d+)? -?\d+(\.\d+)?$')

def cvar_input(r):
    name, d, mx, desc = r[0], r[2], r[4], r[6]
    if name in INPUT_OVERRIDES:
        return INPUT_OVERRIDES[name]
    if d == '':
        return 'str'
    if VEC3_RE.match(d):
        return 'flt flt flt'
    if FLOAT_RE.match(d):
        return 'flt'
    if INT_RE.match(d):
        # decimal min/max implies the value is really a float (e.g. sensitivity)
        if ('.' in r[3] or '.' in mx):
            return 'flt'
        if d in ('0', '1') and (mx == '' or float(mx) <= 1):
            # multi-mode descriptions hint at int enums
            if re.search(r'\b[2-9]\s*=', desc):
                return 'int'
            return 'bool'
        return 'int'
    return 'str'

def cmd_input(name):
    if name in SIGS:
        return SIGS[name]
    return 'none'

# ---------------- platform ----------------
XBOX_PREFIX = ('x360_', 'xbox_', 'xc_', 'dump_x360')
def platform(name):
    if name.startswith(XBOX_PREFIX) or name == 'gameui_xbox':
        return 'xbox'
    if name.startswith(('gl_', 'glm_')):
        return 'linux'
    if name.startswith('dxa_') or name == 'windows_speaker_config':
        return 'windows'
    return 'universal'

# ---------------- context ----------------
RENDER_PREFIX = ('mat_', 'r_', 'gl_', 'glm_', 'fog_', 'pyro_', 'rope_')
RENDER_SUBSTR = ('cam', 'draw', 'render', 'wireframe', 'texture', 'material', 'particle',
                 'decal', 'shadow', 'lod', 'bloom', 'blur', 'glow', 'water', 'skybox',
                 'sprite', 'viewmodel', 'ragdoll', 'detail', 'overlay', 'dof', 'vignette',
                 'crosshair', 'flashlight', 'lightmap', 'cubemap', 'phong', 'bump',
                 'luxel', 'fillrate', 'tonemap', 'hdr', 'envmap', 'muzzle', 'spore',
                 'teeth', 'eyes', 'eyegloss', 'brass', 'effect', 'prop', 'leaf', 'frustum',
                 'pixelvis', 'screenfade')

def context(r):
    name, typ, flags, desc = r[0], r[1], r[5], r[6]
    if name in CONTEXT_OVERRIDES:
        return CONTEXT_OVERRIDES[name]
    is_cmd = typ in ('cmd', 'ncmd')
    if is_cmd:
        if name.startswith(('+', '-')):
            return 'client controls'
        if name in PLAYER_ACTIONS:
            return 'client controls'
        if name in SERVER_CMDS or 'game' in flags.split():
            return 'server commands'
        return 'engine'
    # cvar
    if name in INPUT_OVERRIDES and name in CONTEXT_OVERRIDES:
        return CONTEXT_OVERRIDES[name]
    if 'game' in flags.split():
        return 'server settings'
    if name.startswith(RENDER_PREFIX):
        return 'client render'
    n = name.lower()
    if any(s in n for s in RENDER_SUBSTR):
        return 'client render'
    return 'client settings'

# ---------------- apply ----------------
out = []
for r in rows:
    if r[1] in ('cmd', 'ncmd'):
        inp = cmd_input(r[0])
    else:
        inp = cvar_input(r)
    r += [''] * (10 - len(r))   # tolerate short legacy rows
    r[7], r[8], r[9] = inp, platform(r[0]), context(r)
    out.append(r)

write(out)
print("Wrote res/cfg/allcmds.csv (previous copy in allcmds_bak.csv)")

# ---------------- stats ----------------
n = len(out)
print(f"rows: {n}")
print("input:", dict(Counter(r[7] for r in out).most_common(12)))
print("platform:", dict(Counter(r[8] for r in out)))
print("context:", dict(Counter(r[9] for r in out)))
# sanity: cvar bool count sanity check
bools = [r[0] for r in out if r[7] == 'bool']
print(f"\nbool cvars: {len(bools)}; sample: {bools[:12]}")
