# Install on Linux

This is the first Linux adapter path for `backup-chassis`.

It uses:

- the portable backup / verify / dashboard runtime from the repo
- `systemd --user` for scheduling
- `systemd-inhibit` via a bundled adapter wrapper when available
- `notify-send` via a bundled adapter when available
- the HTML dashboard as the operator surface

It does not currently include a Linux tray integration.

## Requirements

- Linux
- `bash`
- `python3`
- `curl`
- `restic`
- `systemctl`
- `systemd-analyze`

## Validate the host first

From the repo root:

```bash
bin/system3-backup doctor --portable-core
bin/system3-backup doctor --linux-adapter
```

## Bootstrap a local install surface

Run:

```bash
bin/system3-backup install --platform linux
```

For a non-writing install preview:

```bash
bin/system3-backup install --platform linux --plan
bin/system3-backup install --platform linux --plan --json
```

The install surface also exposes explicit backup-age thresholds:

```bash
bin/system3-backup install --platform linux \
  --warn-after-seconds 86400 \
  --critical-after-seconds 172800
```

Those default to `24h` warning and `48h` critical so a laptop or workstation
that sleeps, suspends, or shuts down overnight does not raise a false alarm
too early.

By default this creates:

- config: `~/.config/system3-backup/`
- state: `~/.local/state/system3-backup/`
- dashboard: `~/.local/share/system3-backup/index.html`
- systemd units: `~/.config/systemd/user/system3-backup.service` and `~/.config/systemd/user/system3-backup.timer`

The installer writes:

- `system3-backup.env`
- `paths.txt`
- `excludes.txt`
- a `systemd --user` service
- a `systemd --user` timer

The generated Linux env now points at bundled Linux adapter hooks:

- `SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT="$REPO_ROOT/bin/system3-backup-wrap-systemd-inhibit"`
- `SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT="$REPO_ROOT/bin/system3-backup-notify-linux"`

Those adapters keep Linux on the portable core plus `systemd` path while adding
the first step of local operational parity. If `systemd-inhibit` or
`notify-send` is unavailable, the bundled wrappers degrade quietly instead of
failing the backup run.

And validates:

- the generated unit files with `systemd-analyze verify`
- env/config coherence with `system3-backup-backup --check-config`

## Fill in machine-local config

Edit:

```bash
~/.config/system3-backup/system3-backup.env
~/.config/system3-backup/paths.txt
~/.config/system3-backup/excludes.txt
```

At minimum:

- set `RESTIC_REPOSITORY`
- set backend credentials
- set `RESTIC_PASSWORD`
- adjust `SYSTEM3_BACKUP_WARN_AFTER_SECONDS` and `SYSTEM3_BACKUP_CRITICAL_AFTER_SECONDS` if you want a different warning ladder
- list the paths you actually want to back up

## Re-check before enabling the timer

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup backup --check-config
```

## Enable the scheduler

```bash
systemctl --user daemon-reload
systemctl --user enable --now system3-backup.timer
```

Useful checks:

```bash
systemctl --user status system3-backup.timer
systemctl --user list-timers system3-backup.timer
```

## Dashboard

Generate the dashboard manually:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup dashboard
```

Then open:

```bash
~/.local/share/system3-backup/index.html
```

## CLI status

For a lightweight operator-facing status readout:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup status
```

For machine-readable status:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup status --json
```

## Scope

This is real Linux support for the scheduler + dashboard lane, but it is still
first-cut support:

- no Linux tray/menu integration yet
- no Matt dogfood deployment on Linux yet
- the portable core and the Linux adapter are now explicit, but the repo is still evolving
