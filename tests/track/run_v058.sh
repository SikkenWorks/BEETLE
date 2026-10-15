#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python tests/track/test_beetle_v047_visual.py
python tests/track/test_beetle_v058_moon.py
