# Architecture

## Purpose

`System 3 Backup` is the reusable chassis for a local-first backup system.

It exists to extract the shareable substrate from a working local deployment without copying Matt's personal configuration into the public surface.

## Conceptual model

The system has five layers:

1. `restic` snapshot engine
2. storage backend
3. scheduler
4. status and monitoring surface
5. recoverability proof

In the current local deployment, those map to:

- `restic`
- Backblaze B2
- `launchd`
- SwiftBar plus an HTML dashboard
- periodic `restic check` plus restore canary verification

## Chassis vs bolt-ons

This repository should contain chassis, not bolt-ons.

Chassis:

- reusable shell scripts
- generic config loading
- scheduler template
- status-surface template
- backup verification pattern
- installation docs
- architecture docs

Bolt-ons:

- Matt's bucket name
- Matt's Keychain item names, if kept idiosyncratic
- Matt's exact backup path list
- Matt's healthchecks URL
- Matt's menu wording preferences
- Matt's internal ticket and operator notes

If a fact is specific to Matt's live machine, it does not belong in the public chassis unless it has been generalized into a configurable input.

## Source-of-truth split

This project should not duplicate the same facts across a public repo and an internal vault note at the same level of detail.

The intended boundary is:

- public repo owns reusable implementation and public docs
- internal vault owns local operator history and Matt-specific deployment context

Examples:

- how verification works in the reusable chassis: public repo
- which exact snapshot passed on Matt's machine today: internal vault
- generic launchd template: public repo
- Matt's loaded launch agent state and recent log output: internal vault

## Documentation roles

Within the public surface:

- `README.md` is the compact overview
- `docs/architecture.md` is the deeper reference description
- `docs/install-macos.md` is the first operator-facing setup path
- future install/runbook docs should explain setup and operation for a new user

Within the internal surface:

- the Matt Stack ticket log records execution history
- the backup skill remains the operator quick-reference for Matt
- internal reference notes describe the live local deployment and its decisions

## Public packaging direction

The likely packaging sequence is:

1. repo with docs, config examples, and extracted scripts
2. proven setup flow for one additional machine
3. optional local setup helper or wizard after real demand exists

The public name can be `System 3 Backup`.
The local operator name can remain `Restback`.

## Extraction status

The first extracted executable is the verification layer.

That is intentional. Verification is the piece most tightly coupled to the
system's trust claim: healthy should mean recent proof, not merely recent
backup activity.
