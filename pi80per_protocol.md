# Host-Device Protocol

## `00[len][data]`
write [len] bytes of [data] to UART

## `01[ch]`
read 1 byte from UART

## `02[drive]`
set active disk to [drive]
- allowed drive values: 0-3

## `03[sector]`
set active sector to [sector]
- allowed sector values: 1-32

## `04[track]`
set active track to [track]
- allowed track values: 0-31

## `05[cylinder]`
set active cylinder to [cylinder]
- stick to 0. cp/m doesn't use it

## `06[len][data]`
write [len] bytes of [data] to active disk
- always write complete sectors - 128 bytes

## `07[len]`
read [len] bytes from active disk
- always read complete sectors - 128 bytes

# Notes
- always read and write complete sectors
- cylinder is optional - cp/m doesn't use it