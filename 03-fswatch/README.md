# fswatch

A Linux filesystem inspection tool written in C.

`fswatch` is a systems programming and Linux infrastructure engineering project focused on understanding how Linux represents, exposes, and manages filesystem objects.

The project is intentionally developed from first principles. Instead of treating commands such as `ls`, `stat`, `find`, or `du` as black boxes, the implementation investigates the underlying Linux system calls, kernel interfaces, filesystem metadata, and observable behavior that make those tools possible.

The project is part of my **Enterprise AI Infrastructure Engineer journey**, specifically the filesystem and storage foundations of Linux infrastructure.

---

## Project Status

**Current stage:** Filesystem fundamentals

**Current command:**

```text
fswatch info <path>
```

**Current implementation:**

* command-line argument validation
* filesystem metadata lookup using `stat()`
* filesystem object size reporting
* regular-file detection
* directory detection
* basic error handling
* initial automated verification

The implementation is intentionally small at this stage.

The project will grow incrementally as new filesystem concepts are understood and experimentally verified.

---

# Why This Project Exists

Filesystem knowledge is foundational to Linux infrastructure engineering.

Processes interact with files through file descriptors. Services read configuration files, write logs, access sockets, consume device files, traverse directories, and depend on permissions and ownership. Storage systems expose filesystems through mounts. Containers depend heavily on filesystem namespaces, mounts, overlays, and filesystem isolation.

Understanding these systems requires more than knowing shell commands.

The purpose of `fswatch` is therefore to build a mental and practical model of the Linux filesystem by interacting with the operating system directly.

The project asks questions such as:

* What exactly is a file?
* What is a pathname?
* What happens when a pathname is resolved?
* What metadata does Linux maintain about a filesystem object?
* Where does file size come from?
* What is an inode?
* Why does a directory have a size?
* How does Linux distinguish different filesystem object types?
* What happens when a pathname points to a symbolic link?
* What is the difference between `stat()` and `lstat()`?
* How do hard links work?
* How does Linux represent ownership and permissions?
* How are directory entries represented?
* How does a process traverse a directory?
* What changes when a filesystem is mounted?
* How can a program identify filesystem boundaries?
* How do filesystem metadata and actual storage consumption differ?
* How do filesystem concepts affect real infrastructure systems?

The goal is to answer these questions through implementation, observation, experiments, and documentation.

---

# Engineering Philosophy

The project follows a simple progression:

```text
Understand the system
        ↓
Identify the operating-system interface
        ↓
Implement the smallest useful experiment
        ↓
Observe actual behavior
        ↓
Test the implementation
        ↓
Document what happened
        ↓
Record design decisions and limitations
        ↓
Extend the system
```

The project deliberately avoids premature abstraction.

If one source file is sufficient, the implementation remains in one source file.

If a new abstraction becomes necessary, it is introduced because the system has earned it, not because the project structure looks more sophisticated.

The same principle applies to documentation and testing.

---

# Current Architecture

```text
03-fswatch/
│
├── src/
│   └── main.c
│
├── include/
│
├── tests/
│   └── test_info.sh
│
├── experiments/
│
├── docs/
│   ├── architecture.md
│   ├── design-decisions.md
│   ├── experiments.md
│   ├── filesystem-model.md
│   ├── lessons-learned.md
│   ├── limitations.md
│   └── troubleshooting.md
│
├── CHANGELOG.md
├── Makefile
├── README.md
└── .gitignore
```

## Directory responsibilities

### `src/`

Contains the actual implementation.

The current entry point is:

```text
src/main.c
```

The implementation currently remains in one source file because the program is still small.

Additional source files will only be introduced when there are genuine implementation boundaries worth separating.

### `include/`

Contains shared C header files when the implementation eventually requires them.

It is intentionally empty at the current stage.

### `tests/`

Contains automated verification of expected program behavior.

Current test:

```text
tests/test_info.sh
```

Tests are separate from the implementation so that the program can be verified externally rather than relying only on manual inspection.

### `experiments/`

Contains controlled filesystem experiments.

