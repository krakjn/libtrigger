# Get Triggered

A cross-platform C library for watching files and calling callbacks when events occur.

## Features

- **Cross-platform**: Linux (inotify), macOS (kqueue), Windows (`ReadDirectoryChangesW`)
- **Simple API**: Callback-based interface
- **Event types**: Modifications, creations, deletions
- **Blocking and non-blocking**: `trigger_recv` or `trigger_try_recv`

## Building

```bash
zig build
```

Cross-compile all library targets:

```bash
zig build cross
```

Static library:

```bash
zig build -Dstatic=true
```

Artifacts install to `zig-out/lib/` and `zig-out/include/trigger.h`.

## Examples

Four example programs live in [`examples/`](examples/). Build them with `zig build`; binaries are in `zig-out/bin/`.

| Binary | Pattern |
|--------|---------|
| `single_thread_one_watcher` | One thread, one file, blocking recv |
| `single_thread_multi_watcher` | One thread polls many watchers |
| `multi_thread_one_watcher` | Dedicated watcher thread + event queue (POSIX) |
| `multi_thread_multi_watcher` | One thread per watcher (POSIX) |

See [`examples/README.md`](examples/README.md) for usage.

```bash
zig-out/bin/single_thread_one_watcher /tmp/test.txt
# in another terminal:
echo hello >> /tmp/test.txt
```

## API Reference

See [`include/trigger.h`](include/trigger.h) for the full public API. The watcher type is opaque; the Zig implementation is `trigger_watcher` in [`src/watcher.zig`](src/watcher.zig).

### Functions

- `trigger_init(filepath, callback)` — create a watcher (returns pointer, or `NULL` on error)
- `trigger_start(watcher)` — start watching (`TRIGGER_OK` / `TRIGGER_ERROR`)
- `trigger_stop(watcher)` — stop watching (`TRIGGER_OK` / `TRIGGER_ERROR`)
- `trigger_try_recv(watcher)` — non-blocking poll (`TRIGGER_EVENT_*`, `TRIGGER_OK` if none, or `TRIGGER_ERROR`)
- `trigger_recv(watcher)` — blocking wait (`TRIGGER_EVENT_*` or `TRIGGER_ERROR`)
- `trigger_destroy(watcher)` — free the watcher (`TRIGGER_OK` / `TRIGGER_ERROR`)

### Results and event types

```c
typedef enum {
    TRIGGER_OK = 0,
    TRIGGER_ERROR = -1,
    TRIGGER_EVENT_MODIFIED = 1,
    TRIGGER_EVENT_CREATED = 2,
    TRIGGER_EVENT_DELETED = 3,
} TRIGGER_RESULT;
```

### Callback

```c
typedef void (*trigger_callback_t)(const char* filepath, int event_type);
```

The callback runs on the thread that called `trigger_try_recv` or `trigger_recv`. `event_type` is one of the `TRIGGER_EVENT_*` values.

## Thread safety

The library does **not** use internal locking. Each `trigger_watcher_t` must be driven from **one thread** for its entire lifetime.

| Pattern | Supported? |
|---------|------------|
| One thread owns one watcher | Yes |
| One thread polls many watchers (`trigger_try_recv` loop) | Yes |
| Different watchers on different threads (one watcher per thread) | Yes |
| Dedicated watcher thread; other threads consume events via your own queue | Yes (application pattern) |
| Two or more threads calling `trigger_*` on the **same** watcher | **No** |
| `trigger_destroy` while another thread is in `trigger_recv` | **No** |
| Calling `trigger_*` on the same watcher from inside its callback | **No** |

Example mapping:

1. **Single-thread, single watcher** → `single_thread_one_watcher`
2. **Single-thread, multi watcher** → `single_thread_multi_watcher`
3. **Multi-thread, single watcher** → `multi_thread_one_watcher` (only the watcher thread touches the API)
4. **Multi-thread, multi watcher** → `multi_thread_multi_watcher`

### Anti-patterns

**Two threads, one watcher** — undefined behavior; OS handles are not shared this way.

```c
// Thread A                          Thread B
trigger_recv(w);                     trigger_stop(w);  // race
```

**Destroy while waiting** — use-after-free risk; blocked `recv` is not woken up.

```c
// Thread A: trigger_recv(w);  (blocked)
// Thread B: trigger_destroy(w);
```

**Re-enter from callback** — callback runs on the waiter thread; nested calls deadlock or corrupt state.

```c
void on_change(const char* path, int type) {
    trigger_try_recv(watcher);  // do not do this
}
```

**Shared global watcher** — multiple threads calling `trigger_try_recv(same_w)` is not supported.

Full per-watcher thread safety (including safe destroy-during-wait) may be added in a future release; it is not in scope today.

## Platform support

| OS | Backend |
|----|---------|
| Linux | inotify |
| macOS | kqueue |
| Windows | `ReadDirectoryChangesW` (watches parent directory, filters by filename) |

## Error handling

| Value | Meaning |
|-------|---------|
| `TRIGGER_OK` | Success (`start` / `stop` / `destroy`), or no event yet (`try_recv`) |
| `TRIGGER_ERROR` | Error |
| `TRIGGER_EVENT_MODIFIED` / `CREATED` / `DELETED` | Event delivered (`try_recv` / `recv`); callback was also invoked |

## CI

| Workflow | Purpose |
|----------|---------|
| [`ci.yml`](.github/workflows/ci.yml) | Cross-compile all targets and build release packages |
| [`platforms.yml`](.github/workflows/platforms.yml) | Native build + integration tests on Linux, macOS, and Windows |

Run the same integration tests locally:

```bash
zig build test
```

See [`tests/`](tests/) for details.
