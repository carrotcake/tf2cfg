#!/usr/bin/env python3
"""
Flag (tags) pass:
1. Normalize dump short tokens to canonical wiki-style vocabulary.
2. Union cvar flags: v3 + Linux dump (both normalized).
3. Fill empty flags (cmd + cvar) from CS2 1.39.6.5 FCVAR dump (mapped, engine-only tokens).
Output: allcmds_v4.csv + conflict report.
"""
import csv, re, shutil
from collections import Counter

# ---------- canonical vocabulary ----------
TOK_MAP = {
    'a': 'archive', 'cl': 'client', 'sv': 'game', 'rep': 'replicated',
    'nf': 'notify', 'nr': 'norecord', 'user': 'userinfo', 'prot': 'protected',
    'sp': 'singleplayer', 'print': 'printonly', 'neverprint': 'numeric',
    'server_can_execute': 'svcanexec', 'clientcmd_can_execute': 'clcmdcanexec',
    'matsys': 'matsysthread',
}
# CS2 FCVAR -> canonical (engine flags that exist in TF2 vocabulary)
FCVAR_MAP = {
    'FCVAR_GAMEDLL': 'game',
    'FCVAR_CLIENTDLL': 'client',
    'FCVAR_HIDDEN': 'hidden',
    'FCVAR_PROTECTED': 'protected',
    'FCVAR_SPONLY': 'singleplayer',
    'FCVAR_ARCHIVE': 'archive',
    'FCVAR_NOTIFY': 'notify',
    'FCVAR_USERINFO': 'userinfo',
    'FCVAR_CHEAT': 'cheat',
    'FCVAR_REPLICATED': 'replicated',
    'FCVAR_DEMO': 'demo',
    'FCVAR_DONTRECORD': 'norecord',
    'FCVAR_NOT_CONNECTED': 'notconnected',
    'FCVAR_SERVER_CAN_EXECUTE': 'svcanexec',
    'FCVAR_CLIENTCMD_CAN_EXECUTE': 'clcmdcanexec',
    'FCVAR_SERVER_CANNOT_QUERY': 'svcannotquery',
    'FCVAR_PRINTABLEONLY': 'printonly',
}
# stable append order for unions/fills
ORDER = ['game','client','replicated','userinfo','archive','archivexbox','notify',
         'cheat','norecord','demo','hidden','protected','singleplayer','notconnected',
         'svcanexec','clcmdcanexec','svcannotquery','printonly','numeric','matsysthread']

def canon(tokens):
    out = []
    for t in tokens:
        t = TOK_MAP.get(t, t)
        if t not in out:
            out.append(t)
    out.sort(key=lambda t: ORDER.index(t) if t in ORDER else 99)
    return out

def norm_str(s):
    # tokens may carry comma and quote artifacts:  , "cl"
    toks = [t.strip(',').strip().strip('"').strip(',') for t in s.split()]
    toks = [t for t in toks if t and t != ':']
    return canon(toks)

# ---------- parse Linux dump ----------
dump_flags = {}
for line in open('./cvarlist_linux.txt', encoding='utf-8', errors='replace').read().splitlines()[1:]:
    line = line.strip()
    if not line or line.startswith('cvar list') or line.startswith('----'):
        continue
    parts = line.split(' : ')
    if len(parts) < 3:
        continue
    dump_flags[parts[0].strip()] = norm_str(parts[2].strip())

# ---------- parse CS2 dump ----------
text = open('./cs2_cvarlist.md', encoding='utf-8', errors='replace').read()
cs2 = {}
for line in text.splitlines():
    m = re.match(r'^\s*([^\s|][^|]*?) *\| *([^|]*?) *\|', line)
    if not m:
        continue
    name = m.group(1).strip()
    if not name or name == 'Name' or name.startswith('-'):
        continue
    fc = re.findall(r'FCVAR_[A-Z_0-9]+', m.group(2))
    toks = [FCVAR_MAP[f] for f in fc if f in FCVAR_MAP]
    if toks:
        cs2[name] = canon(toks)

print(f"CS2 evidence entries: {len(cs2)}")

# ---------- apply ----------
rows = list(csv.reader(open('./res/cfg/allcmds_v3.csv', encoding='utf-8')))
stats = Counter()
for r in rows:
    name, typ, flags = r[0], r[1], r[5]
    tokens = norm_str(flags)
    src = 'v3'
    d = dump_flags.get(name, [])
    new = list(tokens)
    if d and typ in ('', 'cvar'):          # dump evidence applies to cvars (cmds have none)
        for t in d:
            if t not in new:
                new.append(t); src += '+dump'
    if not new and name in cs2:            # fill empties from CS2
        new = list(cs2[name]); src = 'cs2-fill'
        stats['filled'] += 1
    elif new and name in cs2 and typ in ('', 'cvar') and set(cs2[name]) - set(new):
        pass                               # CS2 extras NOT unioned for non-empty (version drift)
    if ' '.join(new) != r[5]:
        r[5] = ' '.join(new)
        stats[src] += 1

print("\nchanges:", dict(stats))

# stats
empty_cmd = [r[0] for r in rows if r[1] in ('cmd','ncmd') and not r[5]]
empty_cvar = [r[0] for r in rows if r[1] == '' and not r[5]]
print(f"\nstill empty - cmd/ncmd: {len(empty_cmd)}, cvars: {len(empty_cvar)}")
print("still-empty cmds with CS2 candidates? none (already filled); sample cvars:", empty_cvar[:15])

with open('./res/cfg/allcmds_v4.csv', 'w', newline='', encoding='utf-8') as f:
    csv.writer(f).writerows(rows)
shutil.move('./res/cfg/allcmds_v3.csv', './res/cfg/allcmds_v3_legacy.csv')
print(f"\nWrote {len(rows)} rows to allcmds_v4.csv; v3 -> allcmds_v3_legacy.csv")

tok = Counter(t for r in rows for t in r[5].split())
print("\nfinal token counts:")
for t, n in tok.most_common():
    print(f"  {t:16s} {n}")
