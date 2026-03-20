#!/usr/bin/env bash

SYSTEM3_BACKUP_ENV_SH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

system3_backup_format_interval_label() {
    local seconds="${1:-0}"
    if ! [[ "$seconds" =~ ^[0-9]+$ ]] || [[ "$seconds" -le 0 ]]; then
        echo "Scheduled"
        return
    fi

    if (( seconds % 86400 == 0 )); then
        local days=$((seconds / 86400))
        echo "Every ${days}d"
        return
    fi

    if (( seconds % 3600 == 0 )); then
        local hours=$((seconds / 3600))
        echo "Every ${hours}h"
        return
    fi

    if (( seconds % 60 == 0 )); then
        local minutes=$((seconds / 60))
        echo "Every ${minutes}m"
        return
    fi

    echo "Every ${seconds}s"
}

system3_backup_first_configured_path() {
    local path_file="$1"
    python3 - "$path_file" <<'PY'
import os
import sys

path_file = sys.argv[1]
try:
    with open(path_file) as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            print(os.path.expandvars(os.path.expanduser(line)))
            break
except FileNotFoundError:
    pass
PY
}

system3_backup_runtime_credential_vars() {
    local name=""
    if [[ -n "${SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS:-}" ]]; then
        printf '%s\n' RESTIC_REPOSITORY RESTIC_PASSWORD
        for name in $SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS; do
            [[ -z "$name" ]] && continue
            [[ "$name" == "RESTIC_REPOSITORY" || "$name" == "RESTIC_PASSWORD" ]] && continue
            printf '%s\n' "$name"
        done
        return
    fi

    printf '%s\n' RESTIC_REPOSITORY RESTIC_PASSWORD

    case "${RESTIC_REPOSITORY:-}" in
        s3:*)
            printf '%s\n' AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY
            ;;
        b2:*)
            printf '%s\n' B2_ACCOUNT_ID B2_ACCOUNT_KEY
            ;;
    esac
}

system3_backup_missing_runtime_credentials() {
    local name=""
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        if [[ -z "${!name:-}" ]]; then
            printf '%s\n' "$name"
        fi
    done < <(system3_backup_runtime_credential_vars)
}

