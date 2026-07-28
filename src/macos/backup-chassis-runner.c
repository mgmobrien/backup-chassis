#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pwd.h>
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

#ifndef SYSTEM3_BACKUP_ENV_FILE_PATH
#error "SYSTEM3_BACKUP_ENV_FILE_PATH must be bound at build time"
#endif

static volatile sig_atomic_t child_process_group = -1;

static void forward_signal(int signal_number) {
    int saved_errno = errno;
    sig_atomic_t process_group = child_process_group;
    if (process_group > 0) {
        (void)kill(-process_group, signal_number);
    }
    errno = saved_errno;
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

static int format_environment_value(
    char *buffer,
    size_t buffer_size,
    const char *name,
    const char *value
) {
    int written = snprintf(buffer, buffer_size, "%s=%s", name, value);
    if (written < 0 || (size_t)written >= buffer_size) {
        fprintf(stderr, "backup-chassis-runner: environment value is too long: %s\n", name);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    char runtime_directory[PATH_MAX];
    char runtime_entrypoint[PATH_MAX];
    char home_environment[PATH_MAX + 6];
    char user_environment[256];
    char logname_environment[256];
    char temp_directory[PATH_MAX];
    char temp_environment[PATH_MAX + 8];
    char runtime_environment[PATH_MAX + 42];
    char config_environment[PATH_MAX + 34];
    char scheduler_environment[48];
    char *child_environment[10];
    char *child_argv[3];
    const char *mode = NULL;
    int self_check = 0;
    size_t temp_directory_size;
    posix_spawnattr_t attributes;
    sigset_t blocked_signals;
    sigset_t previous_signal_mask;
    struct passwd *account;
    pid_t child_pid;
    int spawn_result;
    int wait_status;

    if (argc == 2 && strcmp(argv[1], "--check-config") == 0) {
        mode = argv[1];
    } else if (argc == 2 && strcmp(argv[1], "--self-check") == 0) {
        self_check = 1;
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

    if (SYSTEM3_BACKUP_ENV_FILE_PATH[0] != '/') {
        fprintf(stderr, "backup-chassis-runner: bound machine config path is not absolute\n");
        return 70;
    }

    account = getpwuid(getuid());
    if (
        account == NULL ||
        account->pw_dir == NULL ||
        account->pw_dir[0] != '/' ||
        account->pw_name == NULL ||
        account->pw_name[0] == '\0'
    ) {
        fprintf(stderr, "backup-chassis-runner: cannot resolve the local user account\n");
        return 70;
    }

    temp_directory_size =
        confstr(_CS_DARWIN_USER_TEMP_DIR, temp_directory, sizeof(temp_directory));
    if (
        temp_directory_size == 0 ||
        temp_directory_size > sizeof(temp_directory) ||
        temp_directory[0] != '/'
    ) {
        memcpy(temp_directory, "/tmp", sizeof("/tmp"));
    }

    if (
        format_environment_value(
            home_environment,
            sizeof(home_environment),
            "HOME",
            account->pw_dir
        ) != 0 ||
        format_environment_value(
            user_environment,
            sizeof(user_environment),
            "USER",
            account->pw_name
        ) != 0 ||
        format_environment_value(
            logname_environment,
            sizeof(logname_environment),
            "LOGNAME",
            account->pw_name
        ) != 0 ||
        format_environment_value(
            temp_environment,
            sizeof(temp_environment),
            "TMPDIR",
            temp_directory
        ) != 0 ||
        format_environment_value(
            runtime_environment,
            sizeof(runtime_environment),
            "SYSTEM3_BACKUP_BUNDLED_RUNTIME_DIR",
            runtime_directory
        ) != 0 ||
        format_environment_value(
            config_environment,
            sizeof(config_environment),
            "SYSTEM3_BACKUP_ENV_FILE",
            SYSTEM3_BACKUP_ENV_FILE_PATH
        ) != 0 ||
        format_environment_value(
            scheduler_environment,
            sizeof(scheduler_environment),
            "SYSTEM3_BACKUP_LAUNCHED_BY_SCHEDULER",
            mode == NULL ? "1" : "0"
        ) != 0
    ) {
        return 70;
    }

    child_environment[0] = "PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin";
    child_environment[1] = home_environment;
    child_environment[2] = user_environment;
    child_environment[3] = logname_environment;
    child_environment[4] = temp_environment;
    child_environment[5] = "LANG=en_US.UTF-8";
    child_environment[6] = runtime_environment;
    child_environment[7] = config_environment;
    child_environment[8] = scheduler_environment;
    child_environment[9] = NULL;

    if (self_check) {
        printf("runner_executable=%s\n", argv[0]);
        printf("runtime_directory=%s\n", runtime_directory);
        printf("runtime_entrypoint=%s\n", runtime_entrypoint);
        printf("env_file=%s\n", SYSTEM3_BACKUP_ENV_FILE_PATH);
        return 0;
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
        child_environment
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
