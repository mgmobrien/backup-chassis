# Install on Linux

This is the first Linux adapter path for `backup-chassis`.

It uses:

- the portable backup / verify / dashboard runtime from the repo
- `systemd --user` for scheduling
- `systemd-inhibit` via a bundled adapter wrapper when available
- `notify-send` via a bundled adapter when available
- the HTML dashboard as the operator surface
- an optional `yad` tray surface with a generated desktop autostart entry

## Requirements

- Linux
- `bash`
- `python3`
- `curl`
- `restic`
- `systemctl`
- `systemd-analyze`

Optional but recommended for the richer operator surface:

- `notify-send` (`libnotify-bin` on Debian/Ubuntu)
- `xdg-open` (`xdg-utils` on Debian/Ubuntu)
- `yad`

Example Debian/Ubuntu install:

```bash
sudo apt install restic libnotify-bin xdg-utils yad
```

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

To generate the optional tray autostart surface too:

```bash
bin/system3-backup install --platform linux --enable-tray
```

By default this creates:

- config: `~/.config/system3-backup/`
- state: `~/.local/state/system3-backup/`
- dashboard: `~/.local/share/system3-backup/index.html`
- systemd units: `~/.config/systemd/user/system3-backup.service` and `~/.config/systemd/user/system3-backup.timer`
- if `--enable-tray` is used: `~/.config/autostart/system3-backup-tray.desktop`

The installer writes:

- `system3-backup.env`
- `paths.txt`
- `excludes.txt`
- a `systemd --user` service
- a `systemd --user` timer

The generated Linux env now points at bundled Linux adapter hooks:

- `SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT="$REPO_ROOT/bin/system3-backup-wrap-systemd-inhibit"`
- `SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT="$REPO_ROOT/bin/system3-backup-notify-linux"`
- `SYSTEM3_BACKUP_LINUX_TRAY_POLL_SECONDS="300"`

Those adapters keep Linux on the portable core plus `systemd` path while adding
the first step of local operational parity. If `systemd-inhibit` or
`notify-send` is unavailable, the bundled wrappers degrade quietly instead of
failing the backup run.

And validates:

- the generated unit files with `systemd-analyze verify`
- env/config coherence with `system3-backup-backup --check-config`
- Linux tray shell syntax when `--enable-tray` is used

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

Backblaze B2 example:

```bash
RESTIC_REPOSITORY="s3:s3.us-west-000.backblazeb2.com/your-backup-bucket"
AWS_ACCESS_KEY_ID="your-b2-key-id"
AWS_SECRET_ACCESS_KEY="your-b2-application-key"
RESTIC_PASSWORD="your-long-restic-passphrase"
SYSTEM3_BACKUP_STORAGE_LABEL="Backblaze B2 (your-backup-bucket)"
SYSTEM3_BACKUP_STORAGE_URL="https://secure.backblaze.com/b2_buckets.htm"
```

If you want a monitor link in the status surfaces, also set:

```bash
SYSTEM3_BACKUP_HEALTHCHECKS_CHECK_URL="https://healthchecks.io/checks/your-uuid/details/"
```

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

## Tray surface

If you installed `yad`, `notify-send`, and `xdg-open`, you can run the Linux
tray surface directly:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup tray --platform linux
```

If you used `--enable-tray`, the installer already wrote an autostart desktop
entry for the current desktop session.

The tray surface offers:

- current status via tray icon + tooltip
- open dashboard
- run backup now
- run integrity check + restore drill
- open latest backup log

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

This is real Linux support for the scheduler + dashboard + tray lane, but it is
still first-cut support:

- no Matt dogfood deployment on Linux yet
- the portable core and the Linux adapter are now explicit, but the repo is still evolving
