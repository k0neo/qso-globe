#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "qso-input.h"
#include "qso-queue.h"
#include "locator.h"

static void send_packet(
    int fd,
    const struct sockaddr_in *destination,
    const void *data,
    size_t length)
{
    ssize_t sent = sendto(
        fd, data, length, 0,
        (const struct sockaddr *)destination,
        sizeof(*destination));

    assert(sent == (ssize_t)length);
}

static int receive_packet(int fd,
                          qso_event *event)
{
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN
    };

    int ready = poll(&pfd, 1, 2000);

    assert(ready == 1);
    assert(pfd.revents & POLLIN);

    return qso_input_receive(fd, event);
}

static void near_value(double actual,
                       double expected)
{
    assert(fabs(actual-expected) < 1.0e-8);
}

static void test_locator(void)
{
    double lat;
    double lon;

    assert(locator_to_coordinates(
        "AA00", &lat, &lon));

    near_value(lat, -89.5);
    near_value(lon, -179.0);

    assert(locator_to_coordinates(
        "JJ00", &lat, &lon));

    near_value(lat, 0.5);
    near_value(lon, 1.0);

    assert(locator_to_coordinates(
        "EN91JF", &lat, &lon));

    near_value(lat, 41.2291666666667);
    near_value(lon, -81.2083333333333);

    assert(locator_to_coordinates(
        "en91jf", &lat, &lon));

    near_value(lat, 41.2291666666667);
    near_value(lon, -81.2083333333333);

    assert(locator_to_coordinates(
        "AA00AA00", &lat, &lon));

    near_value(lat, -89.9979166666667);
    near_value(lon, -179.9958333333333);

    puts("Maidenhead coordinate conversion: PASS");

    const char *invalid[] = {
        "",
        "AA",
        "AA000",
        "AA00000",
        "SA00",
        "AS00",
        "AAAB",
        "AA00YA",
        "AA00AY",
        "AA00AAAB",
        "AA00AA000"
    };

    size_t count =
        sizeof(invalid)/sizeof(invalid[0]);

    for (size_t i = 0; i < count; i++)
        assert(!locator_to_coordinates(
            invalid[i], &lat, &lon));

    assert(!locator_to_coordinates(
        NULL, &lat, &lon));

    assert(!locator_to_coordinates(
        "AA00", NULL, &lon));

    assert(!locator_to_coordinates(
        "AA00", &lat, NULL));

    printf("Invalid locator rejection: "
           "PASS (%zu cases)\n", count+3);
}

