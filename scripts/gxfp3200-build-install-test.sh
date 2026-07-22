#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Build, install, and test helper for the GXFP3200 out-of-tree kernel module.
set -eu

usage() {
	cat <<'USAGE'
Usage: scripts/gxfp3200-build-install-test.sh [command]

Commands:
  build       Build kernel/gxfp3200.ko for the running kernel (default)
  clean       Remove generated out-of-tree module build artifacts
  load        Load the freshly built module with insmod for one-shot testing
  unload      Unload the module
  reload      Unload, build, and load the module
  status      Show module state, recent dmesg lines, and debugfs status files
  debugfs     Mount debugfs if needed and print matching debugfs directories
  install     Persistently install the module under /lib/modules/.../extra
  uninstall   Remove the persistent install and refresh module dependencies
  test        Run lightweight post-load checks; does not send SPI traffic

Environment:
  KERNEL_RELEASE  Kernel release to build/install for (default: uname -r)
  KERNEL_BUILD    Kernel build directory (default: /lib/modules/$KERNEL_RELEASE/build)
  MODULE_DIR      Module source directory (default: $REPO_ROOT/kernel)
  MODULE_NAME     Kernel module name (default: gxfp3200)
  SUDO            Privilege escalation command (default: sudo when not root)
USAGE
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
KERNEL_RELEASE=${KERNEL_RELEASE:-$(uname -r)}
KERNEL_BUILD=${KERNEL_BUILD:-/lib/modules/$KERNEL_RELEASE/build}
MODULE_DIR=${MODULE_DIR:-$REPO_ROOT/kernel}
MODULE_NAME=${MODULE_NAME:-gxfp3200}
MODULE_KO=$MODULE_DIR/$MODULE_NAME.ko
INSTALL_PATH=/lib/modules/$KERNEL_RELEASE/extra/$MODULE_NAME.ko

if [ "${SUDO+x}" ]; then
	SUDO_CMD=$SUDO
elif [ "$(id -u)" -eq 0 ]; then
	SUDO_CMD=
else
	SUDO_CMD=sudo
fi

run_root() {
	if [ -n "$SUDO_CMD" ]; then
		$SUDO_CMD "$@"
	else
		"$@"
	fi
}

require_kernel_build() {
	if [ ! -d "$KERNEL_BUILD" ]; then
		cat >&2 <<EOF_ERR
Missing kernel build directory: $KERNEL_BUILD
Install headers for the target kernel, for example:
  sudo apt install build-essential linux-headers-$KERNEL_RELEASE kmod
EOF_ERR
		exit 1
	fi
}

build_module() {
	require_kernel_build
	make -C "$KERNEL_BUILD" M="$MODULE_DIR" modules
	if [ ! -f "$MODULE_KO" ]; then
		echo "Build completed but $MODULE_KO was not found" >&2
		exit 1
	fi
	printf 'Built %s\n' "$MODULE_KO"
}

clean_module() {
	require_kernel_build
	make -C "$KERNEL_BUILD" M="$MODULE_DIR" clean
}

module_loaded() {
	awk -v name="$MODULE_NAME" '$1 == name { found = 1 } END { exit !found }' /proc/modules
}

unload_module() {
	if module_loaded; then
		run_root rmmod "$MODULE_NAME"
		printf 'Unloaded %s\n' "$MODULE_NAME"
	else
		printf '%s is not loaded\n' "$MODULE_NAME"
	fi
}

load_module() {
	if [ ! -f "$MODULE_KO" ]; then
		echo "Missing $MODULE_KO; building first"
		build_module
	fi
	if module_loaded; then
		printf '%s is already loaded\n' "$MODULE_NAME"
		return 0
	fi
	run_root modprobe spi || true
	run_root insmod "$MODULE_KO"
	printf 'Loaded %s from %s\n' "$MODULE_NAME" "$MODULE_KO"
}

mount_debugfs() {
	if ! awk '$3 == "debugfs" && $2 == "/sys/kernel/debug" { found = 1 } END { exit !found }' /proc/mounts; then
		run_root mount -t debugfs debugfs /sys/kernel/debug
	fi
}

find_debugfs_dirs() {
	mount_debugfs
	find /sys/kernel/debug -maxdepth 2 -type f -name status -path "*$MODULE_NAME*" \
		-print 2>/dev/null | sed 's#/status$##' | sort -u
}

show_status() {
	printf 'Kernel release: %s\n' "$KERNEL_RELEASE"
	printf 'Kernel build:   %s\n' "$KERNEL_BUILD"
	printf 'Module dir:     %s\n' "$MODULE_DIR"
	if module_loaded; then
		printf 'Module loaded:  yes\n'
	else
		printf 'Module loaded:  no\n'
	fi
	printf '\nRecent kernel messages:\n'
	dmesg | tail -100 | sed -n "/$MODULE_NAME/Ip" || true
	printf '\nDebugfs status files:\n'
	find_debugfs_dirs | while IFS= read -r dir; do
		[ -n "$dir" ] || continue
		printf '\n[%s]\n' "$dir"
		cat "$dir/status" || true
	done
}

show_debugfs() {
	find_debugfs_dirs | while IFS= read -r dir; do
		[ -n "$dir" ] || continue
		printf '%s\n' "$dir"
	done
}

install_module() {
	if [ ! -f "$MODULE_KO" ]; then
		build_module
	fi
	run_root install -D -m 0644 "$MODULE_KO" "$INSTALL_PATH"
	run_root depmod -a "$KERNEL_RELEASE"
	printf 'Installed %s\n' "$INSTALL_PATH"
	printf 'Load it with: sudo modprobe %s\n' "$MODULE_NAME"
}

uninstall_module() {
	if module_loaded; then
		run_root modprobe -r "$MODULE_NAME" || run_root rmmod "$MODULE_NAME"
	fi
	run_root rm -f "$INSTALL_PATH"
	run_root depmod -a "$KERNEL_RELEASE"
	printf 'Removed %s\n' "$INSTALL_PATH"
}

run_test() {
	if ! module_loaded; then
		echo "$MODULE_NAME is not loaded; run '$0 load' first" >&2
		exit 1
	fi
	dirs=$(find_debugfs_dirs)
	if [ -z "$dirs" ]; then
		echo "No $MODULE_NAME debugfs directory found; check dmesg and device binding" >&2
		exit 1
	fi
	printf '%s\n' "$dirs" | while IFS= read -r dir; do
		[ -n "$dir" ] || continue
		printf 'Debugfs directory: %s\n' "$dir"
		cat "$dir/status"
	done
}

cmd=${1:-build}
case "$cmd" in
	build) build_module ;;
	clean) clean_module ;;
	load) load_module ;;
	unload) unload_module ;;
	reload) unload_module; build_module; load_module ;;
	status) show_status ;;
	debugfs) show_debugfs ;;
	install) install_module ;;
	uninstall) uninstall_module ;;
	test) run_test ;;
	-h|--help|help) usage ;;
	*) usage >&2; exit 2 ;;
esac
