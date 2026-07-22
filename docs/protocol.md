# GXFP3200 protocol notes

The packet framing, command IDs, response timing, checksum, and firmware
requirements are unknown. The debugfs raw transport is provided to gather
reproducible observations without encoding unverified assumptions in the
kernel driver.

Record every observation with SPI mode, clock, reset/power state, transmitted
bytes, received bytes, IRQ timing, and the Windows driver version used.


## Research exchange helper

The kernel module exposes a deliberately narrow `gxfp3200_protocol_exchange()`
helper for protocol research. It validates transfer sizes and performs only a
write phase followed by a read phase; it does not encode command IDs, checksums,
firmware loading, or packet parsing.

Use debugfs `exchange` for candidate Goodix SPI command sequences while keeping
observations reproducible. Write `<rx_len> <hex-bytes>` to submit a transaction,
then read the same file to retrieve the last successful response. Failed
exchanges clear the stored response so stale data is not mistaken for a new
observation. Only promote a sequence from debugfs research into permanent
protocol code after it has been confirmed against GXFP3200 hardware traces.
