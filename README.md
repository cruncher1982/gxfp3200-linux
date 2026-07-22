# Goodix GXFP3200 Linux support

This repository develops an upstream-oriented Linux driver for the Goodix
GXFP3200 SPI fingerprint sensor, including the ACPI device ID `GXFP3200` found
on supported Huawei laptops. It is an early hardware-enablement project: it
does **not** yet provide biometric authentication or a libfprint backend.

## Current status

The kernel component provides ACPI/Device Tree matching, optional reset and IRQ
GPIO handling, optional `vdd`/`vddio` regulators, suspend/resume power handling,
retrying SPI transport helpers, and a deliberately privileged debugfs interface
for protocol research. The sensor protocol has not been recovered yet.

## Build, install, and test the external kernel module

For a quick one-shot build and load test:

```sh
./scripts/gxfp3200-build-install-test.sh build
./scripts/gxfp3200-build-install-test.sh load
```

The helper wraps the usual external-module `make` and `insmod` commands.

For prerequisites, verification commands, debugfs test steps, dynamic-debug
tracing, cleanup, and optional persistent installation, see
[Build, install, and test the GXFP3200 driver](docs/build-install-test.md).

To make it selectable in a kernel source tree, include `kernel/Kconfig` from
that tree's SPI Kconfig and add `obj-$(CONFIG_SPI_GXFP3200) += gxfp3200/` to its
Makefile.

## Debugging interface

When debugfs is mounted, the driver creates
`/sys/kernel/debug/<spi-device-name>/`. `status` reports power and IRQ state;
writing anything to `reset` pulses the reset GPIO. `send` accepts an even-length,
contiguous hexadecimal byte string (for example `0102a0`), and `receive` clocks
and returns 256 bytes. `exchange` accepts `<rx_len> <hex-bytes>` (for example
`16 0102a0`), runs one write-then-read SPI exchange, and returns the last
successful response as space-separated hexadecimal bytes. `status` also reports
the stored exchange response length. These controls issue arbitrary traffic to
the sensor and are intended only for local protocol investigation.

## Roadmap

1. Verify platform GPIO, regulator, SPI mode, and clock parameters.
2. Recover and document the wire protocol from hardware traces and Windows
   driver analysis.
3. Add a validated protocol layer and a stable userspace character-device API.
4. Implement image capture and a libfprint backend.

See [the architecture](docs/architecture.md), [hardware research template](docs/hardware.md),
and [protocol notes](docs/protocol.md).
