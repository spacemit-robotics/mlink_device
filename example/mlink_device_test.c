#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <mlink.h>

#define DANCE_DURATION_MS (120 * 1000)
#define PID_FILE_PATH "/tmp/mlink-device-test.pid"
#define LOG_FILE_PATH "/tmp/mlink-device-test.log"
#define STOP_TIMEOUT_MS 5000

static volatile sig_atomic_t g_exit_requested = 0;
static volatile int g_base_running = 0;
static volatile int g_stop_requested = 0;
static pthread_mutex_t g_base_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_base_thread;
static bool g_base_thread_valid = false;

struct motion_task {
    char direction[16];
    int duration_ms;
};

static void handle_signal(int sig) {
    (void)sig;
    g_exit_requested = 1;
}

static void *server_run_thread(void *arg) {
    mlink_server_run((mlink_server_t *)arg);
    return NULL;
}

static void sleep_ms(int ms) {
    if (ms <= 0) {
        return;
    }

    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (int64_t)(ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

static bool read_pid_file(pid_t *out_pid) {
    FILE *fp;
    int64_t pid;

    fp = fopen(PID_FILE_PATH, "r");
    if (!fp) {
        return false;
    }

    if (fscanf(fp, "%" SCNd64, &pid) != 1 || pid <= 0) {
        fclose(fp);
        return false;
    }

    fclose(fp);
    if (out_pid) {
        *out_pid = (pid_t)pid;
    }
    return true;
}

static bool process_is_running(pid_t pid) {
    if (pid <= 0) {
        return false;
    }

    if (kill(pid, 0) == 0) {
        return true;
    }
    return errno == EPERM;
}

static bool get_running_pid(pid_t *out_pid) {
    pid_t pid;

    if (!read_pid_file(&pid)) {
        return false;
    }

    if (process_is_running(pid)) {
        if (out_pid) {
            *out_pid = pid;
        }
        return true;
    }

    unlink(PID_FILE_PATH);
    return false;
}

static bool write_pid_file(void) {
    FILE *fp = fopen(PID_FILE_PATH, "w");
    if (!fp) {
        printf("Failed to write pid file %s: %s\n", PID_FILE_PATH, strerror(errno));
        return false;
    }

    fprintf(fp, "%" PRId64 "\n", (int64_t)getpid());
    fclose(fp);
    return true;
}

static void unlink_pid_file_if_current(void) {
    pid_t pid;

    if (read_pid_file(&pid) && pid == getpid()) {
        unlink(PID_FILE_PATH);
    }
}

static void install_signal_handlers(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
}

static bool redirect_stdio_to_log(void) {
    int log_fd = open(LOG_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    int null_fd = open("/dev/null", O_RDONLY);

    if (log_fd < 0) {
        return false;
    }

    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }

    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    if (log_fd > STDERR_FILENO) {
        close(log_fd);
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    return true;
}

static void join_base_thread_locked(void) {
    if (g_base_thread_valid) {
        pthread_join(g_base_thread, NULL);
        g_base_thread_valid = false;
    }
    g_base_running = 0;
    g_stop_requested = 0;
}

static void *base_motion_thread(void *arg) {
    struct motion_task task = *(struct motion_task *)arg;
    const int tick_ms = (strcmp(task.direction, "dance") == 0) ? 1000 : 50;
    int elapsed_ms = 0;

    free(arg);

    printf("[motion] async start: direction=%s duration_ms=%d\n",
            task.direction, task.duration_ms);

    while (elapsed_ms < task.duration_ms) {
        if (g_stop_requested) {
            printf("[motion] async stop requested\n");
            break;
        }
        if (strcmp(task.direction, "dance") == 0) {
            printf("[dance] ... %d/%d s\n", elapsed_ms / 1000, task.duration_ms / 1000);
        }
        sleep_ms(tick_ms);
        elapsed_ms += tick_ms;
    }

    printf("[motion] async done\n");
    g_base_running = 0;
    return NULL;
}

static void start_base_motion(const char *direction, int duration_ms) {
    struct motion_task *task;

    if (duration_ms <= 0) {
        duration_ms = 1;
    }

    pthread_mutex_lock(&g_base_mutex);

    g_stop_requested = 1;
    join_base_thread_locked();

    task = (struct motion_task *)malloc(sizeof(*task));
    if (!task) {
        pthread_mutex_unlock(&g_base_mutex);
        printf("[motion] failed to allocate task\n");
        return;
    }

    snprintf(task->direction, sizeof(task->direction), "%s", direction);
    task->duration_ms = duration_ms;

    g_stop_requested = 0;
    g_base_running = 1;

    if (pthread_create(&g_base_thread, NULL, base_motion_thread, task) != 0) {
        g_base_running = 0;
        free(task);
        printf("[motion] failed to create worker thread\n");
    } else {
        g_base_thread_valid = true;
    }

    pthread_mutex_unlock(&g_base_mutex);
}

static void request_stop_base_motion(void) {
    pthread_mutex_lock(&g_base_mutex);
    g_stop_requested = 1;
    join_base_thread_locked();
    pthread_mutex_unlock(&g_base_mutex);
}

static struct mlink_return_value move_forward_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[move_forward] start\n");
    start_base_motion("forward", 500);
    return mlink_return_string("move forward started");
}

static struct mlink_return_value move_backward_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[move_backward] start\n");
    start_base_motion("backward", 500);
    return mlink_return_string("move backward started");
}

static struct mlink_return_value turn_left_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[turn_left] start\n");
    start_base_motion("left", 500);
    return mlink_return_string("turn left started");
}

