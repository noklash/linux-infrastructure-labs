PROJECT 01: PROCWATCH

1. Engineering Objective

Build a self-contained C tool that reconstructs process information directly from the Linux /proc filesystem, the same kernel interface used by tools such as ps, top, and htop.

The tool must:

* Enumerate running processes.
* Read process metadata directly from /proc.
* Compute CPU utilisation from successive process samples.
* Support sorting by CPU and memory usage.
* Support process filtering.
* Display the process hierarchy as a tree.
* Inspect file descriptors for a single process.
* Inspect memory mappings for a single process.
* Handle processes that disappear while being inspected.
* Avoid using external process-listing utilities.

The project should demonstrate an understanding of how Linux exposes process state to userspace and how system utilities reconstruct that state.

⸻

2. Linux Concepts Exercised

/proc Virtual Filesystem

Understand /proc as a virtual filesystem exposing kernel-maintained process and system state.

/proc/[pid]/stat

Parse fields including:

* comm
* state
* ppid
* utime
* stime
* num_threads
* starttime
* vsize
* rss
* and other process statistics.

/proc/[pid]/status

Parse human-readable process information including:

* Name
* State
* PPid
* Threads
* VmSize
* VmRSS
* and other process attributes.

/proc/[pid]/cmdline

Read the process command line.

Arguments are separated by NUL (\0) characters rather than normal spaces or newlines.

/proc/[pid]/fd

Inspect a process’s open file descriptors.

Understand the relationship between:

PID
 │
 └── /proc/PID/fd/
       ├── 0 → terminal
       ├── 1 → terminal/file
       ├── 2 → terminal/file
       └── socket:[12345]

This becomes foundational for the later sockwatch project.

/proc/[pid]/maps

Inspect a process’s virtual memory mappings.

Understand mappings for:

* Executable files
* Shared libraries
* Heap
* Stack
* Anonymous memory
* Memory-mapped files
* Shared memory regions

Process Lifecycle Visibility

Recognise process states such as:

* Running
* Sleeping
* Zombie
* Stopped
* Traced
* Dead

CPU Time Accounting

Understand Linux CPU accounting using:

utime + stime

where:

* utime = CPU time spent executing userspace code.
* stime = CPU time spent executing kernel code.

Convert jiffies to seconds using:

sysconf(_SC_CLK_TCK)

CPU Utilisation Sampling

Calculate CPU utilisation from two successive process samples:

Sample 1
   ↓
wait(interval)
   ↓
Sample 2
   ↓
calculate CPU delta
   ↓
calculate CPU %

Race Conditions

Handle cases where:

1. A PID appears during directory enumeration.
2. The process exits before /proc/[pid]/stat is opened.
3. A process exits between reading different /proc/[pid] files.

The program must treat these conditions as normal runtime behaviour rather than fatal errors.

Directory Enumeration

Use Linux/POSIX directory APIs:

opendir()
readdir()
closedir()

to enumerate numeric PID directories under /proc.

File Descriptor to Resource Relationships

Understand how:

/proc/[pid]/fd/[fd]

can point to:

* Regular files
* Pipes
* Terminals
* Devices
* Sockets
* Other kernel resources

This knowledge will later be used by sockwatch.

⸻

3. Architecture

procwatch/
├── main.c
│   └── Argument parsing
│   └── Top-level dispatch
│
├── proc.h
├── proc.c
│   └── Process table
│   └── /proc scanning
│   └── Process parsing
│   └── Sorting
│   └── Process tree construction
│
├── display.h
├── display.c
│   └── Table output
│   └── Tree output
│   └── Single-process output
│
├── util.h
├── util.c
│   └── Safe string helpers
│   └── Time helpers
│   └── Path helpers
│
└── Makefile

Processing Flow

One-Shot Scan

/proc
  ↓
enumerate PID directories
  ↓
read process information
  ↓
populate struct proc_info array
  ↓
sort/filter
  ↓
display

CPU Utilisation

CPU utilisation requires two process samples.

First scan
    ↓
record utime + stime
    ↓
wait configurable interval
    ↓
Second scan
    ↓
record utime + stime
    ↓
calculate delta
    ↓
calculate CPU utilisation

The default sampling interval should be:

1 second

The interval should be configurable through the command line.

Sorting

Sorting should be performed in userspace after the process information has been collected.

Supported sorting criteria should include:

CPU usage
Memory usage

Process Tree

The process tree should be constructed using parent-child relationships.

