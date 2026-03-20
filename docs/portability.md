# Portability

## Current truth

`backup-chassis` is currently a macOS deployment chassis with a partially
portable runtime inside it.

That means:

- the public install story is macOS-only
- the live dogfood deployment is macOS-only
- the core backup and verification logic is more portable than the install and operator surfaces

The repo should stay honest about that distinction.

## Core vs adapters

The intended split is:

### Portable core

These components should be able to run without macOS-specific services:

- env loading and config resolution
- `restic` backup execution
- retention and status-file updates
- verification logic and restore canary proof
- dashboard rendering from state files
- generic config examples

Today that mostly maps to:

- `bin/system3-backup-env.sh`
- `bin/system3-backup-backup`
- `bin/system3-backup-verify`
- `bin/system3-backup-dashboard`

### macOS adapter

These components are explicitly tied to the current macOS deployment:

- `launchd` scheduling
- SwiftBar menubar integration
- `caffeinate`
- `osascript` notifications
- the `system3-backup-install-macos` bootstrap path

Today that mostly maps to:

- `bin/system3-backup-install-macos`
- `bin/system3-backup-swiftbar`
- `templates/launchd/com.system3.backup.plist.example`
- `templates/swiftbar/system3-backup.5m.sh.example`

## What portability does not mean

Portability does not mean:

- "claim Linux support because some scripts are bash"
- "add conditionals for every platform inside the current macOS scripts"
- "pretend Windows is near-term"

The right pattern is explicit adapters, not platform sprawl.

## Linux

Linux is the next plausible target, but not a current claim.

If Linux work starts, the intended shape is:

- keep the portable core contract stable
- add a Linux scheduler adapter, likely `systemd`
- decide separately whether the operator surface is tray, dashboard-only, or both
- keep Linux docs separate from the macOS bootstrap path

## Windows

Windows is not currently in scope.

The runtime is bash-first and the current operator/scheduler assumptions are
Unix-shaped. Windows should remain explicitly unsupported until there is a real
runtime and operator model for it.

## Immediate doctrine

Near-term work should:

- keep public docs honest that installation is macOS-only today
- reduce remaining Matt-local assumptions in the portable core
- move backend- and platform-specific behavior behind explicit config or adapter surfaces
- avoid baking new macOS details into the reusable core
