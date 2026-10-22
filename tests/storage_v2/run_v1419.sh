#!/usr/bin/env bash
# BEETLE V1.4.19. Pure offline storage and native-reader regression tests.
set -euo pipefail
cd "$(dirname "$0")/../.."
bash tests/storage_v2/run_v1418.sh
python tests/storage_v2/test_v1419_coexistence.py
