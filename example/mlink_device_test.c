#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <mlink.h>

static volatile int g_base_running = 0;
static volatile int g_stop_requested = 0;
static pthread_mutex_t g_base_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_base_thread;
static bool g_base_thread_valid = false;

struct motion_task {
    char direction[16];
    int duration_ms;
};

static void sleep_ms(int ms) {
    if (ms <= 0) {
        return;
    }

    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (int64_t)(ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
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

    printf("[base_move] async start: direction=%s duration_ms=%d\n",
            task.direction, task.duration_ms);

    while (elapsed_ms < task.duration_ms) {
        if (g_stop_requested) {
            printf("[base_move] async stop requested\n");
            break;
        }
        if (strcmp(task.direction, "dance") == 0) {
            printf("[dance] ... %d/%d s\n", elapsed_ms / 1000, task.duration_ms / 1000);
        }
        sleep_ms(tick_ms);
        elapsed_ms += tick_ms;
    }

    printf("[base_move] async done\n");
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
        printf("[base_move] failed to allocate task\n");
        return;
    }

    snprintf(task->direction, sizeof(task->direction), "%s", direction);
    task->duration_ms = duration_ms;

    g_stop_requested = 0;
    g_base_running = 1;

    if (pthread_create(&g_base_thread, NULL, base_motion_thread, task) != 0) {
        g_base_running = 0;
        free(task);
        printf("[base_move] failed to create worker thread\n");
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

static struct mlink_return_value base_move_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    const char *dir;

    (void)user_ctx;

    dir = mlink_property_list_get_string(props, "direction");
    if (!dir) {
        printf("[base_move] no 'direction' provided\n");
        return mlink_return_string("missing direction");
    }

    printf("[base_move] direction = %s\n", dir);

    if (strcmp(dir, "forward") == 0) {
        printf("  -> action: FORWARD\n");
        start_base_motion("forward", 500);
    } else if (strcmp(dir, "backward") == 0) {
        printf("  -> action: BACKWARD\n");
        start_base_motion("backward", 500);
    } else if (strcmp(dir, "left") == 0) {
        printf("  -> action: LEFT\n");
        start_base_motion("left", 500);
    } else if (strcmp(dir, "right") == 0) {
        printf("  -> action: RIGHT\n");
        start_base_motion("right", 500);
    } else if (strcmp(dir, "stop") == 0) {
        printf("  -> action: STOP\n");
        request_stop_base_motion();
        return mlink_return_string("base_move stopped");
    } else {
        printf("  -> unknown direction, no action\n");
        return mlink_return_string("unknown direction");
    }

    return mlink_return_string("base_move started");
}

static struct mlink_return_value dance_cb(
    const mlink_property_list_t *props,
    void *user_ctx) {
    (void)props;
    (void)user_ctx;

    printf("[dance] start (30s)\n");
    start_base_motion("dance", 30000);
    return mlink_return_string("dance started");
}

static void register_base_move_tool(mlink_server_t *server) {
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_tool_t *tool;

    if (!props) {
        printf("Failed to create property list for base_move\n");
        return;
    }

    if (!mlink_property_list_add_string(props, "direction", "stop")) {
        printf("Failed to configure base_move properties\n");
        mlink_property_list_destroy(props);
        return;
    }

    tool = mlink_tool_create(
        "base_move",
        "Control the mobile base. direction is one of: "
        "forward/backward/left/right (short ~500ms). "
        "Use stop to interrupt any running action.",
        props,
        base_move_cb,
        NULL,
        false);

    if (!tool || !mlink_server_add_tool(server, tool)) {
        printf("Failed to register base_move\n");
        if (tool) {
            mlink_tool_destroy(tool);
        } else {
            mlink_property_list_destroy(props);
        }
    }
}

static void register_dance_tool(mlink_server_t *server) {
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_tool_t *tool;

    if (!props) {
        printf("Failed to create property list for dance\n");
        return;
    }

    tool = mlink_tool_create(
        "dance",
        "Long-running dance action (~30s). Use base_move(direction=stop) to interrupt.",
        props,
        dance_cb,
        NULL,
        false);

    if (!tool || !mlink_server_add_tool(server, tool)) {
        printf("Failed to register dance\n");
        if (tool) {
            mlink_tool_destroy(tool);
        } else {
            mlink_property_list_destroy(props);
        }
    }
}

int main(void) {
    mlink_server_t *server;

    server = mlink_server_init(TRANSPORT_TYPE_UNIX, "robot");
    if (!server) {
        printf("Failed to init mlink server 'lekiwi_motion'\n");
        return 1;
    }

    register_base_move_tool(server);
    register_dance_tool(server);
    printf("mlink_device_test registered tools: base_move, dance\n");

    mlink_server_run(server);

    request_stop_base_motion();
    mlink_server_destroy(server);
    pthread_mutex_destroy(&g_base_mutex);
    return 0;
}
