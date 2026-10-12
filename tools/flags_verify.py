#!/usr/bin/env python3
"""
Verify res/cfg/allcmds.csv flags against the dump sources:
1. every row's flags are a superset of the Linux dump tokens
2. rows filled from the CS2 dump (still empty in the v3 legacy snapshot)
3. cross-check non-empty flags vs CS2 evidence (expected version drift)
Read-only. Run from the repo root.
"""
import csv
from collections import Counter

from csvio import read

rows = read()

MAP = {'a':'archive','cl':'client','sv':'game','rep':'replicated','nf':'notify','nr':'norecord',
       'user':'userinfo','prot':'protected','sp':'singleplayer','print':'printonly',
       'neverprint':'numeric','server_can_execute':'svcanexec','clientcmd_can_execute':'clcmdcanexec'}
def norm(f):
    toks = [t.strip(',').strip().strip('"').strip(',') for t in f.split()]
    return set(MAP.get(t, t) for t in toks if t and t != ':')

dump = {}
for line in open('./data/cvarlist_linux.txt', encoding='utf-8', errors='replace').read().splitlines()[1:]:
    line = line.strip()
    if not line or line.startswith('cvar list') or line.startswith('----'): continue
    p = line.split(' : ')
    if len(p) < 3: continue
    dump[p[0].strip()] = norm(p[2].strip())

# any row whose flags are NOT a superset of dump flags?
bad = [(r[0], r[5], ' '.join(sorted(dump[r[0]]))) for r in rows if r[0] in dump and dump[r[0]] and not norm(r[5]) >= dump[r[0]]]
print(f"rows missing dump tokens: {len(bad)}")
for b in bad[:10]: print(' ', b)

# the CS2-filled rows (empty in v3 legacy, non-empty now)
old = list(csv.reader(open('./res/cfg/allcmds_v3_legacy.csv', encoding='utf-8')))
oldmap = {r[0]: r for r in old}
filled = [(r[0], r[1] or 'cvar', r[5]) for r in rows if not oldmap[r[0]][5] and r[5]]
print(f"\nfilled from CS2 ({len(filled)}):")
for n, t, f in filled: print(f"  {n} [{t}]: {f}")

# cross-check: existing non-empty flags vs CS2 evidence
import re
FCVAR_MAP = {'FCVAR_GAMEDLL':'game','FCVAR_CLIENTDLL':'client','FCVAR_HIDDEN':'hidden',
    'FCVAR_PROTECTED':'protected','FCVAR_SPONLY':'singleplayer','FCVAR_ARCHIVE':'archive',
    'FCVAR_NOTIFY':'notify','FCVAR_USERINFO':'userinfo','FCVAR_CHEAT':'cheat',
    'FCVAR_REPLICATED':'replicated','FCVAR_DEMO':'demo','FCVAR_DONTRECORD':'norecord',
    'FCVAR_NOT_CONNECTED':'notconnected','FCVAR_SERVER_CAN_EXECUTE':'svcanexec',
    'FCVAR_CLIENTCMD_CAN_EXECUTE':'clcmdcanexec','FCVAR_SERVER_CANNOT_QUERY':'svcannotquery',
    'FCVAR_PRINTABLEONLY':'printonly'}
cs2 = {}
for line in open('./data/cs2_cvarlist.md', encoding='utf-8', errors='replace').read().splitlines():
    m = re.match(r'^\s*([^\s|][^|]*?) *\| *([^|]*?) *\|', line)
    if not m: continue
    name = m.group(1).strip()
    if not name or name == 'Name' or name.startswith('-'): continue
    toks = {FCVAR_MAP[f] for f in re.findall(r'FCVAR_[A-Z_0-9]+', m.group(2)) if f in FCVAR_MAP}
    if toks: cs2[name] = toks

agree = diff = 0
diffs = []
for r in rows:
    if r[5] and r[0] in cs2:
        s = norm(r[5])
        if s == cs2[r[0]]: agree += 1
        else:
            diff += 1
            diffs.append((r[0], r[1] or 'cvar', r[5], ' '.join(sorted(cs2[r[0]]))))
print(f"\ncross-check vs CS2 (non-empty rows with CS2 evidence): agree={agree}, differ={diff}")
print("sample differences (TF2 value vs CS2 2023 value) — expected version drift:")
for d in diffs[:25]: print(f"  {d[0]} [{d[1]}]  tf2: '{d[2]}'  cs2: '{d[3]}'")
