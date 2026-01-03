#!/bin/bash
# Quick test script - starts server and runs basic tests

SERVER_BIN="./dist/bin/server"
TEST_BIN="./dist/bin/server-test"
SERVER_PID=""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

cleanup() {
    if [ ! -z "$SERVER_PID" ]; then
        echo -e "\n${YELLOW}Stopping server...${NC}"
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    fi
    exit
}

trap cleanup INT TERM EXIT

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║     Growtopia HTTP Server - Automated Test Runner            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if binaries exist
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_BIN${NC}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

if [ ! -f "$TEST_BIN" ]; then
    echo -e "${RED}Error: Test binary not found at $TEST_BIN${NC}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

# Create public directory if it doesn't exist
if [ ! -d "public" ]; then
    mkdir -p public
    echo "<html><body><h1>Test Server</h1></body></html>" > public/index.html
    echo -e "${GREEN}✓${NC} Created public/index.html"
fi

# Start server in background
echo -e "${YELLOW}Starting server...${NC}"
$SERVER_BIN > server.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
echo -e "${YELLOW}Waiting for server to start...${NC}"
sleep 2

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}✗ Server failed to start!${NC}"
    echo "Check server.log for details:"
    tail -20 server.log
    exit 1
fi

echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"
echo ""

# Parse command line arguments
TEST_TYPE="${1:-quick}"

case "$TEST_TYPE" in
    quick)
        echo "Running quick test suite..."
        $TEST_BIN -t rps -n 100 -c 5
        ;;
    rps)
        echo "Running RPS test..."
        $TEST_BIN -t rps -n 1000 -c 10
        ;;
    concurrent)
        echo "Running concurrent connections test..."
        $TEST_BIN -t concurrent -c 50 -d 10
        ;;
    ddos)
        echo "Running DDoS protection test..."
        $TEST_BIN -t ddos -r 100
        ;;
    stress)
        echo "Running stress test..."
        $TEST_BIN -t rps -n 10000 -c 100
        ;;
    all)
        echo "Running full test suite..."
        $TEST_BIN -t all
        ;;
    *)
        echo "Usage: $0 [quick|rps|concurrent|ddos|stress|all]"
        echo ""
        echo "Test types:"
        echo "  quick      - Quick test (100 requests, 5 threads)"
        echo "  rps        - RPS test (1000 requests, 10 threads)"
        echo "  concurrent - Concurrent connections test (50 connections, 10s)"
        echo "  ddos       - DDoS protection test (100 req/s)"
        echo "  stress     - Stress test (10000 requests, 100 threads)"
        echo "  all        - Run all tests"
        exit 1
        ;;
esac

TEST_EXIT=$?

echo ""
if [ $TEST_EXIT -eq 0 ]; then
    echo -e "${GREEN}✓ Tests completed successfully${NC}"
else
    echo -e "${RED}✗ Tests failed with exit code $TEST_EXIT${NC}"
fi

# Server log tail
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "Server log (last 10 lines):"
echo "═══════════════════════════════════════════════════════════════"
tail -10 server.log

cleanup
