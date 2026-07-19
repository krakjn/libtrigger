# Tests

Native integration tests for file event delivery on Linux, macOS, and Windows.

```
tests/
├── common/
│   ├── harness.c      # polling helpers, temp paths, runner
│   └── harness.h
├── cases/
│   ├── test_modified.c
│   ├── test_deleted.c
│   └── test_created.c   # platform #ifdefs for watch target / expectations
└── main.c               # registers cases and runs the suite
```

| Test | Verifies |
|------|----------|
| `modified` | Append to a watched file fires `TRIGGER_EVENT_MODIFIED` |
| `deleted` | Removing a watched file fires `TRIGGER_EVENT_DELETED` |
| `created` | Creating a file under a watch fires `TRIGGER_EVENT_CREATED` |

Platform notes in `test_created.c`:

- **Linux** — watches the parent directory (inotify).
- **macOS** — watches the directory; accepts `CREATED` or `MODIFIED` (kqueue).
- **Windows** — watches the future file path before creation (directory filter).

## Run

```bash
zig build test
```

Or via the helper script:

```bash
./tests/run_tests.sh
```

In Docker (requires `just img` first):

```bash
just test-linux
```

The test binary is `zig-out/bin/events_test`.

CI runs the suite on each OS via [`.github/workflows/platforms.yml`](../.github/workflows/platforms.yml).