Experiments are used to investigate Linux behavior that may not be obvious from documentation or source code alone.

Examples will include:

* `stat()` versus `lstat()`
* symbolic links
* inode identity
* hard links
* directory size
* filesystem boundaries
* mount points
* permission behavior
* directory traversal

### `docs/`

Contains the accumulated engineering knowledge produced while developing the project.

The documentation is divided by purpose rather than placing everything into the README.

### `CHANGELOG.md`

Records meaningful changes to the project over time.

### `Makefile`

Defines the reproducible build and test workflow.

### `README.md`

Provides the high-level project entry point, current capabilities, architecture, goals, and development direction.

---

# Current Program

The current command is:

```bash
./fswatch info <path>
```

For example:

```bash
./fswatch info .
```

```bash
./fswatch info src
```

```bash
./fswatch info README.md
```

Example output:

```text
Size: 4096 bytes
Type: directory
```

or:

```text
Size: 0 bytes
Type: regular file
```

The exact size depends on the filesystem object being inspected.

---

# How It Currently Works

At the current stage, the program follows this path:

```text
User
 │
 │ ./fswatch info README.md
 │
 ▼
main()
 │
 │ validate arguments
 │
 ▼
stat()
 │
 │ pathname
 ▼
Linux kernel
 │
 │ filesystem lookup
 │
 ▼
struct stat
 │
 ├── st_size
 │
 └── st_mode
 │
 ▼
fswatch
 │
 └── print filesystem information
```

The central Linux interface currently being studied is:

```c
stat()
```

provided by:

```c
#include <sys/stat.h>
```

`stat()` retrieves metadata about a filesystem object and stores that metadata in a:

```c
struct stat
```

---

# Filesystem Model

One of the first concepts established by this project is that a pathname and a filesystem object are not the same thing.

For example:

```text
README.md
```

is a pathname used to locate an object.

The object itself has metadata maintained by the filesystem.

Conceptually:

```text
pathname
    ↓
filesystem namespace lookup
    ↓
filesystem object
    ↓
metadata
```

The `stat()` interface exposes part of that metadata to a userspace program.

---

# `struct stat`

The Linux `struct stat` structure contains information about a filesystem object.

Important fields include:

```text
st_dev
st_ino
st_mode
st_nlink
st_uid
st_gid
st_size
st_blksize
st_blocks
timestamps
```

The project will investigate these fields incrementally.

At the current stage, only:

```text
st_size
st_mode
```

are being used.

---

# File Size

The current implementation reports:

```c
st.st_size
```

This represents the size associated with the filesystem object.

For a regular file, this normally corresponds to the file's logical byte length.

For example:

```text
README.md
Size: 1520 bytes
```

does not necessarily mean the filesystem consumes exactly 1520 bytes of physical storage.

Logical file size and filesystem storage consumption are different concepts.

This distinction will become important when the project investigates:

```text
st_blocks
du
filesystem block allocation
sparse files
```

---

# Directories Are Filesystem Objects

A directory is not simply an abstract container provided by the shell.

It is itself a filesystem object.

Therefore:

```bash
./fswatch info .
```

can return something such as:

```text
Size: 4096 bytes
Type: directory
```

The reported directory size does **not** mean:

```text
total size of every file inside the directory
```

It represents the directory object itself.

This distinction will be investigated experimentally.

---

# File Types

Linux supports multiple filesystem object types.

The project will eventually distinguish:

```text
regular file
directory
symbolic link
character device
block device
FIFO
socket
```

The file type is encoded within:

```c
st_mode
```

The `<sys/stat.h>` macros are used to interpret it:

```c
S_ISREG()
S_ISDIR()
S_ISLNK()
S_ISCHR()
S_ISBLK()
S_ISFIFO()
S_ISSOCK()
```

The current implementation only handles a subset.

The project will expand this incrementally.

---

# Symbolic Links

Symbolic links are an important filesystem concept because the pathname being examined may not directly identify the final filesystem object.

For example:

```text
test-link -> README.md
```

