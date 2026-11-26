#ifndef QSO_INPUT_H
#define QSO_INPUT_H

#include <stdint.h>

#define QSO_PORT 45832

typedef struct {
    uint64_t id;
    char callsign[32];
    char grid[9];
    double lat;
    double lon;
} qso_event;

int qso_input_open(unsigned short port);
int qso_input_receive(int fd, qso_event *event);
void qso_input_close(int fd);

#endif
