
# Design Decisions

This document records important engineering decisions made during the development of `fswatch`.

The purpose is to preserve the reasoning behind the implementation, not just describe what the code currently does.

---

## Decision 001: Start With a Small Single-File Implementation

### Decision

Keep the initial implementation in `src/main.c`.

### Context

The project is primarily a filesystem learning exercise.

The first version only needs to:

1. accept a command and path,
2. call `stat()`,
3. inspect the returned `struct stat`,
4. report basic filesystem metadata.

Creating multiple source files at this stage would introduce structure without introducing meaningful separation of responsibility.

### Reasoning

A small implementation makes the relationship between the command and the Linux filesystem interface visible.

For example:

```text
fswatch
   |
   v
main()
   |
   v
stat()
   |
   v
struct stat
   |
   v
filesystem metadata
````

This makes the system easier to reason about while the underlying concepts are still being learned.

### Consequence

`src/main.c` may become larger as functionality is added.

That is acceptable temporarily.

The code should only be split into additional modules when a component develops an independently meaningful responsibility.

---

## Decision 002: Use Linux/POSIX Filesystem Interfaces Directly

### Decision

Build filesystem functionality around the operating system interfaces themselves rather than wrapping existing command-line utilities.

Examples include:

```c
stat()
lstat()
opendir()
readdir()
closedir()
open()
read()
write()
close()
```

### Context

The purpose of the project is to understand how Linux filesystem operations work.

Calling external commands such as:

```bash
stat
ls
find
du
```

would hide the operating system interface that the project is intended to study.

### Reasoning

Using the C interfaces forces the implementation to interact with the filesystem model directly.

For example:

```c
struct stat st;

if (stat(path, &st) == -1) {
    perror("stat");
    return 1;
}
```

This exposes the relationship between:

```text
path
  |
  v
system call interface
  |
  v
kernel
  |
  v
VFS
  |
  v
filesystem implementation
  |
  v
filesystem object
  |
  v
metadata
```

The implementation therefore becomes part of the learning process.

### Consequence

The program will initially provide less functionality than mature Linux utilities.

That is intentional.

The project is not intended to recreate `stat`, `find`, `du`, or other standard utilities feature-for-feature.

---

## Decision 003: Keep Experiments Separate From Automated Tests

### Decision

Filesystem experiments live under:

```text
experiments/
```

while repeatable correctness checks live under:

```text
tests/
```

### Context

Experiments and tests serve different purposes.

An experiment is designed to answer a systems question.

A test is designed to determine whether the program continues to behave according to an expected contract.

### Example

Experiment:

```text
Do two different pathnames reference the same filesystem object
when they are hard links?
```

Test:

```text
Does `fswatch info <path>` correctly identify a regular file?
```

The experiment may produce observations that change understanding.

The test should remain stable unless the program's intended behavior changes.

### Reasoning

Combining the two would make the repository harder to understand.

Experiments are allowed to be exploratory.

Tests should be deterministic and repeatable.

### Consequence

A successful experiment does not automatically become a test.

Instead, an observation can later lead to a new implementation requirement and then to an automated test.

---

## Decision 004: Treat Documentation as Part of the Engineering Work

### Decision

Important technical reasoning, observations, limitations, experiments, and troubleshooting information are documented inside the repository.

### Context

The project is intended to demonstrate engineering understanding, not only working code.

Filesystem behavior can be difficult to remember because many concepts are related:

```text
pathname
directory entry
inode
filesystem
device
file descriptor
storage blocks
mount point
symbolic link
hard link
```

The repository therefore needs to preserve the mental model developed during implementation.

### Reasoning

Documentation answers questions that source code alone cannot answer.

For example:

* Why was `stat()` used?
* Why does the tool not call `ls`?
* Why are experiments separate from tests?
* Why does a directory report a size such as 4096 bytes?
* Why can two pathnames have the same inode?
* Why does removing a hard link not necessarily remove the underlying object?
* Why was `_DEFAULT_SOURCE` required?

These are engineering decisions and observations, not implementation details alone.

### Consequence

Documentation should be updated when an important implementation decision or filesystem concept changes.

The repository should remain understandable to someone reading it without the original conversation.

---

## Decision 005: Avoid Premature Abstraction

### Decision

Do not introduce helper modules, libraries, or abstractions until there is a concrete responsibility that justifies them.

### Context

The repository currently contains:

```text
src/main.c
include/
```

The `include/` directory may remain empty until shared interfaces become necessary.

### Reasoning

Abstraction should solve a real problem.

For example, splitting code into:

```text
src/main.c
src/stat.c
src/output.c
src/filesystem.c
src/util.c
```

before those components have meaningful independent responsibilities would increase the amount of code that must be understood without improving the design.

The project should first establish the filesystem behavior.

Structure can then evolve from actual requirements.

### Consequence

The repository may initially look simpler than a production-scale C project.

That is deliberate.

As functionality grows, modules should be introduced when they improve:

* responsibility boundaries,
* testability,
* readability,
* reuse,
* maintainability.

---

## Decision 006: Use `struct stat` as the Initial Metadata Boundary

### Decision

The first filesystem inspection functionality is based on the metadata returned by `stat()`.

### Context

Linux exposes a significant amount of filesystem metadata through `struct stat`.

The current implementation already uses:

```c
st.st_size
st.st_mode
```

Future functionality will use additional fields such as:

```c
st.st_dev
st.st_ino
st.st_nlink
st.st_uid
st.st_gid
st.st_atime
st.st_mtime
st.st_ctime
```

where appropriate.

### Reasoning

`struct stat` provides a useful boundary between the program and the kernel filesystem interface.

Instead of immediately implementing traversal, storage analysis, or file I/O, the project can first understand the metadata associated with a filesystem object.

This gives the project a natural progression:

```text
path
  |
  v
