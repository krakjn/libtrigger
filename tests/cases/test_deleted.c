#include "harness.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#define unlink _unlink
#define close _close
#else
#include <unistd.h>
#endif

int test_deleted(void) {
    char path[512];
    const int fd = make_temp_file_path(path, sizeof(path), "del");
    if (fd < 0) {
        perror("make_temp_file_path");
        return -1;
    }
    close(fd);

    file_watcher_t* watcher = trigger_create_watcher(path, on_file_change);
    if (!watcher || trigger_start_watching(watcher) != 0) {
        fprintf(stderr, "failed to watch %s for delete test\n", path);
        unlink(path);
        return -1;
    }

    if (unlink(path) != 0) {
        perror("unlink");
        trigger_destroy_watcher(watcher);
        return -1;
    }

    const int rc = wait_for_event(watcher, FILE_EVENT_DELETED, 5000);
    trigger_destroy_watcher(watcher);
    return rc;
}