There are now two filesystem objects involved:

```text
test-link
    │
    └──────► README.md
```

The behavior of:

```c
stat()
```

and:

```c
lstat()
```

differs in this situation.

`stat()` follows the symbolic link and reports metadata for the target.

`lstat()` reports metadata for the symbolic link itself.

This behavior will be demonstrated through controlled experiments rather than treated only as a definition.

---

# Inodes

One of the major upcoming concepts is the inode.

An inode is a filesystem data structure containing metadata about a filesystem object.

The project will investigate:

```text
st_ino
```

and:

```text
st_dev
```

to understand filesystem object identity.

This will lead into:

```text
inode
   ↓
hard links
   ↓
symbolic links
   ↓
pathname versus object identity
```

This is particularly important because multiple pathnames can refer to the same underlying filesystem object.

---

# Ownership and Permissions

The project will eventually inspect:

```text
st_uid
st_gid
st_mode
```

to understand:

* user ownership
* group ownership
* read permission
* write permission
* execute permission
* special permission bits
* how permissions are represented internally
* how Linux uses these values during access checks

This will connect filesystem metadata to Linux security behavior.

---

# Timestamps

The project will eventually investigate filesystem timestamps, including:

```text
access time
modification time
status-change time
```

These timestamps will be examined experimentally because their behavior depends on the operation being performed and the filesystem configuration.

---

# Directory Traversal

`stat()` allows the program to inspect a known pathname.

The next major filesystem interface after metadata inspection will be directory traversal.

The project will investigate:

```c
opendir()
readdir()
closedir()
```

Conceptually:

```text
directory
    ↓
opendir()
    ↓
DIR *
    ↓
readdir()
    ↓
directory entries
    ↓
individual pathnames
```

This will eventually allow `fswatch` to inspect directory contents rather than only individual paths.

---

# Directory Entries

A directory contains entries that associate names with filesystem objects.

This introduces an important distinction:

```text
directory entry
        ≠
inode
        ≠
file contents
```

The project will investigate how these concepts relate.

This distinction becomes especially important when studying hard links.

---

# Hard Links

Hard links will be used to demonstrate that multiple directory entries can refer to the same underlying filesystem object.

Conceptually:

```text
pathname A ─────┐
                │
pathname B ─────┼──► inode ───► file data
                │
pathname C ─────┘
```

The project will use:

```text
st_ino
st_nlink
```

to observe this relationship.

---

# Symbolic Links Versus Hard Links

The project will experimentally compare:

```text
hard link
```

and:

```text
symbolic link
```

The investigation will cover:

* inode identity
* link counts
* pathname resolution
* target behavior
* deletion behavior
* filesystem boundaries
* broken symbolic links

This will provide a practical model of Linux link semantics.

---

# Mount Points and Filesystem Boundaries

A Linux system can contain multiple filesystems within one pathname namespace.

For example:

```text
/
├── etc
├── home
├── proc
├── sys
├── dev
└── ...
```

Some of these locations may represent separate mounted filesystems.

The project will eventually investigate:

```text
st_dev
```

to identify filesystem identity and understand when traversal crosses filesystem boundaries.

This is important for infrastructure tools that perform recursive filesystem operations.

---

# Storage Versus Filesystem Metadata

A major theme of this project is distinguishing concepts that appear similar from userspace.

For example:

```text
file logical size
        ≠
filesystem blocks allocated
        ≠
physical storage consumed
```

Similarly:

```text
pathname
        ≠
inode
        ≠
file contents
```

And:

```text
directory size
        ≠
recursive directory contents
```

The project will deliberately investigate these distinctions.

---

# Error Handling

Filesystem operations fail for many reasons.

Examples include:

```text
ENOENT
EACCES
ENOTDIR
ELOOP
ENAMETOOLONG
EIO
```

The program will progressively investigate filesystem-related errors and how Linux communicates them to userspace.

The current implementation uses:

```c
perror()
```

for basic error reporting.

Future versions will distinguish errors more explicitly where useful.

---

# Testing Strategy