static struct mlink_return_value turn_right_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[turn_right] start\n");
    start_base_motion("right", 500);
    return mlink_return_string("turn right started");
}

static struct mlink_return_value stop_motion_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[stop_motion] stop\n");
    request_stop_base_motion();
    return mlink_return_string("motion stopped");
}

static struct mlink_return_value dance_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[dance] start (%ds)\n", DANCE_DURATION_MS / 1000);
    start_base_motion("dance", DANCE_DURATION_MS);
    return mlink_return_string("dance started");
}

static void register_motion_tool(
    mlink_server_t *server,
    const char *name,
    const char *description,
    mlink_tool_callback_t callback) {
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_tool_t *tool;

    if (!props) {
        printf("Failed to create property list for %s\n", name);
        return;
    }

    tool = mlink_tool_create(name, description, props, callback, NULL, false);
    mlink_property_list_destroy(props);

    if (!tool || !mlink_server_add_tool(server, tool)) {
        printf("Failed to register %s\n", name);
        if (tool) {
            mlink_tool_destroy(tool);
        }
    }
}

static void register_move_forward_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "move_forward",
        "用途：当用户要求机器人向前移动时使用，例如："
        "\"机器人前进\"、\"前进\"、\"向前\"、\"往前走\"。"
        "效果：启动一次约 500 ms 的向前短距离运动；会替换当前正在执行的底盘动作。"
        "不要用于持续导航、跳舞或停止动作。",
        move_forward_cb);
}

static void register_move_backward_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "move_backward",
        "用途：当用户要求机器人向后移动时使用，例如："
        "\"机器人后退\"、\"后退\"、\"向后\"、\"往后退\"。"
        "效果：启动一次约 500 ms 的向后短距离运动；会替换当前正在执行的底盘动作。"
        "不要用于持续导航、跳舞或停止动作。",
        move_backward_cb);
}

static void register_turn_left_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "turn_left",
        "用途：当用户要求机器人向左转向时使用，例如："
        "\"机器人向左转\"、\"左转\"、\"向左转\"、\"往左转\"。"
        "效果：启动一次约 500 ms 的向左短暂转向；会替换当前正在执行的底盘动作。"
        "不要用于持续导航、跳舞或停止动作。",
        turn_left_cb);
}

static void register_turn_right_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "turn_right",
        "用途：当用户要求机器人向右转向时使用，例如："
        "\"机器人向右转\"、\"右转\"、\"向右转\"、\"往右转\"。"
        "效果：启动一次约 500 ms 的向右短暂转向；会替换当前正在执行的底盘动作。"
        "不要用于持续导航、跳舞或停止动作。",
        turn_right_cb);
}

static void register_stop_motion_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "stop_motion",
        "用途：当用户要求机器人停止当前动作时使用，例如："
        "\"机器人停止动作\"、\"停止\"、\"停下\"、\"停止跳舞\"、\"别动\"。"
        "效果：立即停止当前运动，包括前进、后退、转向或跳舞。"
        "不要用于启动新的移动、转向或跳舞动作。",
        stop_motion_cb);
}

static void register_dance_tool(mlink_server_t *server) {
    register_motion_tool(
        server,
        "dance",
        "用途：当用户要求机器人跳舞时使用，例如："
        "\"机器人跳个舞\"、\"跳个舞\"、\"跳舞\"。"
        "效果：启动一次约 120 秒的跳舞动作；可通过停止动作中断。"
        "不要用于普通移动、转向或停止动作。",
        dance_cb);
}

