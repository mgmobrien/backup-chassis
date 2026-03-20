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
- local status surfaces for operator confidence
- periodic recovery proof: integrity checks and test restores

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
- Linux: `systemctl`, `systemd-analyze`; `systemd-inhibit`, `notify-send`, `xdg-open`, and `yad` are optional for the richer Linux adapter path, including the tray surface

## Quickstart

If you want the shortest honest path:

```bash
bin/system3-backup doctor --portable-core
bin/system3-backup install --plan
bin/system3-backup install
```

Then fill in:

- `~/.config/system3-backup/system3-backup.env`
- `~/.config/system3-backup/paths.txt`
- `~/.config/system3-backup/excludes.txt`

Then validate and run:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup backup --check-config

SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup backup

SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup verify --force
```

## Backblaze B2 example

Backblaze B2 is the current documented example because it is inexpensive,
simple, and works cleanly through restic's S3-compatible path.

Typical setup:

1. Create a B2 bucket dedicated to backups.
2. Create an application key scoped to that bucket.
3. Put the bucket endpoint in `RESTIC_REPOSITORY`.
4. Put the application key ID and application key in the S3-compatible env vars.
5. Set a strong `RESTIC_PASSWORD`.

Example env values:

```bash
RESTIC_REPOSITORY="s3:s3.us-west-000.backblazeb2.com/your-backup-bucket"
AWS_ACCESS_KEY_ID="your-b2-key-id"
AWS_SECRET_ACCESS_KEY="your-b2-application-key"
RESTIC_PASSWORD="your-long-restic-passphrase"
SYSTEM3_BACKUP_STORAGE_LABEL="Backblaze B2 (your-backup-bucket)"
SYSTEM3_BACKUP_STORAGE_URL="https://secure.backblaze.com/b2_buckets.htm"
```

If you use the native `b2:` backend instead, or any other restic backend, set
the backend-specific credential vars and declare any extra ones in
`SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS` so the status surfaces gate on the
right credentials.

## Notifications and operator surfaces

The chassis is meant to be quiet by default and only ask for attention when it
has a specific action to offer.

macOS:

- SwiftBar is the richer operator surface
- the generated wrapper points at `bin/system3-backup-swiftbar`
- local failure notifications are optional
- to enable them, set `SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT="$REPO_ROOT/bin/system3-backup-notify-macos"`

Linux:

- the shared CLI and HTML dashboard work today
- local notifications use `notify-send` through `bin/system3-backup-notify-linux`
- the first Linux tray/menu path uses `yad`
- `xdg-open` is used for opening the dashboard and latest log from tray actions

If you want the Linux tray surface, install the optional desktop packages first.
On Debian/Ubuntu that usually means:

```bash
sudo apt install yad libnotify-bin xdg-utils
```

Then generate the tray autostart entry:

```bash
bin/system3-backup install --platform linux --enable-tray
```

Or run the tray directly:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup tray --platform linux
```

## Rough Backblaze cost guide

At current Backblaze B2 pay-as-you-go pricing, the rough model is:

- storage: about `$6 / TB / month`
- downloads: free up to `3x` your average monthly stored data, then about `$0.01 / GB`
- first `10 GB` stored is free

Rough storage-only examples:

- `100 GB` stored: about `$0.60 / month`
- `500 GB` stored: about `$3.00 / month`
- `1 TB` stored: about `$6.00 / month`
- `2 TB` stored: about `$12.00 / month`

For a personal encrypted backup repo, storage cost is usually the main number.
Request/API costs are typically minor unless you are doing something unusually
chatty. Check the current official pricing before budgeting:
https://www.backblaze.com/cloud-storage/pricing

## Current status

This repository is now the canonical runtime for the extracted chassis.

Matt's live deployment already dogfoods the repo-backed backup, recovery-proof,
dashboard, and menubar runtimes through thin local wrappers and machine-local
config on macOS.

The repo now also contains a Linux adapter path for `systemd --user`
scheduling, the dashboard surface, and a first optional tray/menu surface, but
that Linux lane is not yet dogfooded by Matt.

The shared cross-platform operator surface is:

- `bin/system3-backup` as the public CLI front door
- `bin/system3-backup-status` for concise text or `--json` machine-readable status

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
- `docs/agents.md` — programmatic install path for Claude/Codex-style agents
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