system3_backup_require_runtime_credentials() {
    local missing=()
    local name=""
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        missing+=("$name")
    done < <(system3_backup_missing_runtime_credentials)

    if (( ${#missing[@]} > 0 )); then
        local missing_text="${missing[*]}"
        missing_text="${missing_text// /, }"
        printf 'Missing required environment variable(s): %s\n' "$missing_text" >&2
        return 1
    fi
}

system3_backup_load_env() {
    export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:$PATH"

    SYSTEM3_BACKUP_CONFIG_HOME="${SYSTEM3_BACKUP_CONFIG_HOME:-${XDG_CONFIG_HOME:-$HOME/.config}/system3-backup}"
    SYSTEM3_BACKUP_ENV_FILE="${SYSTEM3_BACKUP_ENV_FILE:-$SYSTEM3_BACKUP_CONFIG_HOME/system3-backup.env}"

    if [[ -f "$SYSTEM3_BACKUP_ENV_FILE" ]]; then
        set -a
        # shellcheck disable=SC1090
        source "$SYSTEM3_BACKUP_ENV_FILE"
        set +a
    fi

    export SYSTEM3_BACKUP_CONFIG_HOME
    export SYSTEM3_BACKUP_ENV_FILE
    export SYSTEM3_BACKUP_BIN_DIR="${SYSTEM3_BACKUP_BIN_DIR:-$SYSTEM3_BACKUP_ENV_SH_DIR}"
    export SYSTEM3_BACKUP_STATE_DIR="${SYSTEM3_BACKUP_STATE_DIR:-$HOME/.local/state/system3-backup}"
    export SYSTEM3_BACKUP_LOG_DIR="${SYSTEM3_BACKUP_LOG_DIR:-$HOME/.local/var/log/system3-backup}"
    export SYSTEM3_BACKUP_STATUS_DIR="${SYSTEM3_BACKUP_STATUS_DIR:-$SYSTEM3_BACKUP_STATE_DIR}"
    export SYSTEM3_BACKUP_STATUS_FILE="${SYSTEM3_BACKUP_STATUS_FILE:-$SYSTEM3_BACKUP_STATUS_DIR/status.json}"
    export SYSTEM3_BACKUP_SNAPSHOTS_CACHE_FILE="${SYSTEM3_BACKUP_SNAPSHOTS_CACHE_FILE:-$SYSTEM3_BACKUP_STATUS_DIR/snapshots-cache.json}"
    export SYSTEM3_BACKUP_FAIL_COUNT_FILE="${SYSTEM3_BACKUP_FAIL_COUNT_FILE:-$SYSTEM3_BACKUP_STATUS_DIR/consecutive-failures}"
    export SYSTEM3_BACKUP_PID_FILE="${SYSTEM3_BACKUP_PID_FILE:-${TMPDIR:-/tmp}/system3-backup.pid}"
    export SYSTEM3_BACKUP_VERIFY_PID_FILE="${SYSTEM3_BACKUP_VERIFY_PID_FILE:-${TMPDIR:-/tmp}/system3-backup-verify.pid}"
    export SYSTEM3_BACKUP_CANARY_FILE="${SYSTEM3_BACKUP_CANARY_FILE:-$SYSTEM3_BACKUP_CONFIG_HOME/restore-canary-do-not-edit.txt}"
    export SYSTEM3_BACKUP_PATHS_FILE="${SYSTEM3_BACKUP_PATHS_FILE:-$SYSTEM3_BACKUP_CONFIG_HOME/paths.txt}"
    export SYSTEM3_BACKUP_EXCLUDES_FILE="${SYSTEM3_BACKUP_EXCLUDES_FILE:-$SYSTEM3_BACKUP_CONFIG_HOME/excludes.txt}"
    export SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS="${SYSTEM3_BACKUP_REQUIRED_RUNTIME_ENV_VARS:-}"
    export SYSTEM3_BACKUP_SCHEDULE_INTERVAL_SECONDS="${SYSTEM3_BACKUP_SCHEDULE_INTERVAL_SECONDS:-14400}"
    export SYSTEM3_BACKUP_WARN_AFTER_SECONDS="${SYSTEM3_BACKUP_WARN_AFTER_SECONDS:-86400}"
    export SYSTEM3_BACKUP_CRITICAL_AFTER_SECONDS="${SYSTEM3_BACKUP_CRITICAL_AFTER_SECONDS:-172800}"
    export SYSTEM3_BACKUP_FAIL_ALERT_THRESHOLD="${SYSTEM3_BACKUP_FAIL_ALERT_THRESHOLD:-3}"
    export SYSTEM3_BACKUP_NETWORK_CHECK_URL="${SYSTEM3_BACKUP_NETWORK_CHECK_URL:-}"
    export SYSTEM3_BACKUP_KEEP_DAILY="${SYSTEM3_BACKUP_KEEP_DAILY:-7}"
    export SYSTEM3_BACKUP_KEEP_WEEKLY="${SYSTEM3_BACKUP_KEEP_WEEKLY:-4}"
    export SYSTEM3_BACKUP_KEEP_MONTHLY="${SYSTEM3_BACKUP_KEEP_MONTHLY:-12}"
    export SYSTEM3_BACKUP_LOG_RETENTION_DAYS="${SYSTEM3_BACKUP_LOG_RETENTION_DAYS:-30}"
    export SYSTEM3_BACKUP_ALLOW_MISSING_PATHS="${SYSTEM3_BACKUP_ALLOW_MISSING_PATHS:-0}"
    export SYSTEM3_BACKUP_USE_CAFFEINATE="${SYSTEM3_BACKUP_USE_CAFFEINATE:-1}"
    export SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT="${SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT:-}"
    export SYSTEM3_BACKUP_DUMP_BREWFILE="${SYSTEM3_BACKUP_DUMP_BREWFILE:-0}"
    export SYSTEM3_BACKUP_MACHINE_STATE_DIR="${SYSTEM3_BACKUP_MACHINE_STATE_DIR:-$SYSTEM3_BACKUP_CONFIG_HOME/machine-state}"
    export SYSTEM3_BACKUP_ENABLE_MACOS_NOTIFICATIONS="${SYSTEM3_BACKUP_ENABLE_MACOS_NOTIFICATIONS:-0}"
    export SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT="${SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT:-}"
    export SYSTEM3_BACKUP_VERIFY_SCRIPT="${SYSTEM3_BACKUP_VERIFY_SCRIPT:-}"
    export SYSTEM3_BACKUP_DASHBOARD_DIR="${SYSTEM3_BACKUP_DASHBOARD_DIR:-$SYSTEM3_BACKUP_STATE_DIR/dashboard}"
    export SYSTEM3_BACKUP_DASHBOARD_OUTPUT="${SYSTEM3_BACKUP_DASHBOARD_OUTPUT:-$SYSTEM3_BACKUP_DASHBOARD_DIR/index.html}"
    export SYSTEM3_BACKUP_DASHBOARD_ENTRYPOINT="${SYSTEM3_BACKUP_DASHBOARD_ENTRYPOINT:-}"
    export SYSTEM3_BACKUP_DISPLAY_NAME="${SYSTEM3_BACKUP_DISPLAY_NAME:-Backup}"
    export SYSTEM3_BACKUP_STORAGE_LABEL="${SYSTEM3_BACKUP_STORAGE_LABEL:-${RESTIC_REPOSITORY:-Configured object storage}}"
    export SYSTEM3_BACKUP_STORAGE_URL="${SYSTEM3_BACKUP_STORAGE_URL:-}"
    export SYSTEM3_BACKUP_SCHEDULER_LABEL="${SYSTEM3_BACKUP_SCHEDULER_LABEL:-configured scheduler}"
    export SYSTEM3_BACKUP_SCHEDULE_LABEL="${SYSTEM3_BACKUP_SCHEDULE_LABEL:-$(system3_backup_format_interval_label "${SYSTEM3_BACKUP_SCHEDULE_INTERVAL_SECONDS:-14400}")}"
    export SYSTEM3_BACKUP_SCHEDULER_SERVICE="${SYSTEM3_BACKUP_SCHEDULER_SERVICE:-}"
    export SYSTEM3_BACKUP_SCHEDULER_PLIST="${SYSTEM3_BACKUP_SCHEDULER_PLIST:-}"
    export SYSTEM3_BACKUP_MONITOR_LABEL="${SYSTEM3_BACKUP_MONITOR_LABEL:-healthchecks.io}"
    export SYSTEM3_BACKUP_ENCRYPTION_LABEL="${SYSTEM3_BACKUP_ENCRYPTION_LABEL:-restic repository encryption}"
    export SYSTEM3_BACKUP_CREDENTIALS_LABEL="${SYSTEM3_BACKUP_CREDENTIALS_LABEL:-local machine credentials}"
    export SYSTEM3_BACKUP_RUNTIME_ENTRYPOINT="${SYSTEM3_BACKUP_RUNTIME_ENTRYPOINT:-}"
    export SYSTEM3_BACKUP_BACKUP_PROCESS_MATCH="${SYSTEM3_BACKUP_BACKUP_PROCESS_MATCH:-restic backup}"
    export SYSTEM3_BACKUP_VERIFY_PROCESS_MATCH="${SYSTEM3_BACKUP_VERIFY_PROCESS_MATCH:-}"
    export SYSTEM3_BACKUP_CHECK_SUBSET="${SYSTEM3_BACKUP_CHECK_SUBSET:-1/16}"
    export SYSTEM3_BACKUP_CHECK_INTERVAL_DAYS="${SYSTEM3_BACKUP_CHECK_INTERVAL_DAYS:-7}"
    export SYSTEM3_BACKUP_RESTORE_INTERVAL_DAYS="${SYSTEM3_BACKUP_RESTORE_INTERVAL_DAYS:-30}"
    if [[ -z "$SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT" && "$SYSTEM3_BACKUP_USE_CAFFEINATE" == "1" ]]; then
        SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT="$SYSTEM3_BACKUP_BIN_DIR/system3-backup-wrap-caffeinate"
        export SYSTEM3_BACKUP_RUN_WRAPPER_SCRIPT
    fi
    if [[ -z "$SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT" && "$SYSTEM3_BACKUP_ENABLE_MACOS_NOTIFICATIONS" == "1" ]]; then
        SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT="$SYSTEM3_BACKUP_BIN_DIR/system3-backup-notify-macos"
        export SYSTEM3_BACKUP_FAILURE_NOTIFY_SCRIPT
    fi
    export SYSTEM3_BACKUP_SNAPSHOT_MATCH_PATH="${SYSTEM3_BACKUP_SNAPSHOT_MATCH_PATH:-}"
    if [[ -z "$SYSTEM3_BACKUP_SNAPSHOT_MATCH_PATH" && -f "$SYSTEM3_BACKUP_PATHS_FILE" ]]; then
        SYSTEM3_BACKUP_SNAPSHOT_MATCH_PATH="$(system3_backup_first_configured_path "$SYSTEM3_BACKUP_PATHS_FILE")"
        export SYSTEM3_BACKUP_SNAPSHOT_MATCH_PATH
    fi

    mkdir -p "$SYSTEM3_BACKUP_CONFIG_HOME" "$SYSTEM3_BACKUP_LOG_DIR" "$SYSTEM3_BACKUP_STATUS_DIR" "$SYSTEM3_BACKUP_DASHBOARD_DIR"
}

system3_backup_require_env() {
    local missing=()
    local name=""
    for name in "$@"; do
        if [[ -z "${!name:-}" ]]; then
            missing+=("$name")
        fi
    done

    if (( ${#missing[@]} > 0 )); then
        printf 'Missing required environment variable(s): %s\n' "${missing[*]}" >&2
        return 1
    fi
}

system3_backup_log() {
    local message="$1"
    echo "$(date '+%Y-%m-%d %H:%M:%S') — $message" | tee -a "$LOGFILE"
}

system3_backup_iso_utc_now() {
    date -u "+%Y-%m-%dT%H:%M:%SZ"
}

system3_backup_iso_utc_in_future() {
    local seconds="$1"
    python3 - "$seconds" <<'PY'
from datetime import datetime, timedelta, timezone
import sys

seconds = int(sys.argv[1])
target = datetime.now(timezone.utc).replace(microsecond=0) + timedelta(seconds=seconds)
print(target.isoformat().replace("+00:00", "Z"))
PY
}
