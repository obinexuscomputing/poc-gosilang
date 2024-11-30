#ifndef PHANTOMID_H
#define PHANTOMID_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "network.h"

// PhantomID account structure
typedef struct {
    uint8_t seed[32];          // Cryptographic seed
    char id[64];               // Anonymous ID
    uint64_t creation_time;    // Account creation timestamp
    uint64_t expiry_time;      // Account expiry timestamp
    pthread_mutex_t lock;      // Thread safety for account operations
} PhantomAccount;

// PhantomID daemon state
typedef struct {
    NetworkProgram network;    // Network program for handling connections
    PhantomAccount* accounts;  // Array of phantom accounts
    size_t account_count;      // Number of active accounts
    pthread_mutex_t state_lock;// Thread safety for daemon state
    bool running;             // Daemon running state
} PhantomDaemon;

// Function declarations
// Update the function declaration to include port
bool phantom_init(PhantomDaemon* daemon, uint16_t port);
void phantom_cleanup(PhantomDaemon* daemon);
bool phantom_create_account(PhantomDaemon* daemon, PhantomAccount* account);
bool phantom_delete_account(PhantomDaemon* daemon, const char* id);
void phantom_run(PhantomDaemon* daemon);

#endif // PHANTOMID_H
