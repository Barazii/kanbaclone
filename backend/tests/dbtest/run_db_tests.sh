#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Kanba DB Contract Tests ==="
echo ""

# Clean up any previous run
echo "[1/3] Starting test PostgreSQL + building tests..."
docker compose -f docker-compose.test.yml down -v 2>/dev/null || true

# Run everything via docker compose
echo "[2/3] Running contract tests in Docker..."
echo ""
TEST_EXIT=0
docker compose -f docker-compose.test.yml up --build --abort-on-container-exit --exit-code-from testrunner 2>&1 || TEST_EXIT=$?
echo ""

# Cleanup
echo "[3/3] Stopping containers..."
docker compose -f docker-compose.test.yml down -v 2>/dev/null

if [ $TEST_EXIT -eq 0 ]; then
    echo ""
    echo "=== ALL CONTRACT TESTS PASSED ==="
else
    echo ""
    echo "=== SOME TESTS FAILED (exit code: $TEST_EXIT) ==="
fi

exit $TEST_EXIT
