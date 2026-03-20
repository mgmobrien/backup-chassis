# Backup chassis

`Backup chassis` is the extracted chassis for a file-first, restic-based
backup system.

Inside Matt's local system, the visible label is currently `Backup`.

## Current publication phase

This repository is being published first from Matt O'Brien's personal GitHub as
part of the personal-incubation lane.

That means:

- `Backup` is the active local visible label
- this repo is the personal publication of the reusable chassis
- it is not yet being presented as an official `System 3` company surface
- if it later clears the promotion gate, the company-facing framing would likely be `System 3 Backup`

## What it is

`Backup chassis` is a local-first backup chassis built around:

- `restic` for encrypted snapshots
- an operator-chosen `restic` backend, with S3-compatible object storage such as Backblaze B2 as the currently documented example
- a platform scheduler (`launchd` on macOS, `systemd --user` on Linux)
- healthchecks-style heartbeat monitoring
- a local status surface for operator confidence
- periodic integrity and restore-proof verification

The current public examples assume S3-compatible object storage or Backblaze
B2. Other `restic` backends can still work, but if they require additional env
vars you should declare those in `SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS` so
the status surfaces gate on the right credentials.

The intended promise is not merely "snapshots happen." The intended promise is:

- backups are encrypted and inspectable
- scheduling is explicit
- status is honest
- restores are periodically proven, not assumed

## Requirements

Today this chassis supports:

- `bash`
- `python3`
- `curl`
- `restic`

Adapter-specific requirements:

- macOS: `launchctl`, `plutil`; SwiftBar is optional if you want the menubar surface
- Linux: `systemctl`, `systemd-analyze`; there is no Linux tray surface yet

## Current status

This repository is now the canonical runtime for the extracted chassis.

Matt's live deployment already dogfoods the repo-backed backup, verification,
dashboard, and menubar runtimes through thin local wrappers and machine-local
config on macOS.

The repo now also contains a first Linux adapter path for `systemd --user`
scheduling plus the dashboard surface, but that Linux lane is not yet dogfooded
by Matt.

Those local operator surfaces still exist on Matt's machine under:

- `~/.local/bin/`
- `~/.local/etc/restic/`
- `~/.local/share/restic-dashboard/`
- `~/Library/LaunchAgents/`
- `~/Library/Application Support/SwiftBar/Plugins/`

Those local files are the live operator deployment. This repo is where the
reusable chassis is being extracted so it can become shareable without copying
Matt's exact machine state.

## Source boundaries

This project should not create two drifting records for the same facts.

The intended split is:

- public repo: canonical source for reusable code, templates, installation docs, and architecture docs
- internal vault: canonical source for Matt's local operator history, ticket logs, product decisions, and Matt-specific bolt-ons

That means:

- chassis behavior belongs here
- local bucket names, account names, healthchecks IDs, and personal path choices do not
- internal notes should point to this repo for code truth rather than restating the same implementation details

## Repository shape

- `docs/architecture.md` — role boundaries and source-of-truth split
- `docs/install-linux.md` — first Linux setup path
- `docs/install-macos.md` — setup guidance for a second macOS machine
- `docs/portability.md` — portable-core vs platform-adapter boundaries
- `config/` — example config surfaces for reusable deployments
- `templates/` — example `launchd`, `systemd`, and status-surface templates
- `bin/` — reusable scripts for backup system components

## Not yet in scope

This repo does not yet try to be:

- a hosted service
- a managed backup platform
- a polished universal cross-platform product
- a fully automated setup wizard

The current doctrine is: ship the file-first chassis first, and only build a
setup layer after real users clarify the happy path.

## Design principles

- file-first
- local-first
- inspectable
- no required SaaS control plane
- explicit chassis/bolt-on separation
- backup confidence should come from proof, not vibes

## Extracted now

The extracted runtime now includes:

- `bin/system3-backup-backup`
- `bin/system3-backup-dashboard`
- `bin/system3-backup-doctor`
- `bin/system3-backup-ci-check`
- `bin/system3-backup-ci-check-linux`
- `bin/system3-backup-install-linux`
- `bin/system3-backup-install-macos`
- `bin/system3-backup-linux-smoke-test`
- `bin/system3-backup-smoke-test`
- `bin/system3-backup-swiftbar`
- `bin/system3-backup-verify`
- `bin/system3-backup-env.sh`

The runtime command namespace remains `system3-backup-*` for now so the
current live installation can keep dogfooding the extracted repo without a
second compatibility migration.

The backup runner now reads path and exclusion files instead of embedding
Matt's exact machine paths. The verification layer preserves the current trust
model. The dashboard and menubar surfaces now run from the repo while reading
local state files generated by the repo runtime.

There is now also a repo install bootstrap for macOS that can lay down a local
config surface, launchd plist, and SwiftBar wrapper without baking Matt-local
paths into the public chassis.

There is also now a first Linux bootstrap path that can lay down a local config
surface plus `systemd --user` service/timer files without pretending the Linux
operator surface is already as rich as the macOS one.

The portable core no longer calls `caffeinate` or `osascript` directly.
Those behaviors now live behind explicit adapter-script hooks so the core can
stay platform-neutral while the macOS adapter keeps its richer operator
surface.

## Validate before trusting it

Before you point this at a real repository, run:

```bash
bin/system3-backup-doctor --portable-core
bin/system3-backup-doctor
bin/system3-backup-ci-check
```

`system3-backup-doctor --portable-core` checks the reusable runtime
prerequisites.

`system3-backup-doctor` checks the full current macOS deployment assumptions.

`system3-backup-ci-check` runs shell syntax validation plus the repo smoke
test, which bootstraps a temp install surface and verifies that the generated
env, launchd plist, dashboard, and SwiftBar output cohere end to end.

## License

This repo is released under the MIT license. See [LICENSE](LICENSE).
