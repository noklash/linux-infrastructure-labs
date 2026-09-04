# MEMMAP

## Linux Process Virtual Memory Inspection Tool

`memmap` is a C-based Linux systems engineering tool for inspecting the virtual memory layout of a running process.

It reads Linux's `/proc` process interfaces directly, primarily:

* `/proc/<PID>/maps`
* `/proc/<PID>/smaps`
* `/proc/<PID>/comm`

The project was built to understand how Linux represents process memory internally and how tools such as `pmap`, `procps`, `top`, and other system observability utilities obtain information about processes.

The project deliberately operates close to the operating system rather than hiding the underlying mechanisms behind high-level libraries.

---

# 1. Project Purpose

The purpose of `memmap` is to answer questions such as:

* What virtual memory regions does a process have?
* Where are those regions located in the virtual address space?
* How large is each mapping?
* What permissions does each mapping have?
* Is the mapping anonymous or backed by a file?
* Which mappings belong to shared libraries?
* Where is the heap?
* Where is the stack?
* Which regions are executable?
* Which regions are writable?
* How much of each mapping is actually resident in physical memory?
* How much memory is proportionally attributed to a process?
* How much virtual address space has been reserved without necessarily consuming physical RAM?

The project is therefore both:

1. a practical Linux memory inspection utility, and
2. a systems engineering laboratory for understanding virtual memory.

---

# 2. Core Engineering Principle

The project follows a simple principle:

> Observe the operating system through the interfaces it already exposes.

Rather than asking another command such as `pmap` or `ps` for information, `memmap` reads the kernel's process information interfaces itself.

The architecture therefore looks like:

```text
                    Linux Kernel
                         │
                         │
                    /proc/<PID>
                         │
          ┌──────────────┼──────────────┐
          │              │              │
        maps           smaps           comm
          │              │              │
          └──────────────┼──────────────┘
                         │
                         ▼
                    memmap parser
                         │
                         ▼
                  memory data model
                         │
                         ▼
                     analyzer
                         │
                         ▼
                      display
                         │
             ┌───────────┼───────────┐
             │           │           │
           table       summary       JSON
```

This separation is intentional.

The program does not treat `/proc` as the final representation of the data. It parses the kernel-provided text interfaces into structured C data and then performs analysis on that representation.

---

# 3. Project Architecture

Current repository structure:

```text
linux-infrastructure-labs/
└── 02-memmap/
    ├── build/
    ├── docs/
    ├── examples/
    ├── experiments/
    ├── scripts/
    ├── src/
    │   ├── analyzer.c
    │   ├── analyzer.h
    │   ├── cli.c
    │   ├── cli.h
    │   ├── display.c
    │   ├── display.h
    │   ├── main.c
    │   ├── maps.c
    │   ├── maps.h
    │   ├── memory.c
    │   ├── memory.h
    │   ├── proc.c
    │   ├── proc.h
    │   ├── smaps.c
    │   ├── smaps.h
    │   ├── util.c
    │   └── util.h
    ├── tests/
    ├── makefile
    ├── memmap
    ├── memory_test
    ├── memory_test_touched
    ├── memory_test_touched.c
    ├── memory_test_untouched
    ├── memory_test_untouched.c
    ├── memory_test.c.save
    ├── ReadMe.md
    ├── caseStudy.md
    └── .gitignore
```

The important architectural boundary is:

```text
main
 │
 ▼
CLI
 │
 ▼
process collection
 │
 ├── maps
 │
 └── smaps
 │
 ▼
memory model
 │
 ▼
analysis
 │
 ▼
display
```

---

# 4. Source Code Responsibilities

## `main.c`

Owns program execution.

Responsibilities:

* Parse command-line arguments.
* Handle help requests.
* Handle watch mode.
* Register signal handlers.
* Execute one inspection cycle.
* Coordinate the major subsystems.

The main flow is:

```c
parse_cli()
        ↓
run_once()
        ↓
memory_map_init()
        ↓
collect_process_memory()
        ↓
classify_mappings()
        ↓
sort_mappings()
        ↓
display_*
        ↓
memory_map_free()
```

`main.c` deliberately does not parse `/proc` itself.

This keeps the program's orchestration separate from its implementation details.

---

# 5. CLI Layer

## `cli.h`

Defines the command-line configuration.

The `sort_key_t` enumeration represents supported ordering modes:

```text
SORT_NONE
SORT_SIZE
SORT_RSS
SORT_PSS
SORT_START
```

The `struct cli_options` structure stores all user-selected options.

Examples:

```text
PID
summary
libraries
heap
stack
exec filter
write filter
shared filter
private filter
anonymous filter
JSON
watch
interval
sort key
```

---

## `cli.c`

Responsible for converting command-line arguments into structured configuration.

Supported options:

```text
--summary
--libraries
--heap
--stack
--exec
--write
--shared
--private
--anonymous
--sort KEY
--json
--watch
--interval SEC
-h
--help
```

Example:

```bash
./memmap 1234 --summary
```

becomes approximately:

```text
opts.pid = 1234
opts.summary = true
```

Another example:

```bash
./memmap 1234 --sort rss --json
```

produces:

