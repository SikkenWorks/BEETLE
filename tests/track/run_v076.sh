#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 tests/track/test_v076_dump_salvage.py
bash tests/track/run_v075.sh
