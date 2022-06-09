#ifndef BH2_CONNECTION_CLIENT_H
#define BH2_CONNECTION_CLIENT_H

#include <pch.h>

#include "../data/bhash.h"

#include <sys/socket.h>

typedef struct Client
{
    /*
        Unused.
        Originally used by Blackhole 1, using client's address and port to create unique hash identifier for hash map.
    */
    [[deprecated]]
    BHash uid;

    // For logging only.
    time_t connect_time;

    int fd;
    socklen_t addr_len;
    struct sockaddr_storage addr;
    uint16_t port;
} Client;

#endif // !BH2_CONNECTION_CLIENT_H