```text
opts.pid = 1234
opts.sort = SORT_RSS
opts.json = true
```

---

# 6. PID Validation

PID parsing is implemented independently in `util.c`.

The parser rejects:

```text
empty strings
non-numeric strings
negative values
zero
overflow
trailing characters
NULL input
NULL output pointers
```

For example:

```text
1234        valid
1           valid
0           invalid
-1          invalid
123abc      invalid
abc123      invalid
```

The implementation uses:

```c
strtoul()
```

rather than `atoi()`.

This is an intentional engineering decision.

`atoi()` does not provide a reliable mechanism for detecting conversion errors. `strtoul()` provides:

* an end pointer,
* `errno`,
* overflow detection,
* explicit conversion semantics.

---

# 7. `/proc` Integration

## `proc.c`

This is the process-level boundary between `memmap` and Linux's `/proc` filesystem.

The program first verifies that:

```text
/proc/<PID>
```

exists.

It then reads:

```text
/proc/<PID>/comm
/proc/<PID>/maps
/proc/<PID>/smaps
```

The sequence is:

```text
PID
 │
 ▼
/proc/<PID>
 │
 ├── comm
 │
 ├── maps
 │
 └── smaps
```

---

# 8. Why `/proc/<PID>/comm` Exists Separately

`comm` provides the process's command name.

For example:

```text
/proc/1234/comm
```

may contain:

```text
bash
```

The implementation removes the trailing newline and stores a dynamically allocated copy.

Memory ownership is then handled by:

```c
memory_map_free()
```

This prevents the process collection layer from having to manage display lifetime manually.

---

# 9. Linux `/proc/<PID>/maps`

`maps` describes the process's virtual memory mappings.

A typical entry looks conceptually like:

```text
ADDRESS-ADDRESS PERMISSIONS OFFSET DEVICE INODE PATH
```

For example:

```text
7f1234000000-7f1234100000 rw-p 00000000 00:00 0 [anonymous]
```

The parser extracts:

```text
start
end
permissions
offset
device
inode
pathname
```

The mapping size is then calculated as:

```text
size = end - start
```

This is important because the kernel gives us the address range rather than explicitly handing us a "size" field.

---

# 10. Why `getline()` Is Used

The `/proc/<PID>/maps` file contains lines of variable length.

Pathnames can be long.

Using a fixed small buffer would introduce unnecessary assumptions.

The implementation therefore uses:

```c
getline()
```

which dynamically grows the input buffer as required.

Because `getline()` is a POSIX interface, the source explicitly enables the required feature set:

```c
#define _POSIX_C_SOURCE 200809L
```

This is why that definition appears at the top of:

```text
maps.c
smaps.c
```

---

# 11. `ssize_t` vs `size_t`

`getline()` returns:

```c
ssize_t
```

because it needs to represent both:

* a positive number of bytes read
* `-1` for failure or end-of-file

Therefore:

```c
ssize_t nread;
```

is correct.

Using:

```c
size_t nread;
```

would be semantically incorrect because `size_t` is unsigned and cannot represent `-1`.

This was one of the compiler warnings encountered and corrected during development.

---

# 12. `/proc/<PID>/smaps`

`smaps` provides more detailed per-mapping memory statistics.

Examples include:

```text
Size:
Rss:
Pss:
Shared_Clean:
Shared_Dirty:
Private_Clean:
Private_Dirty:
Referenced:
Anonymous:
Swap:
```

These values are reported by Linux in kilobytes.

The program converts them to bytes:

```text
bytes = kilobytes × 1024
```

This allows the internal representation to use a consistent unit.

---

# 13. RSS, PSS and VSS

These are three different concepts and are central to the project.

## VSS

Virtual Set Size, as used by this project, represents the total size of the virtual mappings.

Conceptually:

```text
VSS = sum(mapping sizes)
```

VSS tells us about virtual address space.

It does not mean that the same amount of physical RAM is being consumed.

---

## RSS

Resident Set Size.

RSS represents pages from a process's mappings that are currently resident in physical memory.

Conceptually:

```text
RSS = resident physical pages attributed to mappings
```

RSS is therefore much closer to physical memory usage than VSS.

---

## PSS

Proportional Set Size.

PSS attempts to divide the cost of shared pages among processes sharing them.

For example, if a page is shared by four processes:

```text
RSS attribution:

Process A = 1 page
Process B = 1 page
Process C = 1 page
Process D = 1 page

Total attributed RSS = 4 pages
```

PSS instead approximately attributes:

```text
Process A = 0.25 page
Process B = 0.25 page
Process C = 0.25 page
Process D = 0.25 page
```

This makes PSS particularly useful when comparing actual memory responsibility between processes.

---

# 14. Mapping Data Model

The central data structure is:

```c
struct memory_mapping
```

It represents one virtual memory region.

It contains:

```text
start
end
permissions
offset
device
inode
pathname
size
type
smaps statistics
```

A complete process is represented by:

```c
struct memory_map
```

which contains:

```text
array of mappings
mapping count
array capacity
PID
process command name
system page size
```

This gives the project a clean internal model:

```text
Process
  │
  └── Memory Map
        │
        ├── Mapping 1
        ├── Mapping 2
        ├── Mapping 3
        └── ...
```

