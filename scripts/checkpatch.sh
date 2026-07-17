#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu

CHECKPATCH=${CHECKPATCH:-scripts/checkpatch.pl}
exec "$CHECKPATCH" --no-tree --strict "$@"