stat()
  |
  v
struct stat
  |
  +--> type
  +--> size
  +--> device
  +--> inode
  +--> link count
  +--> ownership
  +--> permissions
  +--> timestamps
```

### Consequence

The `info` command will gradually expose more of the metadata available through `struct stat`.

Additional filesystem operations will be introduced separately when they represent a new concept.

---

## Decision 007: Distinguish Path Identity From Filesystem Object Identity

### Decision

The project treats a pathname and the filesystem object referenced by that pathname as different concepts.

### Context

Experiment 002 demonstrated that:

```text
original.txt
hardlink.txt
```

can refer to the same filesystem object.

Both names had the same:

```text
st_dev
st_ino
```

and both reported the same link count.

### Reasoning

This distinction is fundamental to understanding Linux filesystems.

A pathname belongs to the namespace.

The inode identifies the filesystem object within its filesystem.

A useful conceptual model is:

```text
directory entry
      |
      v
   pathname
      |
      v
inode / filesystem object
      |
      +--> metadata
      |
      +--> file data
```

Therefore, two different pathnames do not necessarily mean two different objects.

### Consequence

Future functionality must avoid assuming that pathname equality and filesystem-object equality are the same thing.

This becomes particularly important when implementing:

* hard-link detection,
* recursive traversal,
* duplicate-object detection,
* filesystem boundary detection,
* symbolic-link handling.

---

## Decision 008: Treat `stat()` and `lstat()` as Different Operations

### Decision

The project will explicitly distinguish between `stat()` and `lstat()` when symbolic links are introduced.

### Context

`stat()` follows a symbolic link and reports information about its target.

`lstat()` reports information about the symbolic link itself.

Therefore:

```text
stat(path)
```

and:

```text
lstat(path)
```

can describe different filesystem objects when `path` is a symbolic link.

### Reasoning

Symbolic links expose an important distinction between:

```text
the pathname being inspected
```

and:

```text
the object ultimately reached through that pathname
```

Understanding this distinction is necessary before implementing reliable filesystem traversal.

### Consequence

Symbolic-link handling will be treated as a filesystem semantics issue rather than merely another file type check.

---

## Decision 009: Build Behavior From Observations

### Decision

When filesystem behavior is unclear, create a controlled experiment before implementing assumptions.

### Context

Filesystem behavior often looks obvious until edge cases are encountered.

Examples include:

* hard links,
* symbolic links,
* directory sizes,
* sparse files,
* mount points,
* deleted-but-open files,
* filesystem boundaries.

### Reasoning

A controlled experiment allows the project to establish what Linux actually does.

The preferred workflow is:

```text
Question
   |
   v