Testing will occur at several levels.

## Unit-style behavioral tests

The shell test suite verifies expected command behavior:

```text
tests/test_info.sh
```

Examples:

```text
regular file detection
directory detection
nonexistent path handling
```

## Controlled experiments

Experiments are designed to answer specific questions about Linux behavior.

For example:

```text
Does stat() follow symbolic links?
```

The experiment should create a known filesystem state, execute the program, record the observation, and explain the result.

## Manual system comparison

Where useful, the project will compare `fswatch` behavior against standard Linux tools such as:

```text
stat
ls
find
du
file
```

These tools are used as observation and validation references.

They are not used as implementation dependencies.

---

# Documentation Strategy

The project documentation is divided according to purpose.

## `architecture.md`

Describes how the program is structured.

## `design-decisions.md`

Records implementation decisions and the reasoning behind them.

## `experiments.md`

Records controlled filesystem experiments and their results.

## `filesystem-model.md`

Builds the conceptual model of how Linux filesystems work.

## `lessons-learned.md`

Records important discoveries from implementation and experimentation.

## `limitations.md`

Documents what the current implementation does not yet handle.

## `troubleshooting.md`

Records build, runtime, and filesystem-specific problems encountered during development.

This separation prevents the README from becoming an unstructured collection of notes.

---

# Build Requirements

The project requires a Linux environment with:

* GCC (GNU Compiler Collection)
* GNU Make
* Bash
* standard Linux filesystem interfaces

The program is written in C using the C17 language standard.

The compiler is configured with:

```text
-Wall
-Wextra
-Wpedantic
-std=c17
```

Warnings are treated as information that should be investigated rather than ignored.

---

# Building

From the project root:

```bash
make
```

This produces:

```text
./fswatch
```

To perform a clean build:

```bash
make clean
make
```

---

# Running

Inspect the current directory:

```bash
./fswatch info .
```

Inspect the source directory:

```bash
./fswatch info src
```

Inspect a regular file:

```bash
./fswatch info README.md
```

Inspect a system file:

```bash
./fswatch info /etc/passwd
```

---

# Testing

Run the automated test suite:

```bash
make test
```

The test script can also be executed directly:

```bash
./tests/test_info.sh
```

If necessary:

```bash
chmod +x tests/test_info.sh
```

---

# Development Workflow

Every new capability follows approximately this workflow:

```text
1. Define the filesystem question
        ↓
2. Learn the relevant Linux interface
        ↓
3. Implement the smallest useful change
        ↓
4. Compile with warnings enabled
        ↓
5. Test expected behavior
        ↓
6. Design a controlled experiment
        ↓
7. Compare observations with the filesystem model
        ↓
8. Document the result
        ↓
9. Record the design decision
        ↓
10. Record limitations
        ↓
11. Update the changelog
        ↓
12. Continue to the next filesystem concept
```

The objective is to make every major feature leave behind both working code and engineering evidence.

---

# Planned Development Path

The project is intentionally progressive.

## Stage 1: Filesystem metadata

Current focus:

```text
stat()
struct stat
st_size
st_mode
filesystem object types
```

---

## Stage 2: Complete object type inspection

Add detection for:

```text
regular files
directories
symbolic links
character devices
block devices
FIFOs
sockets
```

---

## Stage 3: Symbolic links

Investigate:

```text
stat()
lstat()
```

and pathname resolution.

---

## Stage 4: Inodes and object identity

Investigate:

```text
st_ino
st_dev
st_nlink
```

Then experimentally demonstrate hard links.

---

## Stage 5: Ownership and permissions

Investigate:

```text
st_uid
st_gid
st_mode
```

and Linux permission semantics.

---

## Stage 6: Timestamps

Investigate filesystem timestamps and how different operations affect them.

---

## Stage 7: Directory traversal

Introduce:

```text
opendir()
readdir()
closedir()
```

and inspect directory entries.

---

## Stage 8: Recursive inspection

Build controlled recursive traversal.

Investigate:

```text
depth
cycles
symbolic links
errors
filesystem boundaries
```