- `bin/system3-backup`
- `bin/system3-backup-backup`
- `bin/system3-backup-dashboard`
- `bin/system3-backup-doctor`
- `bin/system3-backup-ci-check`
- `bin/system3-backup-ci-check-linux`
- `bin/system3-backup-install-linux`
- `bin/system3-backup-linux-tray`
- `bin/system3-backup-install-macos`
- `bin/system3-backup-linux-smoke-test`
- `bin/system3-backup-notify-linux`
- `bin/system3-backup-notify-macos`
- `bin/system3-backup-status`
- `bin/system3-backup-smoke-test`
- `bin/system3-backup-swiftbar`
- `bin/system3-backup-verify`
- `bin/system3-backup-env.sh`
- `bin/system3-backup-wrap-caffeinate`
- `bin/system3-backup-wrap-systemd-inhibit`

The runtime command namespace remains `system3-backup-*` for now so the
current live installation can keep dogfooding the extracted repo without a
second compatibility migration.

The backup runner now reads path and exclusion files instead of embedding
Matt's exact machine paths. The recovery-proof layer preserves the current trust
model. The dashboard and menubar surfaces now run from the repo while reading
local state files generated by the repo runtime.

There is now also a repo install bootstrap for macOS that can lay down a local
config surface, launchd plist, and SwiftBar wrapper without baking Matt-local
paths into the public chassis.

There is also now a first Linux bootstrap path that can lay down a local config
surface plus `systemd --user` service/timer files without pretending the Linux
operator surface is already as rich as the macOS one.

That Linux lane now also has bundled `systemd-inhibit` and `notify-send`
adapter hooks, so it can take a first step toward local operational parity
without pushing more platform conditionals into the portable core.

There is now also a first optional Linux tray/menu adapter built around `yad`
plus a generated desktop autostart entry for session startup.

The portable core no longer calls `caffeinate` or `osascript` directly.
Those behaviors now live behind explicit adapter-script hooks so the core can
stay platform-neutral while the macOS adapter keeps its richer operator
surface.

## Validate before trusting it

Before you point this at a real repository, run:

```bash
bin/system3-backup doctor --portable-core
bin/system3-backup doctor
bin/system3-backup doctor --linux-adapter
bin/system3-backup-ci-check
```

`system3-backup doctor --portable-core` checks the reusable runtime
prerequisites.

`system3-backup doctor` checks the full current macOS deployment assumptions.

`system3-backup-ci-check` runs shell syntax validation plus the repo smoke
test, which bootstraps a temp install surface and verifies that the generated
env, launchd plist, status surface, dashboard, and SwiftBar output cohere end
to end.

## Public CLI

The public CLI entrypoint is:

```bash
bin/system3-backup
```

That front door is what an agent installer should prefer when presenting and
executing setup, because it exposes the stable public actions instead of making
the human reason about the raw script list.

Useful commands:

```bash
bin/system3-backup help
bin/system3-backup doctor
bin/system3-backup install --plan
bin/system3-backup install --platform linux --plan --json
bin/system3-backup install --platform linux --enable-tray
bin/system3-backup install --warn-after-seconds 86400 --critical-after-seconds 172800
bin/system3-backup backup --check-config
bin/system3-backup status --json
bin/system3-backup tray --platform linux
bin/system3-backup verify --force
bin/system3-backup smoke-test
```

The install command is now programmatic:

- `--plan` prints what would be created without writing files
- `--json` emits machine-readable output for agents and wrappers
- `install` dispatches to the current platform by default, or accepts `--platform macos|linux`
- `--warn-after-seconds` defaults to `86400` (`24h`)
- `--critical-after-seconds` defaults to `172800` (`48h`)

Those backup-age thresholds are explicit because laptops sleep and shut down.
The default policy should stay quiet for normal offline gaps and only escalate
after a full day without a completed backup.

## For agents

If an agent is setting this up for a human, the clean flow is:

1. run `bin/system3-backup doctor --portable-core`
2. run `bin/system3-backup install --plan --json`
3. explain the outputs and next steps to the human
4. fill in `system3-backup.env`, `paths.txt`, and `excludes.txt`
5. run `bin/system3-backup backup --check-config`
6. enable the platform scheduler
7. run one real backup and one forced verification pass
8. confirm with `bin/system3-backup status --json`

There is a dedicated agent-facing doc at `docs/agents.md`.

## License

This repo is released under the MIT license. See [LICENSE](LICENSE).