---

# 15. Dynamic Mapping Storage

The mapping collection grows dynamically.

Initial capacity:

```text
64 mappings
```

When capacity is exhausted:

```text
new capacity = old capacity × 2
```

This gives:

```text
64
128
256
512
1024
...
```

The doubling strategy avoids reallocating memory for every single mapping.

This is a standard dynamic-array strategy with amortized efficient insertion.

---

# 16. Memory Ownership

A particularly important design decision is ownership.

When a mapping is pushed into the collection:

```c
mm->items[mm->count] = *m;
```

the structure is copied.

The dynamically allocated `pathname` pointer is therefore transferred conceptually into the collection.

`memory_map_free()` is responsible for freeing:

```text
every pathname
the mapping array
the process command string
```

This creates a clear ownership model:

```text
memory_map
    owns
      ├── items
      │    └── pathname strings
      └── comm
```

This is important in C because there is no garbage collector.

---

# 17. Mapping Classification

The analyzer converts raw `/proc` information into semantic categories.

Supported types:

```text
unknown
executable
library
heap
stack
vdso
vvar
vsyscall
anonymous
file
```

Examples:

```text
[heap]       → heap
[stack]      → stack
[vdso]       → vdso
[vvar]       → vvar
[vsyscall]   → vsyscall
no pathname  → anonymous
*.so         → library
absolute path + executable → executable
absolute path → file
```

This is an example of a key systems-engineering transformation:

```text
raw kernel data
        ↓
semantic interpretation
        ↓
human-readable model
```

---

# 18. Why Classification Happens After Parsing

The `/proc` parser should answer:

> What did the kernel tell us?

The analyzer should answer:

> What does that information mean?

Keeping these responsibilities separate means the parser does not need to know what a library or heap is.

That makes the architecture easier to test and reason about.

---

# 19. Summary Analysis

`analyze_summary()` calculates:

```text
VSS
RSS
PSS
anonymous RSS
file-backed RSS
executable mapping size
writable mapping size
shared mapping size
private mapping size
```

The analyzer operates on the structured memory model rather than reading `/proc` directly.

This means summary calculations can be tested with artificial mappings without needing a real process.

That separation is extremely useful for automated testing.

---

# 20. Filtering

Filters operate on individual mappings.

Supported filters:

```text
libraries
heap
stack
executable
writable
shared
private
anonymous
```

The function:

```c
mapping_matches_filters()
```

answers:

> Should this mapping be displayed under the current CLI configuration?

This is intentionally separated from the display layer.

The display layer does not need to understand what "anonymous" means.

It simply asks:

```c
mapping_matches_filters(m, opts)
```

---

# 21. Sorting

Mappings can be sorted by:

```text
size
RSS
PSS
start address
```

Size, RSS and PSS are sorted descending.

Start addresses are sorted ascending.

Example:

```bash
./memmap <PID> --sort rss
```

will place mappings with the highest resident memory first.

This is implemented using the standard C library:

```c
qsort()
```

rather than implementing a custom sorting algorithm.

This is an intentional choice.

The engineering value of this project lies in Linux memory inspection, not in reimplementing sorting algorithms unnecessarily.

---

# 22. Display Layer

The display subsystem is intentionally separated from data collection and analysis.

Supported output modes:

```text
normal map table
summary
libraries
heap
stack
JSON
```

This separation allows the same underlying memory model to support multiple presentation formats.

Conceptually:

```text
                    memory_map
                        │
              ┌─────────┼─────────┐
              │         │         │
            table     summary     JSON
```

---

# 23. JSON Output

JSON output allows `memmap` to be consumed by other programs.

Example conceptual structure:

```json
{
  "pid": 1234,
  "name": "example",
  "page_size": 4096,
  "mappings": [
    {
      "start": "0x...",
      "end": "0x...",
      "size": 4096,
      "permissions": "rw-p",
      "offset": 0,
      "inode": 0,
      "path": "",
      "type": "anonymous",
      "rss": 4096,
      "pss": 4096
    }
  ]
}
```

This is an important step toward machine-readable observability tooling.

The JSON output can eventually be consumed by:

```text
Python
Go
Node.js
shell scripts
monitoring systems
dashboards
CI jobs
```

---

# 24. Watch Mode

The tool supports:

```bash
./memmap <PID> --watch
```

The program repeatedly executes the same inspection cycle.

Conceptually:

```text
collect
  ↓
analyze
  ↓
display
  ↓
sleep
  ↓
collect
  ↓
...
```

The interval is configurable:

```bash
./memmap <PID> --watch --interval 2
```

The default interval is:

```text
1 second
```

---

# 25. Signal Handling

Watch mode handles:

```text
SIGINT
SIGTERM
```

The signal handler sets:

```c
g_stop = 1;
```

The main loop then exits naturally.

This is preferable to performing complicated cleanup directly inside the signal handler.

The signal handler remains minimal:

```c
static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}
```

---

# 26. Process Disappearance

A running process can disappear while being inspected.

This is normal Linux behavior.

For example:

```text
memmap checks /proc/1234
             ↓
process exits
             ↓
memmap attempts /proc/1234/maps
```

