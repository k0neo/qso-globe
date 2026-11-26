#define _POSIX_C_SOURCE 200809L

#include "filesystem.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void)
{
    char directory[] = "/tmp/qso-filesystem-XXXXXX";

    assert(mkdtemp(directory) != NULL);

    char first[4096];
    char second[4096];
    char path[4096];
    char blocker[4096];

    assert(snprintf(first, sizeof(first),
                    "%s/config", directory)
           < (int)sizeof(first));

    assert(snprintf(second, sizeof(second),
                    "%s/config/qso-globe", directory)
           < (int)sizeof(second));

    assert(snprintf(path, sizeof(path),
                    "%s/config/qso-globe/state",
                    directory)
           < (int)sizeof(path));

    assert(ensure_parent_directory(path) == 0);

    struct stat st;

    assert(stat(first, &st) == 0);
    assert(S_ISDIR(st.st_mode));

    assert(stat(second, &st) == 0);
    assert(S_ISDIR(st.st_mode));

    puts("Nested directory creation: PASS");

    assert((st.st_mode & 0777) == 0700);

    puts("Private directory permissions: PASS");

    assert(ensure_parent_directory(path) == 0);

    puts("Existing directory handling: PASS");

    assert(access(path, F_OK) == -1);
    assert(errno == ENOENT);

    puts("Parent-only creation: PASS");

    assert(snprintf(blocker, sizeof(blocker),
                    "%s/blocker", directory)
           < (int)sizeof(blocker));

    FILE *fp = fopen(blocker, "w");

    assert(fp != NULL);
    assert(fclose(fp) == 0);

    assert(snprintf(path, sizeof(path),
                    "%s/blocker/state", directory)
           < (int)sizeof(path));

    assert(ensure_parent_directory(path) == -1);
    assert(errno == ENOTDIR);

    puts("Non-directory path rejection: PASS");

    assert(ensure_parent_directory(NULL) == -1);
    assert(errno == EINVAL);

    assert(ensure_parent_directory("") == -1);
    assert(errno == EINVAL);

    puts("Invalid argument rejection: PASS");

    assert(ensure_parent_directory("state") == 0);

    puts("Current-directory path handling: PASS");

    assert(unlink(blocker) == 0);
    assert(rmdir(second) == 0);
    assert(rmdir(first) == 0);
    assert(rmdir(directory) == 0);

    puts("Test artifact cleanup: PASS");

    puts("\nAll filesystem regression tests passed.");

    return EXIT_SUCCESS;
}
