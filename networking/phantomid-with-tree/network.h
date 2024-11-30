#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // For inet_ntop
#include <pthread.h>

// Network Constants
#define NET_MAX_CLIENTS 10
#define NET_BUFFER_SIZE 1024

// Forward declaration of PhantomDaemon
typedef struct PhantomDaemon PhantomDaemon;

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

// Thread-safe client state
typedef struct {
    pthread_mutex_t lock;
    bool is_active;
    int socket_fd;
    struct sockaddr_in addr;
} ClientState;

// Thread-safe endpoint structure
typedef struct {
    pthread_mutex_t lock;
    char address[INET_ADDRSTRLEN];
    uint16_t port;
    NetworkProtocol protocol;
    NetworkRole role;
    NetworkMode mode;
    int socket_fd;
    struct sockaddr_in addr;
    PhantomDaemon* phantom;
} NetworkEndpoint;

// Network packet with thread safety
typedef struct {
    void* data;
    size_t size;
    uint32_t flags;
    pthread_mutex_t lock;
} NetworkPacket;

// Thread-safe program state
typedef struct {
    NetworkEndpoint* endpoints;
    size_t count;
    ClientState clients[NET_MAX_CLIENTS];
    pthread_mutex_t clients_lock;
    volatile bool running;
    struct {
        void (*on_receive)(NetworkEndpoint*, NetworkPacket*);
        void (*on_connect)(NetworkEndpoint*);
        void (*on_disconnect)(NetworkEndpoint*);
    } handlers;
    PhantomDaemon* phantom;
} NetworkProgram;

// Core network functions
bool net_init(NetworkEndpoint* endpoint);
void net_close(NetworkEndpoint* endpoint);
int net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
void net_run(NetworkProgram* program);  // Changed to void return type

#endif // NETWORK_H