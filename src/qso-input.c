#define _POSIX_C_SOURCE 200809L

#include "qso-input.h"
#include "locator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int qso_input_open(unsigned short port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        return -1;

    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0 ||
        fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ||
        fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
        close(fd);
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK)
    };

    if (bind(fd, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int qso_input_receive(int fd, qso_event *event)
{
    char buf[256];

    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n < 0) {
        if (errno == EAGAIN ||
            errno == EWOULDBLOCK ||
            errno == EINTR)
            return 0;

        return -1;
    }

    if (n == 0 ||
        n >= (ssize_t)sizeof(buf) ||
        memchr(buf, '\0', (size_t)n))
        return -2;

    buf[n] = '\0';

    char *fields[4];
    fields[0] = buf;

    for (int i = 1; i < 4; i++) {

        char *p = strchr(fields[i-1], '\t');

        if (!p)
            return -2;

        *p = '\0';
        fields[i] = p + 1;
    }

    if (strcmp(fields[0], "QSO1") != 0 ||
        strchr(fields[3], '\t'))
        return -2;

    if (!*fields[1])
        return -2;

    for (const char *p = fields[1]; *p; p++)
        if (!isdigit((unsigned char)*p))
            return -2;

    errno = 0;

    char *end;

    uintmax_t id = strtoumax(fields[1], &end, 10);

    if (errno || *end || id == 0 ||
        id > UINT64_MAX)
        return -2;

    size_t call_len = strlen(fields[2]);

    if (call_len < 3 || call_len > 20)
        return -2;

    for (size_t i = 0; i < call_len; i++) {

        unsigned char c = fields[2][i];

        if (!(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') &&
            c != '/' && c != '-')
            return -2;
    }

    qso_event result = {0};

    if (!locator_to_coordinates(
            fields[3], &result.lat, &result.lon))
        return -2;

    result.id = (uint64_t)id;

    snprintf(result.callsign,
             sizeof(result.callsign),
             "%s", fields[2]);

    snprintf(result.grid,
             sizeof(result.grid),
             "%s", fields[3]);

    *event = result;

    return 1;
}

void qso_input_close(int fd)
{
    if (fd >= 0)
        close(fd);
}
