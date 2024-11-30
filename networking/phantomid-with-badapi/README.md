# PhantomID

PhantomID is a thread-safe daemon that provides anonymous account creation and management. It uses cryptographic techniques to generate secure, anonymous identifiers and manages account lifecycles in a secure manner.

## Features

- Anonymous account creation using cryptographic seeds
- Thread-safe operation for multiple concurrent clients
- Account lifecycle management (creation, expiration, deletion)
- Secure networking with thread safety
- Command-line interface for port configuration
- Account expiration after 90 days
- Support for multiple concurrent client connections

## Prerequisites

```bash
# Install required packages (Ubuntu/Debian)
sudo apt-get install build-essential libssl-dev netcat-openbsd
```

## Building

```bash
gcc -o phantomid main.c phantomid.c network.c -pthread -lssl -lcrypto
```

## Usage

### Starting the Server

```bash
# Run with default port (8888)
./phantomid

# Run with specific port
./phantomid -p 8890

# Show help
./phantomid --help
```

### Command Line Options

```
Usage: ./phantomid [OPTIONS]
Options:
  -p, --port PORT    Port to listen on (default: 8888)
  -h, --help         Show this help message
```

### Connecting to the Server

Use netcat to connect to the server:
```bash
nc localhost 8888  # Replace 8888 with your chosen port
```

### Available Commands

Once connected, you can use these commands:

- `help` - Show available commands
- `create` - Create a new anonymous account
- `list` - List all active accounts
- `delete <id>` - Delete an account by ID
- `quit` - Disconnect from server

## Architecture

### Components

1. **Network Layer** (network.h, network.c)
   - Thread-safe network operations
   - Client connection management
   - Event-based architecture
   - Buffer management

2. **PhantomID Core** (phantomid.h, phantomid.c)
   - Account management
   - Cryptographic operations
   - State management
   - Command processing

3. **Main Program** (main.c)
   - Command-line parsing
   - Signal handling
   - Program lifecycle management

### Security Features

- OpenSSL for cryptographic operations
- Thread-safe data structures
- Mutex protection for shared resources
- Secure account ID generation
- No personal data storage

## Implementation Details

### Account Structure
```c
typedef struct {
    uint8_t seed[32];          // Cryptographic seed
    char id[64];               // Anonymous ID
    uint64_t creation_time;    // Account creation timestamp
    uint64_t expiry_time;      // Account expiry timestamp
    pthread_mutex_t lock;      // Thread safety
} PhantomAccount;
```

### Network Security
- TCP/IP protocol support
- Thread-safe client handling
- Protected socket operations
- Secure data transmission

### Thread Safety
- Mutex protection for shared resources
- Thread-safe client management
- Protected network operations
- Safe resource cleanup

## Examples

### Creating an Account
```bash
$ nc localhost 8888
> help
Available commands:
create - Create a new anonymous account
delete <id> - Delete an account by ID
list - List all active accounts
help - Show this help message
quit - Disconnect from server

> create
Account created:
ID: ec0023331918ab2406145538a72a5e5c79f9ff8d76164391c2c964e749698550
Creation Time: 1732921478
Expiry Time: 1740697478
```

### Listing Accounts
```bash
> list
Active accounts: 1
ID: ec0023331918ab2406145538a72a5e5c79f9ff8d76164391c2c964e749698550
Created: 1732921478
Expires: 1740697478
```

## Development

### Adding New Features
1. Implement feature in phantomid.c
2. Add necessary declarations to phantomid.h
3. Update network handling if required
4. Update command processing in on_client_data

### Running Tests
```bash
# Build the program
gcc -o phantomid main.c phantomid.c network.c -pthread -lssl -lcrypto

# Test basic functionality
./phantomid -p 8890

# In another terminal
nc localhost 8890
help
create
list
```

## Known Limitations

- IPv4 support only
- Fixed buffer sizes
- No persistent storage
- No authentication system
- 90-day fixed expiration

## Future Improvements

- Add IPv6 support
- Implement persistent storage
- Add account recovery mechanism
- Add custom expiration times
- Implement account metadata
- Add encryption for stored data

