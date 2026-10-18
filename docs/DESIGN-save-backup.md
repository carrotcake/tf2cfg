# Design: Save Backup Rollback

Status: **implemented** (`1201dff`, 2026-10-13); the failure path is pinned by the
engine test `failedBackupWriteKeepsTheOldRestorePoint`.

## Goal
Keep the old `.bak` restore point when a new backup cannot be written, while preserving the existing save behavior when it succeeds.

## Problem
`ConfigController::commitWrite` currently removes the old `.bak` before trying to copy the current target to that same name. A failed copy therefore destroys the only previous restore point, even though the main file was not changed yet.

## Chosen approach
Use two temporary files in the same directory:

1. copy the target to `<name>.bak.new`;
2. only if that succeeds, move the old `<name>.bak` to `<name>.bak.old`;
3. move the new copy into place as `<name>.bak`;
4. if the final swap fails, restore `<name>.bak.old` to `<name>.bak` and report the error.

## Why this shape
- The expensive/fragile operation is the copy; the swap is only metadata and happens after success.
- Same-directory temporary files keep the rename cheap and avoid cross-device link failures.
- The main file is untouched until backup succeeds, so rollback cost is one extra file rename.

## Failure semantics
- A failed copy changes nothing except possibly a leftover `.new` file, which is removed first on the next attempt.
- A failed swap leaves either the old or new backup at `.bak`.
- A successful save still continues through the same `QSaveFile` write path.

## Out of scope
- Filesystem-level durability/fsync.
- Atomic replacement of the target `.cfg` itself.