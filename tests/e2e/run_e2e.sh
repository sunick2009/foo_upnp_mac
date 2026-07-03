#!/bin/sh
# End-to-end test: run upnp-browser-cli against the python mock server.
#
# Usage: run_e2e.sh <path-to-cli> <path-to-mock-server.py>
# Requires python3. Exits nonzero on the first failed assertion.

set -eu

CLI="$1"
MOCK="$2"
PORT="${E2E_PORT:-18299}"
SERVER_URL="http://127.0.0.1:${PORT}/rootDesc.xml"

fail() { echo "FAIL: $1" >&2; exit 1; }

python3 "$MOCK" "$PORT" 2>/dev/null &
MOCK_PID=$!
trap 'kill "$MOCK_PID" 2>/dev/null || true' EXIT

# wait for the mock to accept connections (up to ~5s)
i=0
until python3 -c "import urllib.request,sys; urllib.request.urlopen('$SERVER_URL', timeout=1)" 2>/dev/null; do
    i=$((i + 1))
    [ "$i" -ge 50 ] && fail "mock server did not start on port $PORT"
    sleep 0.1
done

echo "--- browse root (json) ---"
ROOT_JSON=$("$CLI" browse --server "$SERVER_URL")
echo "$ROOT_JSON" | python3 -m json.tool > /dev/null || fail "root browse output is not valid json"
echo "$ROOT_JSON" | grep -q '"id": "music"' || fail "root browse missing music container"
echo "$ROOT_JSON" | grep -q '"child_count": 2' || fail "root browse missing child_count"

echo "--- browse music: selector must prefer flac ---"
MUSIC_JSON=$("$CLI" browse --server "$SERVER_URL" --object-id music)
echo "$MUSIC_JSON" | grep -q '"selected_resource": "http://127.0.0.1:'"$PORT"'/media/track-1.flac"' \
    || fail "selector did not pick the flac resource"
echo "$MUSIC_JSON" | grep -q '第二首歌' || fail "unicode title was mangled"

echo "--- table output ---"
"$CLI" browse --server "$SERVER_URL" --object-id music --output table \
    | grep -q "audio/flac" || fail "table output missing selected mime"

echo "--- soap fault must exit 1 with the upnp code ---"
set +e
FAULT_OUT=$("$CLI" browse --server "$SERVER_URL" --object-id bogus 2>&1)
FAULT_EXIT=$?
set -e
[ "$FAULT_EXIT" -eq 1 ] || fail "fault path exited $FAULT_EXIT, expected 1"
echo "$FAULT_OUT" | grep -q "SoapFault 701" || fail "fault output missing SoapFault 701: $FAULT_OUT"

echo "--- usage error must exit 2 ---"
set +e
"$CLI" browse > /dev/null 2>&1
USAGE_EXIT=$?
set -e
[ "$USAGE_EXIT" -eq 2 ] || fail "usage error exited $USAGE_EXIT, expected 2"

echo "PASS: all e2e assertions"
