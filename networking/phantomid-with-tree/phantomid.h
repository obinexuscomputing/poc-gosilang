#ifndef PHANTOMID_H
#define PHANTOMID_H

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <assert.h>
#include <arpa/inet.h>
#include "network.h"

#define MAX_ACCOUNTS 1000
#define MAX_MESSAGE_SIZE 4096
#define MAX_CHILDREN 10

// Forward declarations
struct PhantomNode;
struct PhantomTree;
struct PhantomMessage;

typedef struct PhantomNode PhantomNode;
typedef struct PhantomTree PhantomTree;
typedef struct PhantomMessage PhantomMessage;

// PhantomID account structure
typedef struct {
    uint8_t seed[32];
    char id[65];
    uint64_t creation_time;
    uint64_t expiry_time;
    pthread_mutex_t lock;
} PhantomAccount;

// Message structure
struct PhantomMessage {
    char from_id[65];
    char to_id[65];
    char content[MAX_MESSAGE_SIZE];
    time_t timestamp;
};

// Tree node structure
struct PhantomNode {
    PhantomAccount account;
    struct PhantomNode* parent;
    struct PhantomNode** children;
    size_t child_count;
    size_t max_children;
    bool is_root;
    bool is_admin;
    pthread_mutex_t node_lock;
};

// Tree structure
struct PhantomTree {
    PhantomNode* root;
    size_t total_nodes;
    pthread_mutex_t tree_lock;
};

// Network handlers declaration
void phantom_on_client_data(NetworkEndpoint* endpoint, NetworkPacket* packet);
void phantom_on_client_connect(NetworkEndpoint* endpoint);
void phantom_on_client_disconnect(NetworkEndpoint* endpoint);

// PhantomID daemon state
typedef struct PhantomDaemon {
    NetworkProgram network;
    PhantomTree* tree;
    pthread_mutex_t state_lock;
    bool running;
} PhantomDaemon;

// Tree traversal callback type
typedef void (*TreeVisitor)(PhantomNode* node, void* user_data);

// Core functions
bool phantom_tree_init(PhantomDaemon* phantom);
void phantom_tree_cleanup(PhantomDaemon* phantom);
bool phantom_init(PhantomDaemon* phantom, uint16_t port);
void phantom_cleanup(PhantomDaemon* phantom);
void phantom_run(PhantomDaemon* phantom);

// Tree operations
PhantomNode* phantom_tree_insert(PhantomDaemon* phantom, const PhantomAccount* account, const char* parent_id);
bool phantom_tree_delete(PhantomDaemon* phantom, const char* id);
PhantomNode* phantom_tree_find(PhantomDaemon* phantom, const char* id);

// Tree traversal
void phantom_tree_bfs(PhantomDaemon* phantom, TreeVisitor visitor, void* user_data);
void phantom_tree_dfs(PhantomDaemon* phantom, TreeVisitor visitor, void* user_data);
void phantom_tree_print(const PhantomDaemon* phantom);

// Status queries
bool phantom_tree_has_root(const PhantomDaemon* phantom);
size_t phantom_tree_size(const PhantomDaemon* phantom);
size_t phantom_tree_depth(const PhantomDaemon* phantom);

// Message operations
bool phantom_message_send(PhantomDaemon* phantom, const char* from_id, const char* to_id, const char* content);
PhantomMessage* phantom_message_get(PhantomDaemon* phantom, const char* id, size_t* count);

// Utility functions
const char* phantom_get_error(void);
time_t phantom_get_time(void);

#endif // PHANTOMID_H