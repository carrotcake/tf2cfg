#!/usr/bin/env python3
"""Diff a live TF2 `cvarlist` dump against res/cfg/allcmds.csv (docs/ROADMAP.md section 7).

    python tools/catalog_diff.py data/cvarlist.txt [--platform auto] [--out DIR]

Prints one count per category and writes the full lists to DIR (default
%TEMP%/catdiff). It reports only: the catalog changes now, as ever, land with a
test and in-game evidence, never straight out of this script.

Not covered, because a dump cannot answer them: the input signature (field 8,
authored in tools/field_dicts.py), the platform and the context columns.
"""
import collections, csv, os, re, sys

# Rows deliberately dropped from the catalog although the stale dump listed them.
# Each needs a reason, or it is not a known removal.
KNOWN_REMOVALS = {
    'grab_window': 'in-game "Unknown command" (docs/in-game-checklist.md, 2026-10-15)',
}

NAMES = collections.OrderedDict([
    ('type_variable_as_command', 'dump says cvar, catalog row reads as a command'),
    ('type_command_as_variable', 'dump says command, catalog row reads as a cvar'),
    ('invalid_type',             'catalog row whose type field is not "", cmd or ncmd'),
    ('cheat_differs',            'cheat flag differs (cvars only; dump records no command flags)'),
    ('description_differs',      'description differs'),
    ('dead_rows',                'in catalog, universal, absent from dump (dead-row candidates)'),
    ('dead_rows_other_platform', 'in catalog, platform-tagged, absent from dump (expected if the '
                                 'dump is from another platform - read the tag in the row)'),
    ('new_rows',                 'in dump, absent from catalog (new-row candidates)'),
])


def norm(s):
    return re.sub(r'\s+', ' ', s).strip().casefold()


def parse_dump(path):
    """name -> (value, flags, description). cvarlist prints `name : value : flags : desc`;
    commands carry `cmd` as the value, header and footer lines are skipped."""
    out, dups = collections.OrderedDict(), set()
    for line in open(path, encoding='utf-8', errors='replace'):
        line = line.rstrip('\r\n')
        if not line or line.startswith('cvar list') or set(line) <= {'-'}:
            continue
        parts = re.split(r'\s+:\s', line, maxsplit=3)
        if len(parts) < 4:
            continue  # the "NNNN total convars/concommands" footer
        name = parts[0].strip().lower()
        if name in out:
            dups.add(name)  # engine prints some names twice; the last wins
        out[name] = tuple(p.strip() for p in parts[1:])
    return out, dups


def main():
    argv = [a for a in sys.argv[1:] if not a.startswith('--')]
    opts = dict(a[2:].split('=', 1) for a in sys.argv[1:] if a.startswith('--') and '=' in a)
    dump_path = argv[0] if argv else 'data/tf2-cvarlist-10-26-gamedump-w64.txt'
    canon = opts.get('canon', 'res/cfg/allcmds.csv')
    out_dir = opts.get('out', os.path.join(os.environ.get('TEMP', '/tmp'), 'catdiff'))

    dump, dups = parse_dump(dump_path)
    rows = list(csv.reader(open(canon, encoding='utf-8', newline='')))
    cat = {r[0].lower(): r for r in rows}

    # Platform is not inferred: a Linux client registers gl_*/ToGL internals a Windows client
    # has not, and the Xbox rows ship nowhere on PC, so an absent row that carries a platform
    # tag gets its own bucket instead of being silently dropped from "absent".
    found = collections.OrderedDict((k, []) for k in NAMES)
    for name, (value, flags, desc) in dump.items():
        if name not in cat:
            if name not in KNOWN_REMOVALS:
                found['new_rows'].append(f'{name}: {value} : {flags} : {desc}')
            continue
        r = cat[name]
        if bool(r[1]) != (value == 'cmd'):
            key = 'type_variable_as_command' if value != 'cmd' else 'type_command_as_variable'
            found[key].append(f'{name}: catalog type="{r[1]}" dump value="{value}"')
        if desc and norm(desc) != norm(r[6]):
            found['description_differs'].append(f'{name}\n  catalog: {r[6]}\n  dump   : {desc}')
        # The dump records no flags for commands (res/cfg/flags.md), so only cvars can
        # contradict the cheat marker the UI shows.
        if not bool(r[1]) and ('cheat' in r[5].split()) != ('"cheat"' in flags):
            found['cheat_differs'].append(
                f'{name}: catalog flags="{r[5]}" (cheat={"cheat" in r[5].split()}) '
                f'dump flags="{flags}" (cheat={"\"cheat\"" in flags})')

    expected_absent = 0
    for name, r in cat.items():
        if r[1] not in ('', 'cmd', 'ncmd'):
            found['invalid_type'].append(f'{name}: type="{r[1]}" (schema: "", cmd or ncmd)')
        if name in dump or name in KNOWN_REMOVALS:
            continue
        # The live cvarlist omits hidden rows (104 of the 109 in this catalog) and never lists
        # undeclared commands (34 of 34), so those absences are expected, not candidates.
        if 'hidden' in r[5].split() or r[1] == 'ncmd':
            expected_absent += 1
            continue
        row = f'{name},{r[1]},{r[7]},{r[8]},{r[9]}'
        found['dead_rows' if r[8] == 'universal' else 'dead_rows_other_platform'].append(row)

    print(f'dump {dump_path}: {len(dump)} names{f" ({len(dups)} printed twice)" if dups else ""}; '
          f'catalog {canon}: {len(rows)} rows; {expected_absent} absent rows are hidden-flagged '
          f'or ncmd, which cvarlist never lists')
    os.makedirs(out_dir, exist_ok=True)
    for key, label in NAMES.items():
        hits = found[key]
        print(f'{len(hits):5d}  {label}')
        for line in hits[:5]:
            print(f'       | {line.splitlines()[0][:110]}')
        if len(hits) > 5:
            print(f'       | ... {len(hits) - 5} more')
        with open(os.path.join(out_dir, key + '.txt'), 'w', encoding='utf-8') as f:
            f.write('\n'.join(hits) + ('\n' if hits else ''))
    if KNOWN_REMOVALS:
        print('known removals (not reported): ' + ', '.join(sorted(KNOWN_REMOVALS)))
    print(f'full lists in {out_dir}')


if __name__ == '__main__':
    main()