The directory or file may now be gone.

The implementation therefore handles:

```text
ENOENT
```

and reports:

```text
Process <PID> exited during inspection.
```

This was explicitly tested.

The observed behavior was:

```text
Error: process 10041 does not exist.
```

The important engineering lesson is:

> `/proc` represents a live system, so its contents can change while being read.

The program must therefore expect races.

---

# 27. Permission Handling

The program explicitly handles:

```text
EACCES
```

and reports permission problems rather than producing unexplained failures.

This is important because `/proc` visibility can depend on:

* user identity
* process ownership
* system security configuration
* Linux `hidepid` configuration
* container isolation
* other security policies

---

# 28. Troubleshooting History

This section documents problems encountered during development because they are part of the engineering story.

---

## 28.1 `getline()` Compiler Problems

The project uses:

```c
getline()
```

Initially, the compiler required the appropriate POSIX feature definition.

The solution was:

```c
#define _POSIX_C_SOURCE 200809L
```

at the top of files using `getline()`.

The lesson:

C library functions are sometimes controlled by feature-test macros.

The compiler does not automatically expose every POSIX interface under every compilation mode.

---

## 28.2 `size_t` vs `ssize_t`

`getline()` returns `ssize_t`.

The code originally required correction from:

```c
size_t nread;
```

to:

```c
ssize_t nread;
```

Reason:

```text
ssize_t
  signed integer
  can represent -1

size_t
  unsigned integer
  cannot represent -1
```

This was a compiler correctness issue rather than merely a stylistic preference.

---

## 28.3 `sscanf()` Parsing Warning

The `/proc/<PID>/maps` parser initially produced a compiler warning around the format string and buffer handling.

The parser was corrected to explicitly limit pathname input:

```c
n = sscanf(line, "%lx-%lx %4s %lx %31s %lu %4095[^\n]",
           &start, &end, perms, &offset, device, &inode, pathbuf);
```

The important engineering properties are:

```text
%4s
%31s
%4095[^\n]
```

These bounds reduce the possibility of overflowing fixed-size buffers.

---

## 28.4 Untouched `malloc()` Experiment

The first memory experiment attempted to allocate approximately 100 MB using:

```c
malloc()
```

without actually using the returned memory.

The resulting mapping was unexpectedly small.

Investigation showed that the compiler could optimize away the allocation because the memory was not meaningfully used.

This was an important lesson:

> Source-level intent does not necessarily equal machine-level behavior.

The compiler is allowed to remove work whose observable effects do not matter.

---

# 29. Why the Memory Experiment Was Changed to `mmap()`

To make the experiment directly demonstrate Linux virtual memory behavior, the experiment was rewritten using:

```c
mmap()
```

with:

```text
MAP_PRIVATE
MAP_ANONYMOUS
PROT_READ | PROT_WRITE
```

This creates an explicit anonymous virtual memory mapping.

Two controlled programs were then created.

---

# 30. Untouched Memory Experiment

The untouched program reserves approximately:

```text
100 MB
```

of anonymous virtual memory.

It does not write to the pages.

Observed result:

```text
Virtual Memory (mapped):  102.71 MB
Resident (RSS):           1.84 MB
Proportional (PSS):       155.0 KB
Anonymous (RSS):          56.0 KB
File-backed (RSS):        1.78 MB
Executable mappings:      1.79 MB
Writable mappings:        100.35 MB
Private mappings:         102.71 MB
```

The critical observation is:

```text
VSS ≈ 102.71 MB
RSS ≈ 1.84 MB
```

The process has a large virtual mapping without having the entire mapping resident in physical memory.

This demonstrates why:

```text
virtual memory ≠ physical memory
```

---

# 31. Touched Memory Experiment

The touched program writes one byte every:

```text
4096 bytes
```

which corresponds to the system's common page size.

Conceptually:

```text
page 1 → touched
page 2 → touched
page 3 → touched
...
```

Observed result:

```text
Virtual Memory (mapped):  102.71 MB
Resident (RSS):           101.84 MB
Proportional (PSS):       100.15 MB
Anonymous (RSS):          100.05 MB
File-backed (RSS):        1.79 MB
Executable mappings:      1.79 MB
Writable mappings:        100.35 MB
Private mappings:         102.71 MB
```

The important comparison is:

```text
                    Untouched       Touched

VSS                 102.71 MB       102.71 MB
RSS                   1.84 MB       101.84 MB
Anonymous RSS         0.05 MB       100.05 MB
```

The virtual mapping size remains essentially unchanged.

Physical residency changes dramatically.

---

# 32. Why Touching Memory Changes RSS

Modern operating systems use demand paging.

Creating a virtual mapping does not necessarily mean every page immediately has a physical page frame backing it.

When the process accesses a page:

```text
CPU accesses virtual address
          ↓
page not resident
          ↓
page fault
          ↓
Linux establishes physical backing
          ↓
instruction continues
```

Therefore:

```text
mmap()
```

can establish a large virtual region while only a small amount of physical memory is resident.

Writing to each page causes those pages to become resident.

This experiment is one of the strongest demonstrations in the project.

---

