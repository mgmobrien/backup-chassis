#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#ifndef SYSTEM3_BACKUP_ENV_FILE_PATH
#error "SYSTEM3_BACKUP_ENV_FILE_PATH must be bound at build time"
#endif

static volatile sig_atomic_t child_process_group = -1;

static void forward_signal(int signal_number) {
    sig_atomic_t process_group = child_process_group;
    if (process_group > 0) {
        (void)kill(-process_group, signal_number);
    }
}

static int install_signal_handler(int signal_number) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = forward_signal;
    sigemptyset(&action.sa_mask);
    return sigaction(signal_number, &action, NULL);
}

static int executable_path(char *buffer, size_t buffer_size) {
    uint32_t requested_size = (uint32_t)buffer_size;
    char unresolved[PATH_MAX];

    if (_NSGetExecutablePath(unresolved, &requested_size) != 0) {
        fprintf(stderr, "backup-chassis-runner: executable path exceeds PATH_MAX\n");
        return -1;
    }

    if (realpath(unresolved, buffer) == NULL) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot resolve executable path: %s\n",
            strerror(errno)
        );
        return -1;
    }

    return 0;
}

static int bundled_runtime_paths(
    char *runtime_directory,
    size_t runtime_directory_size,
    char *runtime_entrypoint,
    size_t runtime_entrypoint_size
) {
    static const char executable_suffix[] =
        "/Contents/MacOS/backup-chassis-runner";
    static const char resources_suffix[] = "/Contents/Resources/bin";
    static const char entrypoint_name[] = "/system3-backup-backup";
    char runner_path[PATH_MAX];
    size_t runner_length;
    size_t suffix_length;
    size_t app_path_length;
    int written;

    if (executable_path(runner_path, sizeof(runner_path)) != 0) {
        return -1;
    }

    runner_length = strlen(runner_path);
    suffix_length = strlen(executable_suffix);
    if (
        runner_length <= suffix_length ||
        strcmp(runner_path + runner_length - suffix_length, executable_suffix) != 0
    ) {
        fprintf(
            stderr,
            "backup-chassis-runner: executable is not inside the signed runner app\n"
        );
        return -1;
    }

    app_path_length = runner_length - suffix_length;
    written = snprintf(
        runtime_directory,
        runtime_directory_size,
        "%.*s%s",
        (int)app_path_length,
        runner_path,
        resources_suffix
    );
    if (written < 0 || (size_t)written >= runtime_directory_size) {
        fprintf(stderr, "backup-chassis-runner: runtime directory path is too long\n");
        return -1;
    }

    written = snprintf(
        runtime_entrypoint,
        runtime_entrypoint_size,
        "%s%s",
        runtime_directory,
        entrypoint_name
    );
    if (written < 0 || (size_t)written >= runtime_entrypoint_size) {
        fprintf(stderr, "backup-chassis-runner: runtime entrypoint path is too long\n");
        return -1;
    }

    return 0;
}

static int validate_runtime_entrypoint(const char *runtime_entrypoint) {
    struct stat entrypoint_stat;

    if (stat(runtime_entrypoint, &entrypoint_stat) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: bundled runtime is unavailable: %s\n",
            strerror(errno)
        );
        return -1;
    }

    if (!S_ISREG(entrypoint_stat.st_mode) || access(runtime_entrypoint, X_OK) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: bundled runtime is not an executable regular file\n"
        );
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    char runtime_directory[PATH_MAX];
    char runtime_entrypoint[PATH_MAX];
    char *child_argv[3];
    const char *mode = NULL;
    posix_spawnattr_t attributes;
    sigset_t blocked_signals;
    sigset_t previous_signal_mask;
    pid_t child_pid;
    int spawn_result;
    int wait_status;

    if (argc == 2 && strcmp(argv[1], "--check-config") == 0) {
        mode = argv[1];
    } else if (argc != 1) {
        fprintf(stderr, "backup-chassis-runner: command arguments are not accepted\n");
        return 64;
    }

    if (
        bundled_runtime_paths(
            runtime_directory,
            sizeof(runtime_directory),
            runtime_entrypoint,
            sizeof(runtime_entrypoint)
        ) != 0 ||
        validate_runtime_entrypoint(runtime_entrypoint) != 0
    ) {
        return 66;
    }

    if (setenv("SYSTEM3_BACKUP_BUNDLED_RUNTIME_DIR", runtime_directory, 1) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot bind bundled runtime: %s\n",
            strerror(errno)
        );
        return 70;
    }
    if (SYSTEM3_BACKUP_ENV_FILE_PATH[0] != '/') {
        fprintf(stderr, "backup-chassis-runner: bound machine config path is not absolute\n");
        return 70;
    }
    if (setenv("SYSTEM3_BACKUP_ENV_FILE", SYSTEM3_BACKUP_ENV_FILE_PATH, 1) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot bind machine config: %s\n",
            strerror(errno)
        );
        return 70;
    }

    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGTERM);
    sigaddset(&blocked_signals, SIGINT);
    sigaddset(&blocked_signals, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &previous_signal_mask) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot block supervisor signals: %s\n",
            strerror(errno)
        );
        return 70;
    }
    if (
        install_signal_handler(SIGTERM) != 0 ||
        install_signal_handler(SIGINT) != 0 ||
        install_signal_handler(SIGHUP) != 0
    ) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot install signal handlers: %s\n",
            strerror(errno)
        );
        return 70;
    }

    child_argv[0] = runtime_entrypoint;
    child_argv[1] = (char *)mode;
    child_argv[2] = NULL;

    if (posix_spawnattr_init(&attributes) != 0) {
        fprintf(stderr, "backup-chassis-runner: cannot initialize spawn attributes\n");
        return 70;
    }
    if (
        posix_spawnattr_setflags(
            &attributes,
            POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK
        ) != 0 ||
        posix_spawnattr_setpgroup(&attributes, 0) != 0 ||
        posix_spawnattr_setsigmask(&attributes, &previous_signal_mask) != 0
    ) {
        (void)posix_spawnattr_destroy(&attributes);
        fprintf(stderr, "backup-chassis-runner: cannot configure child process group\n");
        return 70;
    }

    spawn_result = posix_spawn(
        &child_pid,
        runtime_entrypoint,
        NULL,
        &attributes,
        child_argv,
        environ
    );
    (void)posix_spawnattr_destroy(&attributes);
    if (spawn_result != 0) {
        (void)sigprocmask(SIG_SETMASK, &previous_signal_mask, NULL);
        fprintf(
            stderr,
            "backup-chassis-runner: cannot start bundled runtime: %s\n",
            strerror(spawn_result)
        );
        return 70;
    }

    child_process_group = child_pid;
    if (sigprocmask(SIG_SETMASK, &previous_signal_mask, NULL) != 0) {
        fprintf(
            stderr,
            "backup-chassis-runner: cannot restore supervisor signal mask: %s\n",
            strerror(errno)
        );
        (void)kill(-child_pid, SIGTERM);
        return 70;
    }
    for (;;) {
        if (waitpid(child_pid, &wait_status, 0) >= 0) {
            break;
        }
        if (errno != EINTR) {
            fprintf(
                stderr,
                "backup-chassis-runner: wait failed: %s\n",
                strerror(errno)
            );
            (void)kill(-child_pid, SIGTERM);
            return 70;
        }
    }
    child_process_group = -1;

    if (WIFEXITED(wait_status)) {
        return WEXITSTATUS(wait_status);
    }
    if (WIFSIGNALED(wait_status)) {
        return 128 + WTERMSIG(wait_status);
    }

    return 70;
}
