# Growtopia HTTP Server Documentation

## Overview

This is a high-performance HTTP server built with the h2o library for handling Growtopia game traffic.

## Architecture

### Project Structure

```
├── include/growtopia/    # Public headers
│   ├── handlers.h        # HTTP request handlers
│   ├── listener.h        # Network listener interface
│   └── config/           # Configuration headers
├── src/                  # Implementation files
│   ├── main.c           # Application entry point
│   ├── handlers.c       # Handler implementations
│   └── listener.c       # Listener implementations
├── externals/           # Third-party dependencies (h2o)
├── tests/               # Unit and integration tests
└── docs/                # Documentation
```

## Components

### Handlers

The handlers module (`handlers.h`/`handlers.c`) provides HTTP request handling functionality:

- **register_handler**: Register a request handler for a specific path
- **chunked_test**: Test handler for chunked transfer encoding
- **reproxy_test**: Test handler for X-Reproxy-URL functionality
- **post_test**: Test handler for POST requests

### Listener

The listener module (`listener.h`/`listener.c`) manages the network listener:

- **listener_init**: Initialize the listener with h2o context
- **listener_start**: Start accepting connections
- **listener_stop**: Stop the listener and cleanup

## Building

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# The binary will be in bin/server
./bin/server
```

## Configuration

Server configuration is defined in `include/growtopia/config/server.h`.

## Testing

Tests are located in the `tests/` directory. To run tests:

```bash
# TODO: Add test commands once tests are implemented
```

## Contributing

1. Follow the code style defined in `.clang-format`
2. Run `clang-tidy` before submitting changes
3. Ensure all tests pass
4. Document new features in this documentation

## License

See [LICENSE](../LICENSE) file for details.