# 33. Anonymous Mapping Validation

For the touched experiment,:

```bash
./memmap <PID> --anonymous
```

produced an approximately:

```text
100 MB
```

anonymous writable mapping.

Example:

```text
START            END              SIZE       PERMS  RSS        TYPE
0000723d7c400000 0000723d82800000 100.00 MB rw-p   100.00 MB  anonymous
```

This demonstrates that:

1. the mapping exists in the process address space,
2. it is writable,
3. it is anonymous,
4. it has approximately 100 MB of resident memory.

---

# 34. Page Size

The program obtains the system page size using:

```c
sysconf(_SC_PAGESIZE)
```

and stores it in:

```c
mm->page_size
```

The observed system page size during testing was:

```text
4096 bytes
```

This matters because memory management operates in pages rather than arbitrary byte-sized units.

---

# 35. Manual Validation

The following features have been manually tested successfully.

## Summary

```bash
./memmap <PID> --summary
```

Validated against:

```text
/proc/<PID>/status
```

For example:

```text
VmSize: 8808 kB
VmRSS:  2940 kB
```

The tool's values were compared against the kernel-provided process information.

---

## Libraries

```bash
./memmap <PID> --libraries
```

Validated.

---

## Heap

```bash
./memmap <PID> --heap
```

Validated.

---

## Stack

```bash
./memmap <PID> --stack
```

Validated.

---

## Executable mappings

```bash
./memmap <PID> --exec
```

Validated.

---

## Writable mappings

```bash
./memmap <PID> --write
```

Validated.

---

## Shared mappings

```bash
./memmap <PID> --shared
```

Validated.

---

## Private mappings

```bash
./memmap <PID> --private
```

Validated.

---

## Anonymous mappings

```bash
./memmap <PID> --anonymous
```

Validated.

---

## Sorting

Validated:

```bash
./memmap <PID> --sort rss
./memmap <PID> --sort pss
./memmap <PID> --sort size
./memmap <PID> --sort start
```

---

## JSON

Validated:

```bash
./memmap <PID> --json
```

---

## Watch

Validated:

```bash
./memmap <PID> --watch
```

and:

```bash
./memmap <PID> --watch --interval 2
```

---

## Invalid PID

Validated.

---

## Nonexistent PID

Validated.

---

## Process disappearance

Validated.

The tool correctly detects when the inspected process no longer exists.

---

# 36. Compiler Configuration

The project uses:

```make
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g
```

Each option serves a purpose.

## `-std=c11`

Use the C11 language standard.

This gives the project a predictable language baseline.

---

## `-Wall`

Enable a broad set of compiler warnings.

---

## `-Wextra`

Enable additional warnings beyond `-Wall`.

---

## `-Wpedantic`

Ask the compiler to diagnose non-standard language extensions more aggressively.

---

## `-O2`

Enable a practical optimization level.

This is appropriate for the normal build.

---

## `-g`

Include debugging information.

This makes tools such as:

```text
gdb
addr2line
sanitizers
```

more useful.

---

# 37. Debug Build

The project also defines a debug configuration using:

```text
-O0
-g
-fsanitize=address
-fsanitize=undefined
```

AddressSanitizer (ASan) is intended to detect memory safety problems such as:

```text
heap buffer overflow
use-after-free
double free
invalid memory access
```

UndefinedBehaviorSanitizer (UBSan) helps identify forms of undefined behavior.

This is especially valuable for C because memory safety is largely the programmer's responsibility.

---

# 38. Build Process

Normal build:

```bash
make
```

Produces:

```text
memmap
```

Object files are stored in:

```text
build/
```

This keeps generated object files out of the source directories.

---

# 39. Cleaning

```bash
make clean
```

removes generated build artifacts and binaries.

The intention is to allow a clean rebuild:

```bash
make clean
make
```

---

# 40. Automated Testing Strategy

The automated test system should be divided into two levels.

## Unit testing

Test deterministic functions without depending on a live process.

Examples:

```text
parse_pid()
format_size()
xstrdup()
parse_maps_line()
memory_map_init()
memory_map_push()
memory_map_free()
classify_mappings()
analyze_summary()
mapping_matches_filters()
sort_mappings()
parse_cli()
```

These tests should use synthetic data.

---

## Integration testing

Integration tests should use real Linux `/proc` behavior.

Examples:

```text
real process
real /proc/<PID>/maps
real /proc/<PID>/smaps
real CLI
real output
```

This catches problems that unit tests cannot.

---

# 41. Why Both Unit and Integration Tests Are Necessary

A unit test might prove:

```text
parse_maps_line()
```

can correctly interpret a synthetic line.

It cannot prove that:

```text
/proc/<PID>/maps
```

is opened correctly.

Likewise, an integration test may prove that:

```bash
./memmap <PID> --summary
```

works.

It does not necessarily isolate the exact function responsible when something breaks.

Therefore:

```text
unit tests
+
integration tests
=
stronger confidence
```

---

# 42. Testing Principles

Tests should:

* avoid fixed process IDs
* avoid relying on a particular user's processes
* avoid assumptions about a specific distribution
* use the current process where practical
* create controlled helper processes for memory experiments
* test failure paths
* test cleanup
* test command exit codes
* test machine-readable output
* run on Linux

