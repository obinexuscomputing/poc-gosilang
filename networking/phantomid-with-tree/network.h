#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

// Network Constants
#define NET_MAX_CLIENTS 10
#define NET_BUFFER_SIZE 1024
#define NET_MAX_BACKLOG 5
#define NET_TIMEOUT_SEC 1
#define NET_TIMEOUT_USEC 0

// Network Error Codes
typedef enum {
    NET_SUCCESS = 0,
    NET_ERROR_SOCKET = -1,
    NET_ERROR_BIND = -2,
    NET_ERROR_LISTEN = -3,
    NET_ERROR_ACCEPT = -4,
    NET_ERROR_SEND = -5,
    NET_ERROR_RECEIVE = -6,
    NET_ERROR_MEMORY = -7,
    NET_ERROR_INVALID = -8
} NetworkError;

// Network Protocol Types
typedef enum {
    NET_TCP,            // TCP protocol
    NET_UDP,            // UDP protocol
    NET_RAW,           // Raw sockets
    NET_PROTOCOL_MAX   // Protocol count
} NetworkProtocol;

// Network Role Types
typedef enum {
    NET_CLIENT,        // Client role
    NET_SERVER,        // Server role
    NET_PEER,         // Peer-to-peer role
    NET_ROLE_MAX      // Role count
} NetworkRole;

// Network Mode Types
typedef enum {
    NET_BLOCKING,      // Blocking mode
    NET_NONBLOCKING,   // Non-blocking mode
    NET_MODE_MAX      // Mode count
} NetworkMode;

// Network Socket State
typedef enum {
    NET_STATE_CLOSED,      // Socket is closed
    NET_STATE_LISTENING,   // Server is listening
    NET_STATE_CONNECTED,   // Client is connected
    NET_STATE_ERROR       // Error state
} NetworkState;

// Forward declaration
typedef struct PhantomDaemon PhantomDaemon;

// Client Connection State
typedef struct {
    pthread_mutex_t lock;           // State mutex
    bool is_active;                 // Active flag
    int socket_fd;                  // Socket descriptor
    struct sockaddr_in addr;        // Client address
    NetworkState state;             // Connection state
    time_t connect_time;            // Connection timestamp
    uint64_t bytes_sent;           // Total bytes sent
    uint64_t bytes_received;       // Total bytes received
} ClientState;

// Network Endpoint
typedef struct {
    pthread_mutex_t lock;           // Endpoint mutex
    char address[INET_ADDRSTRLEN];  // IP address
    uint16_t port;                  // Port number
    NetworkProtocol protocol;       // Protocol type
    NetworkRole role;               // Endpoint role
    NetworkMode mode;               // Socket mode
    NetworkState state;             // Current state
    int socket_fd;                  // Socket descriptor
    struct sockaddr_in addr;        // Socket address
    PhantomDaemon* phantom;         // Phantom daemon reference
    struct {
        int backlog;                // Listen backlog
        bool reuse_addr;            // Address reuse
        bool keep_alive;            // Keep-alive
        int timeout_sec;            // Timeout seconds
        int timeout_usec;           // Timeout microseconds
    } options;
} NetworkEndpoint;

// Network Packet
typedef struct {
    pthread_mutex_t lock;           // Packet mutex
    void* data;                     // Packet data
    size_t size;                    // Data size
    uint32_t flags;                 // Packet flags
    struct {
        uint32_t sequence;          // Sequence number
        uint32_t ack;              // Acknowledgment
        time_t timestamp;          // Packet timestamp
        NetworkError error;        // Error code
    } header;
} NetworkPacket;

// Network Program
typedef struct {
    NetworkEndpoint* endpoints;      // Endpoint array
    size_t count;                   // Endpoint count
    ClientState clients[NET_MAX_CLIENTS]; // Client states
    pthread_mutex_t clients_lock;    // Clients mutex
    volatile bool running;           // Running flag
    struct {
        void (*on_receive)(NetworkEndpoint*, NetworkPacket*);  // Data handler
        void (*on_connect)(NetworkEndpoint*);                  // Connect handler
        void (*on_disconnect)(NetworkEndpoint*);               // Disconnect handler
    } handlers;
    PhantomDaemon* phantom;         // Phantom daemon reference
} NetworkProgram;

// Core Network Functions
bool net_init(NetworkEndpoint* endpoint);
void net_close(NetworkEndpoint* endpoint);
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
void net_run(NetworkProgram* program);

// Utility Functions
bool net_is_port_in_use(uint16_t port);
bool net_release_port(uint16_t port);
NetworkError net_get_last_error(void);
const char* net_error_string(NetworkError error);

#endif // NETWORK_H