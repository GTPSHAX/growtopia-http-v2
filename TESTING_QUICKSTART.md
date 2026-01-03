# Quick Start - Testing

## Automated Testing (Recommended)

The easiest way to test the server:

```bash
# Quick test (100 requests)
./scripts/test.sh quick

# Full test suite
./scripts/test.sh all

# Specific tests
./scripts/test.sh rps        # RPS test
./scripts/test.sh concurrent # Concurrent connections
./scripts/test.sh ddos       # DDoS protection
./scripts/test.sh stress     # Heavy load test
```

The script will:

1. ✓ Start the server automatically
2. ✓ Wait for it to be ready
3. ✓ Run the tests
4. ✓ Stop the server cleanly
5. ✓ Show server logs

## Manual Testing

### 1. Start the server

```bash
./dist/bin/server
```

### 2. In another terminal, run tests

```bash
# Quick test
./dist/bin/server-test -t rps -n 100 -c 5

# Full test
./dist/bin/server-test -t all

# Custom test
./dist/bin/server-test -h localhost -p 8000 -t rps -n 5000 -c 50
```

## Troubleshooting

**"Connection refused" errors:**

- Make sure the server is running
- Check that port 8000 is available
- Verify with: `netstat -tuln | grep 8000`

**Segmentation fault:**

- Rebuild the project: `cmake --build build`
- Check system limits: `ulimit -n`

**All requests fail:**

- Server may not be running on port 8000
- Check firewall: `sudo ufw status`
- Test manually: `curl http://localhost:8000/`

## Quick Health Check

```bash
# Start server in background
./dist/bin/server &

# Quick test
./dist/bin/server-test -t rps -n 10 -c 2

# Stop server
killall server
```

For detailed documentation, see [docs/TESTING.md](docs/TESTING.md)