A good test should be reproducible.

---

# 43. Recommended Test Cases

## PID parser

Test:

```text
valid PID
PID 1
PID 0
negative PID
empty input
alphabetic input
mixed input
overflow
NULL
```

---

## Maps parser

Test:

```text
normal mapping
anonymous mapping
file-backed mapping
special mapping
malformed line
long pathname
invalid address range
```

---

## Classification

Test:

```text
[heap]
[stack]
[vdso]
[vvar]
[vsyscall]
anonymous
.so library
executable
file-backed mapping
unknown mapping
```

---

## Summary

Construct known mappings and verify:

```text
VSS
RSS
PSS
anonymous
file
executable
writable
shared
private
```

---

## Sorting

Verify:

```text
size descending
RSS descending
PSS descending
start ascending
no sort
```

---

## CLI

Test:

```text
missing PID
multiple PIDs
unknown option
missing --sort value
invalid sort key
missing --interval value
invalid interval
--help
normal PID
```

---

# 44. Important Current Parser Regression

During code review of the current source, the `maps.c` pathname conversion should be verified carefully.

The intended format is:

```c
"%lx-%lx %4s %lx %31s %lu %4095[^\n]"
```

The pathname conversion must not suppress assignment.

If the format contains:

```c
%*4095[^\n]
```

the `*` means:

> perform the conversion but do not assign the result.

That would prevent `pathbuf` from receiving the pathname.

This is especially serious because pathname information drives classification.

For example:

```text
[heap]
[stack]
.so
executable path
file path
```

would become unavailable to the classifier.

A dedicated parser regression test should therefore verify:

```text
input maps line
       ↓
pathname
       ↓
expected pathname string
```

This test is valuable because it protects a critical architectural boundary.

---

# 45. Why This Bug Is Easy to Miss

The address and permission fields could still parse correctly.

The program could therefore appear to work.

For example:

```text
START
END
SIZE
PERMS
```

might look valid while:

```text
TYPE
PATH
```

are wrong.

This is a classic systems-programming failure mode:

> A parser can partially succeed while silently corrupting the semantic meaning of the data.

That is why parser tests should verify every field, not merely successful return codes.

---

# 46. Error Handling Philosophy

The project distinguishes between:

## Fatal collection errors

Examples:

```text
cannot open maps
memory allocation failure
unexpected system failure
```

These can cause the inspection to fail.

## Best-effort enrichment

`smaps` is intentionally treated as best-effort.

If:

```text
smaps
```

cannot be read because of:

```text
EACCES
ENOENT
```

the program can still retain the information obtained from:

```text
maps
```

This is an important design decision.

The tool's core purpose is mapping inspection.

Detailed `smaps` statistics are an enhancement.

---

# 47. Why `maps` Comes Before `smaps`

The architecture intentionally collects:

```text
maps
```

first.

Then:

```text
smaps
```

enriches those mappings.

This creates a natural model:

```text
maps
  ↓
struct memory_mapping
  ↓
smaps enrichment
  ↓
struct mapping_stats
```

The mapping exists independently of its detailed statistics.

That makes the program resilient when `smaps` information is unavailable.

---

# 48. Resource Cleanup

The code consistently closes:

```text
FILE *
```

handles.

It also frees:

```text
getline buffers
pathname strings
mapping arrays
command strings
```

The general lifecycle is:

```text
allocate
   ↓
use
   ↓
free
```

The central cleanup function is:

```c
memory_map_free()
```

This reduces the chance of leaking resources across different execution paths.

---

# 49. Security Considerations

This tool reads process information.

It does not:

* inject code,
* modify another process,
* ptrace another process,
* write to `/proc`,
* alter process memory,
* execute commands on behalf of the inspected process.

The tool is therefore observational.

Nevertheless, input parsing remains security-sensitive because `/proc` contains externally supplied textual data from the kernel interface.

The implementation therefore uses bounded string conversions such as:

```text
%4s
%31s
%4095[^\n]
```

and explicit buffer sizes.

---

# 50. Portability

The project is intentionally Linux-specific.

It depends on:

```text
/proc
Linux process memory semantics
Linux smaps
Linux mapping conventions
POSIX interfaces
```

It should therefore be described as:

> Linux systems software

rather than portable C software.

This is a deliberate tradeoff.

The project is intended to demonstrate Linux systems knowledge.

---

# 51. Why C Was Chosen

C is an appropriate language for this project because it exposes:

```text
memory ownership
pointers
system calls
file descriptors
process interfaces
binary layout
manual allocation
```

without a large runtime abstraction layer.

The project is therefore able to demonstrate understanding of what happens underneath higher-level languages.

For example, a JavaScript application may ask:

```javascript
someLibrary.getProcessMemory()
```

while this project asks Linux directly:

```text
/proc/<PID>/maps
/proc/<PID>/smaps
```

and constructs the representation itself.

---

# 52. Engineering Lessons

This project demonstrates several important systems concepts.

## Virtual memory

A process sees virtual addresses rather than physical RAM addresses.

---

## Demand paging

Virtual memory can exist without all pages being resident.

