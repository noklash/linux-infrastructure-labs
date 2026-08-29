##PROJECT 01: PROCWATCH1. 

Engineering objective 
Build a self-contained C tool that reconstructs process information directly from the Linux /proc filesystem (the same source used by ps, top, and htop). The tool must enumerate processes, compute CPU utilisation from successive samples, support sorting and filtering, show a process tree, and inspect file descriptors / memory maps for a single PID. No external process listing utilities are used.

2. Linux concepts exercised/proc as a virtual filesystem exposing kernel process state
/proc/[pid]/stat (comm, state, ppid, utime, stime, num_threads, starttime, vsize, rss, …)
/proc/[pid]/status (Name, State, PPid, Threads, VmSize, VmRSS, …)
/proc/[pid]/cmdline (null-separated arguments)
/proc/[pid]/fd and /proc/[pid]/maps
Process lifecycle visibility (zombie, stopped, running, …)
CPU time accounting (user + system jiffies) and conversion using sysconf(_SC_CLK_TCK)
Sampling interval for utilisation calculation
Race conditions when a process exits between directory read and file open
Directory entry enumeration with readdir / opendir
File descriptor → open file / socket relationship (later used by sockwatch)

3. Architecture

procwatch
├── main.c              – argument parsing, top-level dispatch
├── proc.h / proc.c     – process table, scanning, sorting, tree
├── display.h / display.c – table / tree / single-process output
├── util.h / util.c     – helpers (safe string, time, path)
└── Makefile

One-shot scan → populate array of struct proc_info
For CPU %: two scans separated by a configurable interval (default 1 s)
Sorting by CPU or memory performed in userspace after the second sample
Tree built by walking parent–child links (PPID)
Single-PID mode and --fds / --maps open the corresponding /proc files directly

4. Directory structuretext

linux-infrastructure-labs/
└── 01-procwatch/
    ├── README.md
    ├── Makefile
    ├── src/
    │   ├── main.c
    │   ├── proc.h
    │   ├── proc.c
    │   ├── display.h
    │   ├── display.c
    │   ├── util.h
    │   └── util.c
    ├── tests/
    │   └── test_basic.sh
    ├── examples/
    └── scripts/

5–6. Complete source

