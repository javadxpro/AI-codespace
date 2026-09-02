#!/usr/bin/env bash
cd "$(dirname "$0")"
PORT="${1:-8080}"
echo ""
echo "  کیمیا — ویرایشگر جهان"
echo "  http://127.0.0.1:${PORT}"
echo "  http://0.0.0.0:${PORT}"
echo ""
if command -v python3 >/dev/null 2>&1; then
  exec python3 -m http.server "$PORT" --bind 0.0.0.0
elif command -v python >/dev/null 2>&1; then
  exec python -m http.server "$PORT" --bind 0.0.0.0
else
  echo "Python پیدا نشد. در Termux: pkg install python"
  exit 1
fi
