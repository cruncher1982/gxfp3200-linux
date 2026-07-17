# GXFP3200 protocol notes

The packet framing, command IDs, response timing, checksum, and firmware
requirements are unknown. The debugfs raw transport is provided to gather
reproducible observations without encoding unverified assumptions in the
kernel driver.

Record every observation with SPI mode, clock, reset/power state, transmitted
bytes, received bytes, IRQ timing, and the Windows driver version used.
