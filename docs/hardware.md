# GXFP3200 hardware discovery

## Known identification

- ACPI hardware ID: `GXFP3200`
- Bus: SPI
- Target systems: Huawei laptops (specific models still to be recorded)

## Information to collect per machine

| Item | Value |
| --- | --- |
| DMI model / BIOS version | |
| SPI controller and chip-select | |
| SPI mode, word size, and maximum frequency | |
| Reset GPIO polarity and timing | |
| IRQ GPIO polarity and trigger | |
| `vdd` / `vddio` rails and voltages | |
| ACPI `_CRS`, `_DSM`, and GPIO resources | |

Do not infer electrical settings from this skeleton. Capture ACPI tables and
confirm them against hardware traces before changing defaults.