static int run_device_server(void) {
    mlink_server_t *server;
    pthread_t server_thread;
    bool server_thread_valid = false;

    install_signal_handlers();
    server = mlink_server_init(TRANSPORT_TYPE_UNIX, "robot");
    if (!server) {
        printf("Failed to init mlink server 'lekiwi_motion'\n");
        return 1;
    }

    register_move_forward_tool(server);
    register_move_backward_tool(server);
    register_turn_left_tool(server);
    register_turn_right_tool(server);
    register_dance_tool(server);
    register_stop_motion_tool(server);
    if (!write_pid_file()) {
        request_stop_base_motion();
        mlink_server_destroy(server);
        pthread_mutex_destroy(&g_base_mutex);
        return 1;
    }

    printf("mlink_device_test registered tools: move_forward, move_backward, "
        "turn_left, turn_right, dance, stop_motion\n");
    printf("pid: %" PRId64 "\n", (int64_t)getpid());
    fflush(stdout);

    if (pthread_create(&server_thread, NULL, server_run_thread, server) != 0) {
        printf("Failed to create mlink server run thread\n");
        request_stop_base_motion();
        mlink_server_destroy(server);
        unlink_pid_file_if_current();
        pthread_mutex_destroy(&g_base_mutex);
        return 1;
    }
    server_thread_valid = true;

    while (!g_exit_requested) {
        sleep_ms(200);
    }

    if (server_thread_valid) {
        pthread_cancel(server_thread);
        pthread_join(server_thread, NULL);
    }
    request_stop_base_motion();
    mlink_server_destroy(server);
    unlink_pid_file_if_current();
    pthread_mutex_destroy(&g_base_mutex);
    return 0;
}

static int cmd_status(void) {
    pid_t pid;

    if (get_running_pid(&pid)) {
        printf("mlink_device_test is running: pid=%" PRId64 " log=%s\n", (int64_t)pid, LOG_FILE_PATH);
        return 0;
    }

    printf("mlink_device_test is not running\n");
    return 1;
}

static int cmd_start(void) {
    pid_t pid;
    pid_t child;

    if (get_running_pid(&pid)) {
        printf("mlink_device_test already running: pid=%" PRId64 " log=%s\n", (int64_t)pid, LOG_FILE_PATH);
        return 0;
    }

    child = fork();
    if (child < 0) {
        printf("Failed to fork: %s\n", strerror(errno));
        return 1;
    }

    if (child == 0) {
        if (setsid() < 0) {
            exit(1);
        }
        if (!redirect_stdio_to_log()) {
            exit(1);
        }
        exit(run_device_server());
    }

    for (int elapsed = 0; elapsed < 2000; elapsed += 100) {
        int status;
        pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            printf("mlink_device_test failed to start, see %s\n", LOG_FILE_PATH);
            return 1;
        }
        if (get_running_pid(&pid)) {
            printf("mlink_device_test started: pid=%" PRId64 " log=%s\n", (int64_t)pid, LOG_FILE_PATH);
            return 0;
        }
        sleep_ms(100);
    }

    printf("mlink_device_test start timed out, see %s\n", LOG_FILE_PATH);
    return 1;
}

static int cmd_stop(void) {
    pid_t pid;

    if (!read_pid_file(&pid)) {
        printf("mlink_device_test is not running\n");
        return 0;
    }

    if (!process_is_running(pid)) {
        unlink(PID_FILE_PATH);
        printf("Removed stale pid file\n");
        return 0;
    }

    if (kill(pid, SIGTERM) != 0) {
        printf("Failed to stop pid=%" PRId64 ": %s\n", (int64_t)pid, strerror(errno));
        return 1;
    }

    for (int elapsed = 0; elapsed < STOP_TIMEOUT_MS; elapsed += 100) {
        if (!process_is_running(pid)) {
            unlink(PID_FILE_PATH);
            printf("mlink_device_test stopped\n");
            return 0;
        }
        sleep_ms(100);
    }

    printf("Timed out stopping mlink_device_test pid=%" PRId64 "\n", (int64_t)pid);
    return 1;
}

static int cmd_restart(void) {
    int stop_result = cmd_stop();
    if (stop_result != 0) {
        return stop_result;
    }
    return cmd_start();
}

static void print_usage(const char *prog) {
    printf("Usage: %s [run|start|stop|restart|status]\n", prog);
    printf("  run      Run in foreground (default)\n");
    printf("  start    Run in background, log to %s\n", LOG_FILE_PATH);
    printf("  stop     Stop background or foreground process recorded in %s\n", PID_FILE_PATH);
    printf("  restart  Stop then start in background\n");
    printf("  status   Show process status\n");
}

int main(int argc, char **argv) {
    const char *cmd = (argc > 1) ? argv[1] : "run";

    if (strcmp(cmd, "run") == 0) {
        pid_t pid;
        if (get_running_pid(&pid) && pid != getpid()) {
            printf("mlink_device_test already running: pid=%" PRId64 "\n", (int64_t)pid);
            return 1;
        }
        return run_device_server();
    }
    if (strcmp(cmd, "start") == 0) {
        return cmd_start();
    }
    if (strcmp(cmd, "stop") == 0) {
        return cmd_stop();
    }
    if (strcmp(cmd, "restart") == 0) {
        return cmd_restart();
    }
    if (strcmp(cmd, "status") == 0) {
        return cmd_status();
    }
    if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