---

## Page faults

Accessing an unbacked page can cause Linux to establish physical backing.

---

## Memory mappings

A process's address space is composed of multiple mappings with different properties.

---

## File-backed memory

Executable code and shared libraries can be mapped into a process address space.

---

## Anonymous memory

Memory does not need a filesystem pathname.

---

## Shared memory

Mappings can be shared between processes.

---

## Copy-on-write

Private mappings can share physical pages until modification requires separation.

---

## RSS and PSS

Different memory accounting models answer different operational questions.

---

## `/proc`

Linux exposes process state through a virtual filesystem interface.

---

# 53. Why This Is an Infrastructure Engineering Project

The project is small enough to understand completely while touching several infrastructure-level concepts:

```text
Linux kernel
     ↓
/proc
     ↓
process state
     ↓
memory management
     ↓
C systems programming
     ↓
observability
     ↓
machine-readable output
```

It therefore provides a strong foundation for future projects involving:

```text
container runtimes
process supervisors
resource monitors
Linux agents
observability collectors
Kubernetes node agents
AI infrastructure monitoring
GPU/process telemetry
```

---

# 54. What the Project Does Not Attempt

The project intentionally does not attempt to become:

```text
htop
pmap
perf
valgrind
gdb
a debugger
a profiler
a memory allocator
```

Those are separate problems.

The scope is:

> Understand and expose Linux process virtual memory through `/proc`.

Keeping the scope focused is an architectural decision.

---

# 55. Known Limitations

Several areas can be improved later.

## JSON escaping

The current JSON renderer directly prints path strings.

A pathname containing characters requiring JSON escaping could produce invalid JSON.

A future version should implement proper JSON string escaping.

---

## Classification heuristics

Library classification currently uses patterns such as:

```text
.so
ld-linux
ld-musl
```

This is practical but heuristic.

A more rigorous implementation could inspect ELF metadata.

---

## `smaps` matching

The current implementation matches `smaps` entries to `maps` entries using:

```text
start
end
```

and walks mappings sequentially.

This is efficient for the expected ordered kernel output but assumes the entries correspond in the expected order.

---

## Summary semantics

The project's VSS/RSS/PSS terminology should be documented as the tool's accounting model rather than assumed to exactly reproduce every metric from another utility.

---

## Thread stacks

Linux can expose multiple stack-related mappings.

The classifier intentionally recognizes:

```text
[stack
```

rather than only:

```text
[stack]
```

so mappings such as:

```text
[stack:1234]
```

can also be recognized where supported.

---

# 56. Future Improvements

Potential future work should remain focused.

## Testing

Add:

```text
automated unit tests
integration tests
sanitizer runs
JSON validation
controlled helper processes
```

---

## JSON

Implement:

```text
proper JSON escaping
summary JSON
library JSON
schema documentation
```

---

## `/proc` robustness

Potentially add:

```text
more race handling
partial smaps handling
additional kernel-version compatibility
```

---

## Observability

A future project could consume:

```text
memmap --json
```

and expose metrics to:

```text
Prometheus
OpenTelemetry
```

That would be a natural progression from this project.

---

# 57. Recommended Validation Workflow

Before considering a release:

```bash
make clean
make
```

Then:

```bash
./memmap --help
```

Start a controlled process:

```bash
./memory_test_touched
```

Record its PID.

Run:

```bash
./memmap <PID> --summary
./memmap <PID> --anonymous
./memmap <PID> --sort rss
./memmap <PID> --json
```

Then terminate the process.

Finally:

```bash
./memmap <PID> --summary
```

should demonstrate the missing-process handling.

---

# 58. Clean Build Requirement

A clean build should produce no warnings.

The project's standard compiler configuration is intentionally strict:

```text
-Wall
-Wextra
-Wpedantic
```

Warnings should be treated as engineering feedback.

For systems software, compiler warnings often identify:

```text
incorrect format strings
signed/unsigned mismatches
uninitialized variables
type mismatches
unused values
possible portability problems
```

The project should therefore maintain a zero-warning normal build.

---

# 59. Debugging Workflow

When something fails:

## Step 1: reproduce

Make the failure deterministic.

## Step 2: inspect the kernel interface

For example:

```bash
cat /proc/<PID>/maps
cat /proc/<PID>/smaps
cat /proc/<PID>/status
```

## Step 3: compare tool output

Determine whether the error originates from:

```text
collection
parsing
classification
analysis
display
```

## Step 4: use compiler diagnostics

Build with:

```text
-Wall
-Wextra
-Wpedantic
```

## Step 5: use sanitizers

Use:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

## Step 6: inspect with a debugger if required

Use:

```bash
gdb ./memmap
```

The debugging strategy should always move toward the actual layer responsible for the incorrect behavior.

---

# 60. Portfolio Presentation

The strongest way to present this project is not simply:

> "I built a memory monitoring tool in C."

A stronger description is:

> Built a Linux process virtual-memory inspection tool in C by parsing `/proc/<PID>/maps` and `/proc/<PID>/smaps` directly, implementing mapping classification, RSS/PSS analysis, filtering, sorting, JSON output, watch mode, and controlled demand-paging experiments.

