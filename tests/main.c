#include "harness.h"

int test_modified(void);
int test_deleted(void);
int test_created(void);

int main(void) {
#if defined(_WIN32)
    const char* platform = "windows";
#elif defined(__APPLE__)
    const char* platform = "macos";
#else
    const char* platform = "linux";
#endif

    const test_case_t cases[] = {
        { "modified", test_modified },
        { "deleted", test_deleted },
        { "created", test_created },
    };

    return run_tests(platform, cases, sizeof(cases) / sizeof(cases[0]));
}
