#ifndef BH2_CONNECTION_CLIENT_H
#define BH2_CONNECTION_CLIENT_H

#include <pch.h>

#include <sys/socket.h>

typedef struct Client
{
    int socket_fd;
    socklen_t addr_len;
    struct sockaddr_storage addr;
    uint16_t port;
} Client;

#endif // !BH2_CONNECTION_CLIENT_H
