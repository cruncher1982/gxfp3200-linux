# Build, install, and test the GXFP3200 driver

This guide explains how to build the GXFP3200 driver as an out-of-tree kernel
module, load it on a test machine, confirm that it bound to hardware, exercise
its debugfs research interface, and then unload it. The current driver is a
hardware-enablement and protocol-research driver only; it does not enroll
fingerprints, authenticate users, or provide a libfprint backend.

## What this codebase builds

The `kernel/` directory is an external Linux kernel module. Its Makefile builds
one module, `gxfp3200.ko`, from the lifecycle, SPI transport, GPIO, regulator,
IRQ, protocol-research, and debugfs source files. The driver matches ACPI device
ID `GXFP3200` and Device Tree compatible string `goodix,gxfp3200`.

At runtime the module:

- powers optional `vdd` and `vddio` regulators;
- acquires optional `reset` and `irq` GPIOs;
- requests a rising-edge threaded IRQ when an IRQ GPIO exists;
- retries SPI transfers up to three times;
- creates privileged debugfs files for manual SPI/protocol research.

## Prerequisites

Install the tools and headers for the exact kernel you will boot for testing:

```sh
sudo apt install build-essential linux-headers-$(uname -r) kmod ripgrep
```

You also need:

- a kernel configured with SPI support and debugfs support;
- the target GXFP3200 sensor exposed by ACPI as `GXFP3200` or by Device Tree as
  `goodix,gxfp3200`;
- root privileges to load and unload kernel modules;
- Secure Boot disabled, or a locally signed module accepted by your machine's
  Machine Owner Key (MOK) database.

## Build the module

The repository includes a helper script that automates the common build, load,
status, test, and install actions. To build for the running kernel, run:

```sh
./scripts/gxfp3200-build-install-test.sh build
```

The underlying command is:

```sh
make -C /lib/modules/$(uname -r)/build M="$PWD/kernel" modules
```

The expected output artifact is:

```sh
kernel/gxfp3200.ko
```

To remove generated build artifacts later, run:

```sh
./scripts/gxfp3200-build-install-test.sh clean
```

## Optional static checks

If you have a kernel source tree with `scripts/checkpatch.pl`, run:

```sh
CHECKPATCH=/path/to/linux/scripts/checkpatch.pl \
  ./scripts/checkpatch.sh kernel/*.c kernel/*.h kernel/Kconfig kernel/Makefile
```

The repository script runs checkpatch in `--no-tree --strict` mode.

## Install for a one-shot test

For development, prefer loading the freshly built module directly instead of
installing it permanently:

```sh
./scripts/gxfp3200-build-install-test.sh load
```

The helper loads `spi` first when available and then runs `insmod` on the local
`kernel/gxfp3200.ko` artifact.

If Secure Boot rejects the unsigned module, `insmod` fails with an operation or
key-related error. Disable Secure Boot for the test or sign the module with a
MOK-enrolled key before loading it.

## Confirm that the driver loaded

Check that the module is present and inspect kernel logs:

```sh
./scripts/gxfp3200-build-install-test.sh status
```

The equivalent manual checks are:

```sh
lsmod | rg '^gxfp3200'
sudo dmesg | tail -100 | rg -i gxfp3200
```

A successful probe logs a message similar to `Goodix GXFP3200 sensor
initialized`. If there is no probe message, confirm that the machine actually
exposes a matching SPI device:

```sh
rg "GXFP3200" /sys/bus/acpi/devices 2>/dev/null || true
find /sys/bus/spi/devices -maxdepth 2 -type f -name modalias -print -exec cat {} \;
```

Common issues are missing kernel headers, a module built for a different kernel
release, Secure Boot lockdown, or platform firmware that does not describe the
sensor as an SPI device with the expected ID.

## Mount debugfs

The research controls are under debugfs. Mount it if necessary:

```sh
./scripts/gxfp3200-build-install-test.sh debugfs
```

The helper mounts debugfs if needed and prints matching debugfs directories. The
manual mount command is:

```sh
sudo mount -t debugfs debugfs /sys/kernel/debug 2>/dev/null || true
```

Find the directory created for the SPI device:

```sh
sudo find /sys/kernel/debug -maxdepth 2 -type f -name status -path '*gxfp3200*' -print
```

Depending on how the SPI device is named, the directory is usually similar to
`/sys/kernel/debug/spi-GXFP3200:00/` or another SPI device name.

## Basic driver tests

Set a shell variable to the discovered debugfs directory:

```sh
DBG=/sys/kernel/debug/<spi-device-name>
```

Read status:

```sh
sudo cat "$DBG/status"
```

Expected fields are `powered`, `interrupts`, and `last_exchange_rx`.

Pulse the optional reset GPIO:

```sh
printf 1 | sudo tee "$DBG/reset" >/dev/null
```

If the platform did not describe a reset GPIO, the driver logs a debug message
and the operation is otherwise a no-op.

Read 256 raw bytes from the sensor:

```sh
sudo cat "$DBG/receive"
```

Send a contiguous even-length hexadecimal byte string:

```sh
printf '0102a0' | sudo tee "$DBG/send" >/dev/null
```

Run one write-then-read research exchange and read the stored response:

```sh
printf '16 0102a0' | sudo tee "$DBG/exchange" >/dev/null
sudo cat "$DBG/exchange"
```

The first token is the requested receive length. The second token is contiguous
hexadecimal transmit data. The driver accepts exchange sizes up to 4096 bytes.
These operations send arbitrary SPI traffic to the sensor, so use them only on a
local test machine where protocol experimentation is acceptable.

## Enable verbose SPI traces

The SPI helpers use dynamic debug hex dumps. Enable them with:

```sh
echo 'file gxfp3200_spi.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control >/dev/null
echo 'file gxfp3200_protocol.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control >/dev/null
sudo dmesg -w
```

Run the debugfs `send`, `receive`, or `exchange` commands in another terminal to
observe transmit and receive dumps.

## Unload the module after testing

Remove the module and confirm it is gone:

```sh
./scripts/gxfp3200-build-install-test.sh unload
```

## Optional persistent install

Only after a successful manual test, copy the module into the running kernel's
extra modules directory and refresh module dependency metadata:

```sh
./scripts/gxfp3200-build-install-test.sh install
sudo modprobe gxfp3200
```

To remove the persistent install:

```sh
./scripts/gxfp3200-build-install-test.sh uninstall
```

## Safety notes

- This is not a biometric authentication driver yet.
- The debugfs interface is intentionally privileged and unstable.
- Do not script production workflows against the debugfs files.
- Capture `dmesg` output, ACPI/Device Tree identifiers, and SPI traces when
  reporting test results.