---

## Stage 9: Filesystem identity

Investigate:

```text
st_dev
```

mount points and filesystem boundaries.

---

## Stage 10: Storage accounting

Compare:

```text
st_size
st_blocks
du
filesystem block allocation
```

Investigate sparse files and logical versus allocated size.

---

## Stage 11: File descriptors

Connect filesystem objects to process I/O through:

```text
open()
close()
read()
write()
```

This will connect the filesystem project to the earlier Linux process work.

---

## Stage 12: Advanced filesystem behavior

Potential areas include:

```text
/proc
/sys
/dev
pseudo-filesystems
device files
sockets
FIFOs
mount namespaces
overlay filesystems
filesystem limits
```

The exact scope will depend on what the earlier stages reveal.

---

# Relationship to Linux Infrastructure

The project is not intended to produce a production replacement for existing filesystem utilities.

Its purpose is infrastructure understanding.

Filesystem behavior appears throughout Linux infrastructure:

```text
Linux services
    ↓
configuration files
    ↓
logs
    ↓
sockets
    ↓
process I/O
    ↓
permissions
    ↓
mounts
    ↓
storage
    ↓
containers
    ↓
Kubernetes
    ↓
distributed systems
```

Understanding these foundations makes higher-level infrastructure behavior easier to reason about.

For example, container storage eventually involves concepts such as:

```text
mount namespaces
overlay filesystems
bind mounts
volume mounts
filesystem permissions
inode behavior
filesystem capacity
```

Those concepts become much easier to understand after establishing a concrete filesystem model.

---

# Project Principles

## Understand before abstracting

The implementation should remain close to the operating-system concepts being studied.

## Experiment before assuming

When Linux behavior can be observed directly, perform an experiment.

## Keep evidence

Commands, outputs, observations, and conclusions should be preserved when they contribute to the engineering understanding of the project.

## Automate repeatable verification

If a behavior can be tested automatically, add a test.

## Avoid premature complexity

A small program should remain small until its requirements justify additional structure.

## Document decisions

Important implementation choices should be explainable later.

## Record limitations

A tool is more credible when its boundaries are explicitly understood.

---

# Current Limitations

The current implementation is intentionally incomplete.

It does not yet provide:

* complete filesystem object classification
* symbolic-link-specific inspection
* inode information
* hard-link analysis
* permissions
* ownership
* timestamps
* directory traversal
* recursive inspection
* mount detection
* filesystem boundary handling
* storage allocation analysis
* detailed error classification
* configuration files
* a complex command parser

These are planned areas of investigation rather than missing production requirements.

---

# Non-Goals

`fswatch` is not intended to immediately become:

* a replacement for `find`
* a replacement for `stat`
* a replacement for `ls`
* a replacement for `du`
* a full filesystem debugger
* a production filesystem monitoring daemon

The project may eventually become a more capable diagnostic tool, but understanding Linux filesystem behavior remains the primary objective.

---

# Expected Outcome

By the end of the project, I should be able to reason from a pathname down toward the underlying filesystem concepts involved in resolving and accessing it.

The target mental model is approximately:

```text
pathname
    ↓
directory entries
    ↓
inode / filesystem object
    ↓
metadata
    ├── type
    ├── permissions
    ├── ownership
    ├── timestamps
    ├── link count
    └── size
    ↓
filesystem
    ├── blocks
    ├── mounts
    └── storage
    ↓
kernel filesystem layer
    ↓
userspace system calls
```

And from the other direction:

```text
process
    ↓
system call
    ↓
kernel
    ↓
filesystem
    ↓
filesystem object
    ↓
storage
```

The purpose of `fswatch` is to make those relationships concrete.

---

# Final Principle

The project is intentionally small.

The complexity should come from **understanding Linux**, not from writing unnecessarily complicated code.

Each feature should answer a real filesystem question.

Each experiment should produce evidence.

Each design decision should have a reason.

Each limitation should be explicit.

The result should be more than a working C program. It should be a practical record of how Linux filesystems actually behave.
