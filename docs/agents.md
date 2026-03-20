# Agent setup path

This repo is meant to be installable by a human or by an agent working on the
human's behalf.

The CLI is the front door. Agents should prefer that over calling the raw
installer scripts directly.

## Recommended flow

From the repo root:

```bash
bin/system3-backup doctor --portable-core
bin/system3-backup install --plan --json
```

That gives an agent a non-writing plan it can explain before anything touches
the machine.

After the human approves the shape:

```bash
bin/system3-backup install
```

Or for Linux with the tray surface:

```bash
bin/system3-backup install --platform linux --enable-tray
```

## Files an agent should explain

The generated install surface is intentionally explicit:

- `system3-backup.env` — repository target, credentials, labels, scheduling, thresholds
- `paths.txt` — backup roots
- `excludes.txt` — exclusions
- scheduler file(s) — `launchd` on macOS or `systemd --user` on Linux
- optional Linux tray autostart desktop entry

The agent should explain:

- what each output file is for
- whether it is being written or preserved
- which next steps still require the human to provide secrets or path choices

## Minimum post-install validation

After filling in config:

```bash
SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup backup --check-config

SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup backup

SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup verify --force

SYSTEM3_BACKUP_ENV_FILE="$HOME/.config/system3-backup/system3-backup.env" \
  bin/system3-backup status --json
```

An agent should not call the installation complete until:

- the config resolves cleanly
- one real backup has completed
- one verification pass has completed
- the status surface reflects that state honestly

## Human-facing explanations that matter

When helping a human, the agent should be explicit about:

- where the encrypted repository lives
- which credentials are required to restore it
- what the warning ladder is
- how often backup runs are scheduled
- what `integrity check` and `restore drill` mean
- what to click or run when action is actually required

## Agent-specific principle

This repo should not require a Claude-specific skill to be usable.

If you build a Claude Code skill around it, that skill should be a thin helper
over the public CLI and these docs, not a second hidden install path.