static void test_network(void)
{
    int receiver = qso_input_open(0);

    assert(receiver >= 0);

    struct sockaddr_in destination = {0};

    socklen_t length = sizeof(destination);

    assert(getsockname(
        receiver,
        (struct sockaddr *)&destination,
        &length) == 0);

    assert(destination.sin_family == AF_INET);

    assert(destination.sin_addr.s_addr ==
           htonl(INADDR_LOOPBACK));

    assert(ntohs(destination.sin_port) != 0);
    assert(ntohs(destination.sin_port) != QSO_PORT);

    printf("Ephemeral UDP receiver: PASS "
           "(port %u)\n",
           (unsigned int)ntohs(destination.sin_port));

    assert(fcntl(receiver, F_GETFL) & O_NONBLOCK);
    assert(fcntl(receiver, F_GETFD) & FD_CLOEXEC);

    puts("Socket descriptor flags: PASS");

    qso_event event = {0};

    assert(qso_input_receive(
        receiver, &event) == 0);

    puts("Nonblocking empty receive: PASS");

    int sender = socket(AF_INET, SOCK_DGRAM, 0);

    assert(sender >= 0);

    const char valid[] =
        "QSO1\t900001\tKD8WCK\tEN91JF";

    send_packet(sender, &destination,
                valid, strlen(valid));

    assert(receive_packet(receiver, &event) == 1);

    assert(event.id == UINT64_C(900001));

    assert(strcmp(event.callsign,
                  "KD8WCK") == 0);

    assert(strcmp(event.grid,
                  "EN91JF") == 0);

    near_value(event.lat, 41.2291666666667);
    near_value(event.lon, -81.2083333333333);

    puts("Valid QSO datagram and decoded fields: PASS");

    const char maximum_id[] =
        "QSO1\t18446744073709551615\t"
        "K0TEST/P\tAA00AA00";

    send_packet(sender, &destination,
                maximum_id, strlen(maximum_id));

    assert(receive_packet(receiver, &event) == 1);

    assert(event.id == UINT64_MAX);

    puts("Maximum unsigned 64-bit QSO ID: PASS");

    const char *invalid[] = {
        "QSO2\t1\tK0TEST\tAA00",
        "QSO1\t0\tK0TEST\tAA00",
        "QSO1\t-1\tK0TEST\tAA00",
        "QSO1\t+1\tK0TEST\tAA00",
        "QSO1\t\tK0TEST\tAA00",
        "QSO1\t1x\tK0TEST\tAA00",
        "QSO1\t18446744073709551616\tK0TEST\tAA00",
        "QSO1\t1\tAB\tAA00",
        "QSO1\t1\tk0test\tAA00",
        "QSO1\t1\tBAD CALL\tAA00",
        "QSO1\t1\tABCDEFGHIJKLMNOPQRSTUVWXYZ\tAA00",
        "QSO1\t1\tK0TEST\tZZ99",
        "QSO1\t1\tK0TEST\tAA00YA",
        "QSO1\t1\tK0TEST",
        "QSO1\t1\tK0TEST\tAA00\textra",
        ""
    };

    size_t count =
        sizeof(invalid)/sizeof(invalid[0]);

    qso_event previous = event;

    for (size_t i = 0; i < count; i++) {

        send_packet(sender, &destination,
                    invalid[i], strlen(invalid[i]));

        assert(receive_packet(
            receiver, &event) == -2);

        assert(memcmp(&event, &previous,
                      sizeof(event)) == 0);
    }

    puts("Rejected packets preserve previous event: PASS");

    char oversized[512];

    memset(oversized, 'X', sizeof(oversized));

    send_packet(sender, &destination,
                oversized, sizeof(oversized));

    assert(receive_packet(receiver, &event) == -2);

    const char embedded_null[] =
        "QSO1\t1\tK0TEST\tAA00\0extra";

    send_packet(sender, &destination,
                embedded_null,
                sizeof(embedded_null)-1);

    assert(receive_packet(receiver, &event) == -2);

    printf("Malformed datagram rejection: "
           "PASS (%zu cases)\n", count+2);

    /*
     * Receiver must remain usable after
     * processing invalid traffic.
     */

    send_packet(sender, &destination,
                valid, strlen(valid));

    assert(receive_packet(receiver, &event) == 1);

    assert(event.id == UINT64_C(900001));

    puts("Receiver recovery after malformed input: PASS");

    assert(close(sender) == 0);

    qso_input_close(receiver);

    puts("Socket cleanup: PASS");
}

static void test_queue(void)
{
    qso_event event = {0};
    qso_event received = {0};

    assert(queue_pop(&received) == 0);

    event.id = 1;

    assert(queue_push(event) == 1);
    assert(queue_push(event) == 0);

    assert(queue_pop(&received) == 1);
    assert(received.id == 1);

    assert(queue_push(event) == 0);

    puts("Duplicate suppression: PASS");

    for (uint64_t i = 0; i < QUEUE_SIZE; i++) {

        event.id = 100+i;

        assert(queue_push(event) == 1);
    }

    event.id = 10000;

    assert(queue_push(event) == -1);

    puts("Queue capacity protection: PASS");

    for (uint64_t i = 0; i < QUEUE_SIZE; i++) {

        assert(queue_pop(&received) == 1);
        assert(received.id == 100+i);
    }

    assert(queue_pop(&received) == 0);

    puts("FIFO ordering and ring-buffer wrap: PASS");

    /*
     * A rejected event must not be entered
     * into the duplicate history.
     */

    event.id = 10000;

    assert(queue_push(event) == 1);
    assert(queue_pop(&received) == 1);
    assert(received.id == 10000);

    puts("Queue overflow recovery: PASS");

    /*
     * Duplicate history has a bounded lifetime.
     */

    for (uint64_t i = 0; i < SEEN_SIZE; i++) {

        event.id = 20000+i;

        assert(queue_push(event) == 1);
        assert(queue_pop(&received) == 1);
        assert(received.id == event.id);
    }

    event.id = 1;

    assert(queue_push(event) == 1);
    assert(queue_pop(&received) == 1);

    puts("Duplicate history rollover: PASS");
}

int main(void)
{
    printf("=== Maidenhead regression ===\n");

    test_locator();

    printf("\n=== UDP protocol regression ===\n");

    test_network();

    printf("\n=== QSO queue regression ===\n");

    test_queue();

    puts("\nAll network regression tests passed.");

    return EXIT_SUCCESS;
}
