# PhantomID Network System

PhantomID is a secure network communication system that implements anonymous account management and messaging capabilities. The system is designed with privacy and security at its core, enabling confidential communication while maintaining user anonymity.

## Project Overview

The project implements a robust client-server architecture utilizing OpenSSL for cryptographic operations and secure communication. The system manages anonymous accounts in a hierarchical tree structure, allowing for organized user relationships while preserving privacy.

## Project Structure

The codebase is organized into distinct modules for clarity and maintainability:

```
phantomid-with-tree/
├── bin/               # Compiled binaries and runtime libraries
├── obj/              # Compilation intermediates and object files
├── -p/               # Project-specific configuration files
├── main.c            # Program entry point and initialization
├── network.c         # Network communication implementation
├── network.h         # Network protocol definitions
├── phantomid.c       # Core functionality implementation
├── phantomid.h       # System interface definitions
├── Makefile          # Build configuration for Unix systems
├── Makefile.win      # Build configuration for Windows
└── README.md         # Project documentation
```

## System Requirements

The development environment requires specific tools based on your operating system:

### Windows Development Environment
- MinGW-w64 (with POSIX threads support) for C compilation
- OpenSSL development package (version 3.x) for cryptographic operations
- Windows 10 or later operating system
- Git for version control (recommended)

### Unix Development Environment
- GCC compiler suite
- OpenSSL development libraries and headers
- POSIX-compliant operating system
- Make build system

## Installation Guide

### Windows Environment Setup

1. Install MinGW-w64:
   - Navigate to https://mingw-w64.org/
   - Download the x86_64 architecture installer
   - During installation:
     - Architecture: x86_64
     - Threads: posix
     - Exception: seh
   - Add the installation's bin directory to your system PATH

2. Install OpenSSL:
   ```powershell
   winget install ShiningLight.OpenSSL.Dev
   ```
   After installation:
   - Copy required DLLs to your project's bin directory:
     - `libssl-3-x64.dll`
     - `libcrypto-3-x64.dll`

3. Configure Your Build:
   Edit Makefile.win to match your OpenSSL installation:
   ```makefile
   OPENSSL_DIR := C:/Program Files/OpenSSL-Win64
   ```

### Unix Environment Setup

Install the required development packages using your system's package manager:

For Debian-based systems:
```bash
sudo apt-get update
sudo apt-get install gcc make libssl-dev git
```

For RPM-based systems:
```bash
sudo dnf install gcc make openssl-devel git
```

## Build Instructions

### Windows Build Process

Open Command Prompt in the project directory:

```cmd
# Full build
mingw32-make -f Makefile.win

# Clean build artifacts
mingw32-make -f Makefile.win clean

# Debug build with symbols
mingw32-make -f Makefile.win debug

# Build and execute
mingw32-make -f Makefile.win run
```

### Unix Build Process

Open Terminal in the project directory:

```bash
# Standard build
make

# Remove build artifacts
make clean

# Debug build
make debug

# Build and execute
make run
```

## Usage Guide

The PhantomID system provides a command-line interface with several configuration options:

Start the server:
```bash
bin/phantomid [OPTIONS]
```

Configuration options:
```
-p, --port PORT    Specify listening port (default: 8888)
-v, --verbose      Enable detailed logging
-d, --debug        Enable debug mode
-h, --help         Display usage information
```

System defaults:
- Server port: 8888
- Maximum concurrent clients: 10
- Network buffer size: 1024 bytes

## Troubleshooting Guide

### Windows-Specific Issues

1. OpenSSL Integration Problems:
   - Verify OpenSSL installation in Program Files
   - Ensure DLLs are in the correct location
   - Check system PATH for OpenSSL binaries

2. Compilation Errors:
   - Confirm MinGW-w64 installation
   - Verify PATH includes MinGW-w64 binaries
   - Check for correct DLL placement

### Unix-Specific Issues

1. Library Dependencies:
   - Verify OpenSSL development files installation
   - Check system library paths
   - Ensure correct permissions

2. Build System Issues:
   - Clear previous builds with `make clean`
   - Verify GCC installation
   - Check for missing dependencies

## Security Considerations

The system implements several security measures:
- Anonymous account management
- Encrypted communication channels
- Secure message routing
- Protected user hierarchy

## License and Legal

This software is proprietary and confidential. All rights reserved.

## Support and Contact

For technical support or inquiries:
- Report issues through the project management system
- Contact the development team lead
- Refer to internal documentation for additional guidance

For security-related concerns, please contact the security team immediately.