The project demonstrates:

```text
C
Linux
/proc
virtual memory
memory accounting
systems programming
observability
process inspection
resource management
testing
debugging
```

---

# 61. Recommended Portfolio Demonstration

A particularly strong demonstration is the 100 MB memory experiment.

Show:

```text
Program A
100 MB mmap
memory untouched
RSS ≈ 2 MB
```

Then:

```text
Program B
100 MB mmap
every page touched
RSS ≈ 102 MB
```

This tells a much stronger engineering story than a screenshot of a CLI table.

The demonstration proves that the tool is observing a real operating-system phenomenon.

---

# 62. Suggested Case Study Structure

The separate `caseStudy.md` should explain:

```text
1. Problem
2. Why /proc
3. Architecture
4. Parsing maps
5. Parsing smaps
6. Memory model
7. Classification
8. RSS vs PSS vs VSS
9. Demand paging experiment
10. Troubleshooting
11. Testing
12. Engineering lessons
13. Future work
```

The most compelling narrative is:

```text
Question
  ↓
How can a process reserve 100 MB without consuming 100 MB RAM?
  ↓
Experiment
  ↓
mmap()
  ↓
Untouched pages
  ↓
RSS remains low
  ↓
Touch every page
  ↓
RSS rises dramatically
  ↓
memmap observes the difference
```

---

# 63. Engineering Decisions Summary

| Decision                                | Reason                                                     |
| --------------------------------------- | ---------------------------------------------------------- |
| C                                       | Direct systems-level control                               |
| `/proc` directly                        | Avoid hiding the kernel interface                          |
| Separate parser/analyzer/display layers | Clear responsibility boundaries                            |
| `maps` as base data                     | Core mapping information                                   |
| `smaps` as enrichment                   | Detailed metrics are optional                              |
| Dynamic mapping array                   | Processes have variable mapping counts                     |
| Doubling capacity                       | Efficient dynamic growth                                   |
| `strtoul()` for PID parsing             | Reliable validation                                        |
| `getline()`                             | Variable-length `/proc` lines                              |
| `ssize_t` for `getline()` result        | Correct representation of `-1`                             |
| `qsort()`                               | Avoid unnecessary custom algorithms                        |
| JSON mode                               | Machine-readable observability                             |
| Watch mode                              | Live observation                                           |
| Minimal signal handler                  | Safe shutdown architecture                                 |
| Best-effort `smaps`                     | Preserve useful information when detailed data unavailable |
| `mmap()` experiment                     | Deterministic virtual-memory demonstration                 |
| Strict compiler warnings                | Catch defects early                                        |
| Sanitizers                              | Detect memory/undefined behavior                           |
| No frontend                             | CLI is the appropriate interface for a systems tool        |

---

# 64. Definition of Done

The project can be considered functionally complete when:

```text
[x] Parse process mappings
[x] Read process command name
[x] Parse smaps statistics
[x] Classify mappings
[x] Calculate summary statistics
[x] Filter mappings
[x] Sort mappings
[x] Display mappings
[x] Display summary
[x] Display libraries
[x] Display heap
[x] Display stack
[x] JSON output
[x] Watch mode
[x] Configurable watch interval
[x] Process disappearance handling
[x] Permission handling
[x] Controlled mmap experiments
[x] VSS/RSS experiment
[x] Strict compiler warnings
[ ] Automated test suite fully wired into Makefile
[ ] Sanitizer test workflow
[ ] Final README cleanup
[ ] Final architecture documentation
[ ] Portfolio case study
```

---

# 65. Final Architectural Picture

The entire project can be understood as five layers:

```text
┌──────────────────────────────────────────────┐
│                  CLI / UI                    │
│                                              │
│  --summary --heap --json --sort --watch     │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│                Orchestration                 │
│                                              │
│                 main.c                       │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│             Linux Data Collection            │
│                                              │
│       /proc/<PID>/maps                       │
│       /proc/<PID>/smaps                      │
│       /proc/<PID>/comm                       │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│              Memory Analysis                 │
│                                              │
│ classification                              │
│ summary                                     │
│ filtering                                   │
│ sorting                                     │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│                  Output                      │
│                                              │
│ table | summary | libraries | regions | JSON│
└──────────────────────────────────────────────┘
```

At the bottom of everything is Linux itself:

```text
                 Linux Kernel
                      │
                      ▼
             Process Address Space
                      │
                      ▼
                    /proc
                      │
                      ▼
                   memmap
```

That is the central idea of the project.

`memmap` is not pretending that memory is a single number.

It exposes the structure underneath that number.

---

# 66. Project Takeaway

The most important lesson from the project is:

```text
A process does not simply "have memory."

It has a virtual address space composed of mappings.
Those mappings have permissions, backing, ownership,
and residency characteristics.
Linux exposes much of that information through /proc.
```

The project turns that kernel interface into an inspectable model.

That makes `02-memmap` a useful bridge between:

```text
Linux fundamentals
        ↓
C systems programming
        ↓
virtual memory
        ↓
observability
        ↓
infrastructure engineering
```

This is exactly the type of small, deeply understood systems project that can serve as a foundation for larger infrastructure work.
