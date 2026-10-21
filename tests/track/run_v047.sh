#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python tests/track/test_beetle_v047_visual.py
bash tests/track/run_v046.sh
