#!/usr/bin/env bash
#
# Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

binary="${SROBOTIS_OUTPUT_STAGING:-output/staging}/bin/mlink_device_test"

if [[ ! -x "${binary}" ]]; then
  echo "[mlink-device] ERROR: missing executable: ${binary}" >&2
  exit 1
fi

if "${binary}" status >/tmp/mlink-device-test-status.log 2>&1; then
  cat /tmp/mlink-device-test-status.log >&2 || true
  echo "[mlink-device] ERROR: mlink_device_test is already running; refusing to stop an existing process" >&2
  exit 1
fi

cleanup() {
  "${binary}" stop >/dev/null 2>&1 || true
}
trap cleanup EXIT

"${binary}" start
"${binary}" status
"${binary}" restart
"${binary}" status
"${binary}" stop

if "${binary}" status >/tmp/mlink-device-test-status.log 2>&1; then
  cat /tmp/mlink-device-test-status.log >&2 || true
  echo "[mlink-device] ERROR: process still running after stop" >&2
  exit 1
fi

echo "MLINK DEVICE DAEMON LIFECYCLE TEST PASSED."
