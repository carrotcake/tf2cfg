#!/usr/bin/env python3
"""
Shared io for the res/cfg/allcmds.csv tooling. The canonical file is
./res/cfg/allcmds.csv (10 cols, see res/cfg/schema.md); write() rotates the
previous canonical copy to allcmds_bak.csv (local backup, not committed).
Run scripts from the repo root: data paths are cwd-relative.
"""
import csv, os, shutil

CANON = './res/cfg/allcmds.csv'
BAK = './res/cfg/allcmds_bak.csv'


def read():
    with open(CANON, encoding='utf-8', newline='') as f:
        return list(csv.reader(f))


def write(rows):
    tmp = CANON + '.new'
    with open(tmp, 'w', newline='', encoding='utf-8') as f:
        csv.writer(f).writerows(rows)
    if os.path.exists(CANON):
        shutil.move(CANON, BAK)
    shutil.move(tmp, CANON)