Each process contains a:

PPID

The tree is created by matching:

child.ppid == parent.pid

Conceptually:

init/systemd
├── process A
│   ├── child A1
│   └── child A2
├── process B
│   └── child B1
└── process C

Single-PID Inspection

Single-process mode should read the relevant /proc files directly.

For example:

/proc/<pid>/stat
/proc/<pid>/status
/proc/<pid>/cmdline

Additional inspection modes:

--fds
    ↓
/proc/<pid>/fd/
/--maps
    ↓
/proc/<pid>/maps

⸻

4. Directory Structure

linux-infrastructure-labs/
└── 01-procwatch/
    ├── README.md
    ├── Makefile
    │
    ├── src/
    │   ├── main.c
    │   ├── proc.h
    │   ├── proc.c
    │   ├── display.h
    │   ├── display.c
    │   ├── util.h
    │   └── util.c
    │
    ├── tests/
    │   └── test_basic.sh
    │
    ├── examples/
    │
    └── scripts/

⸻

5. Complete Source

The implementation must include the complete source code for:

src/main.c
src/proc.h
src/proc.c
src/display.h
src/display.c
src/util.h
src/util.c
Makefile
tests/test_basic.sh

The implementation should be written in portable, conventional C suitable for Linux userspace systems programming.

⸻

6. Engineering Requirements

Core Requirements

procwatch must be able to:

* Enumerate /proc.
* Identify numeric PID directories.
* Read /proc/[pid]/stat.
* Read /proc/[pid]/status.
* Read /proc/[pid]/cmdline.
* Store process information in a structured process table.
* Display process information.
* Calculate CPU utilisation.
* Sort processes by CPU.
* Sort processes by memory.
* Filter processes.
* Display a process tree.
* Inspect file descriptors.
* Inspect memory maps.
* Handle disappearing processes safely.
* Avoid invoking ps, top, htop, or other external process-listing tools.

Engineering Constraints

The program should use Linux/POSIX interfaces directly, including APIs such as:

opendir()
readdir()
closedir()
open()
read()
close()
fopen()
fgets()
fclose()
sysconf()
qsort()
snprintf()
strncpy()
strstr()

Where appropriate, prefer robust error handling over assuming that /proc entries remain valid after they have been discovered.

⸻

7. Expected Result

The finished project should behave like a small, purpose-built process inspection utility built from first principles.

Example conceptual output:

PID      USER       STATE    CPU%    MEM%    THREADS    COMMAND
1        root       S        0.0     0.1     1          /sbin/init
742      user       S        2.4     1.8     12         /usr/bin/code
1832     user       R        8.7     3.2     8          ./worker
2914     user       S        0.1     0.4     2          ./server

Tree mode:

1 /sbin/init
├── 742 /usr/bin/code
│   ├── 801 renderer
│   └── 802 renderer
├── 1832 ./worker
│   ├── 1833 ./worker
│   └── 1834 ./worker
└── 2914 ./server

File descriptor inspection:

PID 2914
FD     TARGET
0      /dev/pts/2
1      /tmp/server.log
2      /tmp/server.err
3      socket:[48291]
4      socket:[48292]

Memory map inspection:

PID 2914
ADDRESS RANGE          PERMS    OFFSET     PATH
00400000-00412000      r-xp     00000000   ./server
00611000-00612000      r--p     00011000   ./server
00612000-00613000      rw-p     00012000   ./server
7f20...-7f21...        r-xp     00000000   libc.so.6
7ffd...-7ffe...        rw-p     00000000   [stack]

⸻

8. Definition of Done

The project is complete when:

1. procwatch builds successfully using the provided Makefile.
2. The executable can enumerate real Linux processes directly through /proc.
3. Process information is represented using C structures rather than shell parsing.
4. CPU utilisation is calculated from two samples.
5. Sorting and filtering work without delegating to external utilities.
6. The process hierarchy is reconstructed using PPID relationships.
7. File descriptors can be inspected for a selected PID.
8. Memory mappings can be inspected for a selected PID.
9. Processes disappearing during inspection do not crash the program.
10. Basic automated tests pass.
11. The implementation can be understood as a miniature reconstruction of the core process-observation functionality provided by tools such as ps and top.

⸻

9. Engineering Principle

Observe the operating system directly before relying on abstractions built on top of it.

procwatch is intended to establish practical understanding of how Linux exposes process state to userspace and how a systems engineer can reconstruct higher-level observability tools from those primitive interfaces.
