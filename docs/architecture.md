# Architecture

The kernel module is separated into device lifecycle (`gxfp3200_main.c`), SPI
transport (`gxfp3200_spi.c`), GPIO reset, power, IRQ, and debugfs components.
Transport owns retries and serializes individual SPI operations; protocol code
will be added only after packet framing has been validated. No userspace ABI is
promised at this stage.

The intended future path is a narrow, documented character-device ABI consumed
by a libfprint backend. Sensor-specific packet parsing must not be duplicated in
that backend.
