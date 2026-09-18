# Permanent archive

Backup answers "what if the disk dies." It runs on a schedule, keeps a
retention window, and you never think about it.

Archive answers "this is finished, get it off the machine for good." It runs
only when you ask, keeps everything forever, and writes down what left.

They are two modes of the same chassis: same `restic` engine, same config
loading, same status surface, same menubar. They never share a repository.

## Why a separate repository

The backup repository forgets old snapshots by policy. If you deleted a folder
locally and relied on the backup, that folder would age out of the backup
within the retention window. The archive has no retention, so deleting the
local copy after archiving is safe.

The runtime enforces this: `system3-backup archive` refuses to run if the
archive repository resolves to the same value as the backup repository.

## Configuration

Copy `config/system3-archive.env.example` to
`~/.config/system3-backup/system3-archive.env` and fill in a repository,
credentials, and password that are different from the backup's. On macOS,
read the secrets from Keychain rather than storing them in the file.

Keep a recovery copy of the archive password in your password manager. If
the machine is lost, Keychain goes with it and the archive is unreadable
without that password.

Validate:

```bash
bin/system3-backup archive check-config
```

## Commands

```bash
bin/system3-backup archive add <path> [--label <name>] [--tag <tag>] [--dry-run]
bin/system3-backup archive list [--json]
bin/system3-backup archive snapshots [--json]
bin/system3-backup archive restore <snapshot-id|label> <target-dir>
bin/system3-backup archive status [--json]
```

`add` uploads the path directly to the archive repository, with progress in
the status file and the menubar. It never removes the local copy. When the
upload completes it appends one line to the manifest:

```json
{"label": "old-projects-2026", "path": "/Users/me/Old Projects", "host": "laptop",
 "snapshot_id": "…", "repository": "…", "started_at": "…", "completed_at": "…",
 "files": 12345, "bytes": 42949672960, "data_added": 40000000000, "local_removed": false}
```

Deleting the local copy is a separate, deliberate step you take after reading
that line. The manifest is the record of what has left the machine; it is
plain JSON lines, so it is searchable and easy to keep in notes.

`restore` accepts either a snapshot id or a manifest label.

## Files

- `~/.config/system3-backup/system3-archive.env`, configuration
- `<status dir>/archive-status.json`, live state for the menubar and scripts
- `<status dir>/archive-manifest.jsonl`, the record of archived items
- `<log dir>/archive-<timestamp>.log`, one log per upload

## Operating pattern

1. Something is finished and large. A project export, an old checkout, a
   snapshot you kept for rollback and no longer need on disk.
2. `archive add` it with a label you will recognise in a year.
3. Read the manifest line. Optionally `archive snapshots` to see it in the
   repository.
4. Delete the local copy.
5. Months later, `archive restore <label> <dir>` if you ever need it back.

Nothing here runs on a schedule. Archiving is a decision; backup is a habit.
