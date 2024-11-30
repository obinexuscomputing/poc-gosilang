#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Network types
typedef enum {
    NET_TCP,
    NET_UDP,
    NET_RAW
} NetworkProtocol;

typedef enum {
    NET_CLIENT,
    NET_SERVER,
    NET_PEER
} NetworkRole;

typedef enum {
    NET_BLOCKING,
    NET_NONBLOCKING
} NetworkMode;

// Network structures
typedef struct {
    char address[INET_ADDRSTRLEN];
    uint16_t port;
    NetworkProtocol protocol;
    NetworkRole role;
    NetworkMode mode;
    int socket_fd;
    struct sockaddr_in addr;
} NetworkEndpoint;

typedef struct {
    void* data;
    size_t size;
    uint32_t flags;
} NetworkPacket;

typedef struct NetworkProgram {
    NetworkEndpoint* endpoints;
    size_t count;
    void (*on_receive)(NetworkEndpoint*, NetworkPacket*);
    void (*on_connect)(NetworkEndpoint*);
    void (*on_disconnect)(NetworkEndpoint*);
} NetworkProgram;

// Function declarations
bool net_init(NetworkEndpoint* endpoint);
bool net_connect(NetworkEndpoint* endpoint);
bool net_accept(NetworkEndpoint* server, NetworkEndpoint* client);
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet);
void net_close(NetworkEndpoint* endpoint);
void net_run(NetworkProgram* program);

#endif // NETWORK_H
