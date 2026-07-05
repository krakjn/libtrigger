#include "harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) {
    Sleep((DWORD)ms);
}
#else
#include <unistd.h>
static void sleep_ms(int ms) {
    usleep((useconds_t)ms * 1000);
}
#endif

static int g_last_event = 0;
static int g_callback_count = 0;

void on_file_change(const char* filepath, int event_type) {
    (void)filepath;
    g_last_event = event_type;
    g_callback_count++;
}

int wait_for_event(file_watcher_t* watcher, int expected, int timeout_ms) {
    g_last_event = 0;
    g_callback_count = 0;

    for (int elapsed = 0; elapsed < timeout_ms; elapsed += 10) {
        const int result = trigger_check_changes(watcher);
        if (result < 0) {
            fprintf(stderr, "trigger_check_changes failed\n");
            return -1;
        }
        if (result == 1 && g_last_event == expected) {
            return 0;
        }
        sleep_ms(10);
    }

    fprintf(
        stderr,
        "timeout waiting for %s, last event was %s (%d callbacks)\n",
        trigger_get_event_string(expected),
        trigger_get_event_string(g_last_event),
        g_callback_count
    );
    return -1;
}

int wait_for_any_event(file_watcher_t* watcher, const int* expected, size_t count, int timeout_ms) {
    g_last_event = 0;
    g_callback_count = 0;

    for (int elapsed = 0; elapsed < timeout_ms; elapsed += 10) {
        const int result = trigger_check_changes(watcher);
        if (result < 0) {
            fprintf(stderr, "trigger_check_changes failed\n");
            return -1;
        }
        if (result == 1) {
            for (size_t i = 0; i < count; i++) {
                if (g_last_event == expected[i]) {
                    return 0;
                }
            }
        }
        sleep_ms(10);
    }

    fprintf(
        stderr,
        "timeout waiting for event, last event was %s (%d callbacks)\n",
        trigger_get_event_string(g_last_event),
        g_callback_count
    );
    return -1;
}

int make_temp_file_path(char* path, size_t cap, const char* tag) {
#ifdef _WIN32
    char tmp_dir[MAX_PATH];
    if (GetTempPathA(sizeof(tmp_dir), tmp_dir) == 0) {
        return -1;
    }
    if (snprintf(path, cap, "%strigger-%s-XXXXXX", tmp_dir, tag) >= (int)cap) {
        return -1;
    }
#else
    if (snprintf(path, cap, "/tmp/trigger-%s-XXXXXX", tag) >= (int)cap) {
        return -1;
    }
#endif
    return mkstemp(path);
}

int make_temp_dir(char* path, size_t cap) {
#ifdef _WIN32
    char tmp_dir[MAX_PATH];
    if (GetTempPathA(sizeof(tmp_dir), tmp_dir) == 0) {
        return -1;
    }
    if (snprintf(path, cap, "%strigger-dir-XXXXXX", tmp_dir) >= (int)cap) {
        return -1;
    }
#else
    if (snprintf(path, cap, "/tmp/trigger-dir-XXXXXX") >= (int)cap) {
        return -1;
    }
#endif
    return mkdtemp(path) ? 0 : -1;
}

int run_tests(const char* platform, const test_case_t* cases, size_t count) {
    int failed = 0;

    for (size_t i = 0; i < count; i++) {
        printf("[%s] running %s... ", platform, cases[i].name);
        fflush(stdout);

        if (cases[i].run() == 0) {
            printf("ok\n");
        } else {
            printf("FAIL\n");
            failed++;
        }
    }

    if (failed != 0) {
        fprintf(stderr, "%d %s test(s) failed\n", failed, platform);
        return 1;
    }

    printf("all %zu %s file event tests passed\n", count, platform);
    return 0;
}