Hypothesis
   |
   v
Controlled experiment
   |
   v
Observation
   |
   v
Explanation
   |
   v
Implementation
   |
   v
Automated test
```

This is more useful for systems engineering than implementing behavior from memory and testing only the final result.

### Consequence

The `experiments/` directory is part of the engineering process.

Experiments should remain reproducible and should document the commands, observations, and conclusions.

---

## Decision 010: Prefer Reproducible Builds

### Decision

The project uses a Makefile as the primary build interface.

The expected workflow is:

```bash
make
make test
make clean
```

### Context

The project should be buildable without relying on undocumented shell commands.

### Reasoning

A reproducible build provides a consistent entry point for:

* development,
* testing,
* debugging,
* future automation.

The Makefile also makes compiler flags explicit:

```text
-Wall
-Wextra
-Wpedantic
-std=c17
```

These flags help expose incorrect assumptions and compiler issues early.

### Consequence

Build configuration belongs in the repository rather than only in personal shell history.

---

## Decision 011: Compiler Warnings Are Part of the Development Process

### Decision

Warnings are treated as development feedback rather than something to suppress.

### Context

The project uses:

```text
-Wall -Wextra -Wpedantic
```

and explicitly targets:

```text
C17
```

### Reasoning

Systems programming depends heavily on precise types, interfaces, memory behavior, and platform APIs.

Compiler warnings can reveal:

* missing declarations,
* incorrect types,
* incompatible interfaces,
* suspicious conversions,
* incomplete code.

For example, the use of `S_ISSOCK()` exposed a feature-visibility issue when compiling under strict C17 settings on glibc.

The solution was to explicitly define:

```c
#define _DEFAULT_SOURCE
```

rather than weakening the compiler configuration.

### Consequence

The project should prefer fixing the cause of a compiler warning over disabling the warning.

Platform-specific behavior should be documented when it affects portability or compilation.

---

## Decision 012: Keep the Project Scope Focused on Filesystem Understanding

### Decision

New functionality must contribute directly to understanding Linux filesystem behavior.

### Context

It would be easy to expand the project into a general-purpose system utility.

That is not the goal.

### Reasoning

The project exists inside a larger infrastructure engineering learning path.

The immediate objective is to understand the Linux filesystem deeply enough to reason about higher-level infrastructure systems.

The project therefore prioritizes concepts such as:

```text
filesystem objects
metadata
inodes
directory entries
links
permissions
ownership
timestamps
directory traversal
mounts
storage allocation
file descriptors
pseudo-filesystems
```

### Consequence

Features should be added because they teach or demonstrate an important filesystem concept.

Features that merely make the program look more complete should not drive the roadmap.

---

## Current Architectural Direction

The decisions above produce the following development model:

```text
                 fswatch
                    |
                    v
             command interface
                    |
                    v
              filesystem API
                    |
                    v
             kernel / VFS
                    |
                    v
          filesystem implementation
                    |
                    v
               storage
```

The metadata inspection path currently looks like:

```text
path
 |
 v
stat()
 |
 v
struct stat
 |
 +---- st_mode  ---> object type
 |
 +---- st_size  ---> logical size
 |
 +---- st_dev   ---> filesystem identity
 |
 +---- st_ino   ---> object identity
 |
 +---- st_nlink ---> number of directory references
```

The next implementation step is therefore to expose:

```text
Device
Inode
Links
Size
Type
```

from the existing `struct stat` result.

This follows directly from the experiments already completed rather than introducing unrelated functionality.
