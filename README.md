# Growtopia HTTP Server

A high-performance HTTP server built with H2O (C) for low latency and high throughput, designed for Growtopia game traffic.

## Project Structure

```
├── .devcontainer/         # GitHub Codespaces configuration
├── .github/               # GitHub Actions CI/CD workflows
├── .vscode/               # VS Code configuration
├── bin/                   # Compiled executables (created by build)
├── build/                 # Intermediate build files (created by build)
├── docs/                  # Documentation
├── include/               # Public header files
│   └── growtopia/        # Growtopia-specific headers
│       ├── handlers.h    # HTTP request handlers
│       ├── listener.h    # Network listener interface
│       └── config/       # Configuration headers
├── externals/             # Third-party libraries (h2o)
├── scripts/               # Build and maintenance scripts
├── src/                   # C source files
│   ├── main.c            # Application entry point
│   ├── handlers.c        # Handler implementations
│   └── listener.c        # Listener implementations
├── tests/                 # Unit and integration tests
├── .clang-format         # Code formatter configuration
├── .clang-tidy           # Static analyzer configuration
├── .gitignore
├── CMakeLists.txt        # Build configuration
├── compile_commands.json # Compilation database (symlink)
├── LICENSE
└── README.md
```

## Features

- High-performance HTTP/1.1 and HTTP/2 support via h2o
- Chunked transfer encoding
- X-Reproxy-URL support
- POST request handling
- Modular and maintainable codebase
- Comprehensive testing framework (in development)

## Quick Start

### Prerequisites

- CMake 3.10+
- GCC or Clang
- OpenSSL development libraries
- libuv (optional, for libuv backend)

### Building

```bash
# Quick setup and build
./scripts/setup.sh
./scripts/build.sh

# Or manually:
cmake -B build -S .
cmake --build build

# The binary will be in: bin/server
./bin/server
```

### Development

```bash
# Format code
./scripts/format.sh

# Run static analysis
./scripts/lint.sh

# VS Code users: Open the workspace and use the preconfigured tasks
# - Press Ctrl+Shift+B to build
# - Press F5 to debug
```

## Configuration

Server configuration is defined in [include/growtopia/config/server.h](include/growtopia/config/server.h).

## Documentation

For detailed documentation, see the [docs/](docs/) directory:

- [Architecture Overview](docs/README.md)
- API documentation (coming soon)
- Deployment guide (coming soon)

## Testing

```bash
# Build with tests enabled
cmake -DBUILD_TESTING=ON -B build -S .
cmake --build build

# Run tests
cd build && ctest
```

## Contributing

1. Follow the code style defined in [.clang-format](.clang-format)
2. Run `./scripts/format.sh` before committing
3. Run `./scripts/lint.sh` to check for issues
4. Ensure all tests pass
5. Update documentation as needed

## License

See [LICENSE](LICENSE) file for details.

## Acknowledgments

Built with [h2o](https://github.com/h2o/h2o) - an optimized HTTP server library.
