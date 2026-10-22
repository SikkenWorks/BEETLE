#!/usr/bin/env bash
# BEETLE V1.4.17 offline storage / migration regression. Never touches FM1.
set -euo pipefail
cd "$(dirname "$0")/../.."
python tests/storage_v2/test_legacy_flash_audit.py
python tests/storage_v2/test_migration_preflight.py
python tests/storage_v2/run_tests.py
python tests/dpcm4/test_storage_integration.py
python tests/dpcm4/test_v1415_pinned_storage.py
