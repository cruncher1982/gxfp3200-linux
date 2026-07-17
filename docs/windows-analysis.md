# Windows driver analysis

This document is intentionally a research log, not a protocol specification.
Obtain `gxfp.sys` from hardware you own or are authorized to inspect. Record
its version and cryptographic hash before analysis. Focus first on device
initialization, SPI framing, reset and power sequencing, IRQ handling, and
firmware loading paths.
