#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python tests/storage_v2/test_native_v2_reader.py
python tests/storage_v2/test_v1418_migration_image.py
bash tests/storage_v2/run_v1417.sh
python tests/dpcm4/test_v1414_storage_prefetch.py
