# Server Load Testing Tool

A comprehensive testing tool for benchmarking and stress-testing the Growtopia HTTP Server.

## Features

### 1. **RPS (Requests Per Second) Test**

Measures the maximum throughput of the server by sending concurrent requests and calculating requests per second.

### 2. **Concurrent Connections Test**

Tests how the server handles multiple simultaneous connections over a sustained period.

### 3. **DDoS Protection Test**

Simulates a DDoS attack to verify that the security module correctly rate-limits and blocks excessive requests.

## Building

The tool is automatically built with the main project:

```bash
cmake --build build
```

Binary location: `dist/bin/server-test`

## Usage

```bash
./dist/bin/server-test [options]
```

### Options

| Option      | Description                                   | Default   |
| ----------- | --------------------------------------------- | --------- |
| `-h <host>` | Server hostname                               | localhost |
| `-p <port>` | Server port                                   | 8000      |
| `-u <path>` | URL path to test                              | /         |
| `-t <test>` | Test type: `rps`, `concurrent`, `ddos`, `all` | all       |
| `-n <num>`  | Number of requests (RPS test)                 | 1000      |
| `-c <num>`  | Number of concurrent connections              | 10        |
| `-d <sec>`  | Duration in seconds (concurrent test)         | 10        |
| `-r <rate>` | Attack rate for DDoS test (req/sec)           | 100       |
| `--help`    | Show help message                             | -         |

## Examples

### Basic RPS Test

Test with 10,000 requests using 50 concurrent threads:

```bash
./dist/bin/server-test -t rps -n 10000 -c 50
```

### Concurrent Connections Test

Test with 100 concurrent connections for 30 seconds:

```bash
./dist/bin/server-test -t concurrent -c 100 -d 30
```

### DDoS Protection Test

Simulate attack at 200 requests per second:

```bash
./dist/bin/server-test -t ddos -r 200
```

### Full Test Suite

Run all tests with default parameters:

```bash
./dist/bin/server-test -t all
```

### Test Remote Server

Test a remote server:

```bash
./dist/bin/server-test -h example.com -p 8080 -t all
```

## Output

### RPS Test Output

```
═══════════════════════════════════════════════════════════════
  RPS (Requests Per Second) Test
═══════════════════════════════════════════════════════════════
  Host: localhost:8000
  Path: /
  Threads: 50
  Requests: 10000

───────────────────────────────────────────────────────────────
  Test Results
───────────────────────────────────────────────────────────────
  Total Requests:      10000
  Successful:          10000 (100.0%)
  Failed:              0 (0.0%)
  Duration:            2.45 seconds
  Requests/sec:        4081.63
  Data transferred:    1250.00 KB
  Throughput:          510.20 KB/s

  Response Times:
    Average:           12.23 ms
    Min:               5.12 ms
    Max:               45.67 ms
───────────────────────────────────────────────────────────────
```

### DDoS Protection Test Output

```
═══════════════════════════════════════════════════════════════
  DDoS Protection Test
═══════════════════════════════════════════════════════════════
  Host: localhost:8000
  Path: /
  Attack Rate: 200 req/sec

  Progress: ..........XXXXXXXXXXXXXXXXXXXXXXXXXX

───────────────────────────────────────────────────────────────
  DDoS Protection Results
───────────────────────────────────────────────────────────────
  Duration:            10.05 seconds
  HTTP 200 (OK):       150
  HTTP 429 (Limited):  850
  Other Status:        0
  Connection Errors:   0
  Total Requests:      1000

  Protection Status:   ✓ ACTIVE (Rate limiting detected)
───────────────────────────────────────────────────────────────
```

**Legend:**

- `.` = Request succeeded (200 OK)
- `X` = Rate limited (429 Too Many Requests)
- `?` = Other status code

## Test Workflow

### 1. Start the Server

```bash
./dist/bin/server
```

### 2. Run Tests in Another Terminal

```bash
# Quick test
./dist/bin/server-test -t rps -n 1000 -c 10

# Heavy load test
./dist/bin/server-test -t concurrent -c 200 -d 60

# Verify DDoS protection
./dist/bin/server-test -t ddos -r 500
```

## Interpreting Results

### RPS Test

- **High RPS**: Server is performing well
- **Low RPS**: Check for bottlenecks (CPU, I/O, network)
- **High failure rate**: Server may be overloaded or misconfigured

### Concurrent Connections Test

- Shows sustained performance under constant load
- Monitor for connection drops or errors
- Useful for testing connection limits

### DDoS Protection Test

- **Status 429 responses**: Security module is working
- **All 200 responses**: Rate limiting may not be configured
- **Connection errors**: Server may be rejecting connections

## Performance Tips

1. **Adjust thread count** (`-c`): More threads = more load, but diminishing returns after CPU cores
2. **Monitor server resources**: Use `htop`, `iostat` during tests
3. **Network limits**: Local tests are limited by loopback performance
4. **Server warm-up**: First requests may be slower (caching, JIT)

## Troubleshooting

### Connection Refused

- Ensure server is running
- Check host and port settings
- Verify firewall rules

### Low Performance

- Check server configuration
- Monitor system resources
- Test network latency: `ping localhost`

### Test Tool Crashes

- Reduce thread count (`-c`)
- Reduce request rate (`-r` for DDoS test)
- Check system limits: `ulimit -n`

## Advanced Usage

### Test Specific Endpoint

```bash
./dist/bin/server-test -u /api/endpoint -t rps -n 5000
```

### Long Duration Stress Test

```bash
./dist/bin/server-test -t concurrent -c 500 -d 3600
```

### Quick Health Check

```bash
./dist/bin/server-test -t rps -n 100 -c 5
```

## Notes

- Tests generate real HTTP traffic
- DDoS test will trigger security features
- Interrupted tests (Ctrl+C) will stop gracefully
- Maximum thread count is limited to 1000

## Integration with CI/CD

Example for automated testing:

```bash
#!/bin/bash
# Start server in background
./dist/bin/server &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Run tests
./dist/bin/server-test -t rps -n 1000 -c 10
TEST_RESULT=$?

# Stop server
kill $SERVER_PID

exit $TEST_RESULT
```

## See Also

- [Server Configuration](../include/growtopia/config/server.h) - Configure rate limits
- [Security Module](../include/growtopia/security.h) - DDoS protection details
- [Main Documentation](README.md) - Project overview
