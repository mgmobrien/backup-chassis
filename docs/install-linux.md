# Install on Linux

This is the first Linux adapter path for `backup-chassis`.

It uses:

- the portable backup / verify / dashboard runtime from the repo
- `systemd --user` for scheduling
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
bin/system3-backup-doctor --portable-core
bin/system3-backup-doctor --linux-adapter
```

## Bootstrap a local install surface

Run:

```bash
bin/system3-backup-install-linux
```

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
- list the paths you actually want to back up

## Re-check before enabling the timer

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup-backup --check-config
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
  bin/system3-backup-dashboard
```

Then open:

```bash
~/.local/share/system3-backup/index.html
```

## Scope

This is real Linux support for the scheduler + dashboard lane, but it is still
first-cut support:

- no Linux tray/menu integration yet
- no Matt dogfood deployment on Linux yet
- the portable core and the Linux adapter are now explicit, but the repo is still evolving
