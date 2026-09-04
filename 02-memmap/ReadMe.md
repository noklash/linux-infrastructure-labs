# memmap

Linux process virtual-memory inspection tool.

Reads `/proc/PID/maps` and `/proc/PID/smaps`, builds an internal memory model,
classifies regions, and reports VSS / RSS / PSS.

## Build

```bash
make
make examples


./memmap PID
./memmap --summary PID
./memmap --libraries PID
./memmap --heap PID
./memmap --stack PID
./memmap --anonymous --write PID
./memmap --sort rss PID
./memmap --json PID
./memmap --watch --interval 2 PID