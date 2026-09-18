
# Filesystem Experiments

This directory contains controlled experiments used to understand Linux filesystem behavior.

The experiments are separate from the main `fswatch` implementation and automated tests.

The purpose is to observe filesystem behavior directly, record what happens, and use those observations to guide implementation decisions.

## Why Experiments Exist

Filesystem behavior can be easy to misunderstand when learned only from API descriptions.

For example:

- a pathname is not the same thing as a filesystem object,
- multiple names can refer to the same inode,
- symbolic links have their own filesystem objects,
- `stat()` and `lstat()` can return different objects for the same pathname,
- deleting a pathname does not necessarily destroy the underlying filesystem object,
- directory size does not represent the total size of the files contained within it.

Experiments provide observable evidence for these concepts.

Each experiment should:

1. Start from a known filesystem state.
2. Perform a small number of controlled operations.
3. Observe the resulting state.
4. Record the relevant system calls or utilities.
5. Explain the result using the filesystem model.
6. Be reproducible.
7. Leave behind documentation.

## Experiments vs Tests

Experiments and tests serve different purposes.

### Experiments

Experiments are used to answer engineering questions.

They investigate behavior and help build understanding.

Examples:

- What happens to inode numbers when two names are created with a hard link?
- Does writing through a hard link modify the original file?
- What does `stat()` report for a symbolic link?
- What does `lstat()` report for the same symbolic link?
- What happens when the target of a symbolic link is removed?
- What does the kernel report as the size of a directory?

Experimental results may contain environment-dependent values such as:

- inode numbers,
- device numbers,
- timestamps,
- filesystem block sizes,
- file sizes,
- ownership information.

The important result is the observed relationship and behavior, not a specific environment-dependent number.

### Tests

Tests verify that the `fswatch` implementation continues to behave according to an already established design.

Tests should therefore contain stable assertions.

For example:

```text
expected:
Type: regular file
````

is appropriate for a test.

A specific inode number such as:

```text
Inode: 397277
```

is generally not appropriate because it can change between runs.

## Experiment 001: `stat()` Metadata

### Objective

Understand the metadata returned by the Linux `stat` utility and the relationship between that output and the C `struct stat` used by `fswatch`.

### Files

```text
experiments/001-stat-metadata.sh
```

### Method

The experiment compares:

```text
./fswatch info <path>
```

with:

```text
stat <path>
```

The comparison is performed against both a regular file and a directory.

### Observations

For a regular file, `fswatch` reported values corresponding to fields from `struct stat`:

* device
* inode
* link count
* size
* file type

The Linux `stat` utility reported additional metadata including:

* permissions
* owner
* group
* timestamps
* allocated blocks
* filesystem block size

For the observed `README.md`:

```text
size: 23338 bytes
type: regular file
links: 1
```

For the observed project directory:

```text
size: 4096 bytes
type: directory
```

### Important Result

`st_size` does not mean "total size of everything inside this directory."

For a directory, it describes the size of the directory filesystem object itself.

Recursive directory contents require separate traversal.

### Engineering Lesson

`struct stat` provides a metadata boundary between the application and the kernel filesystem interface.

The current implementation intentionally exposes only a subset of the available metadata.

---

## Experiment 002: Hard Links

### Objective

Understand filesystem object identity and determine whether two different pathnames can refer to the same underlying filesystem object.

### Files

```text
experiments/002-hard-links/
├── README.md
└── original.txt
```

The experiment creates `hardlink.txt` during execution.

### Method

The experiment:

1. Creates `original.txt`.
2. Creates a hard link named `hardlink.txt`.
3. Compares both objects with `ls -li`.
4. Compares device and inode information with `stat`.
5. Writes through `hardlink.txt`.
6. Reads through `original.txt`.
7. Removes `hardlink.txt`.
8. Inspects the remaining link.

### Observations

Both pathnames reported the same device and inode:

```text
name=original.txt dev=2050 inode=397192 links=2 size=31
name=hardlink.txt dev=2050 inode=397192 links=2 size=31
```

The exact device and inode values are environment-dependent.

Writing through `hardlink.txt` changed the contents visible through `original.txt`.

The link count changed from:

```text
2
```

to:

```text
1
```

after `hardlink.txt` was removed.

### Filesystem Model

The important model is:

```text
original.txt ─────┐
                  |
                  v
              inode/object
                  |
                  v
               file data
                  ^
                  |
hardlink.txt ────┘
```

The two names are separate directory entries.

They refer to the same underlying filesystem object.

### Important Result

A hard link is not a copy of a file.

It is another directory entry referring to the same filesystem object.

The `(st_dev, st_ino)` pair can therefore be used as an important object-identity signal within the filesystem model.

### Engineering Lessons

The experiment established several important distinctions:

* pathname identity is different from object identity,
* inode identity can be shared by multiple pathnames,
* `st_nlink` represents the number of hard links,
* modifying one hard-link pathname modifies the same underlying object,
* removing one pathname does not necessarily remove the object.

---

## Experiment 003: Symbolic Links

### Objective

Understand how Linux represents symbolic links and how `stat()`, `lstat()`, and `readlink()` behave when operating on them.

The experiment also examines what happens when the target of a symbolic link is removed.

### Files

```text
experiments/003-symbolic-links/
├── README.md
├── compare.c
└── run.sh
```

Generated during execution:

```text
target.txt
link.txt
compare
```

The generated files are removed automatically when the experiment finishes.

### Method

The experiment:

1. Compiles `compare.c`.
2. Creates `target.txt`.
3. Creates `link.txt` as a symbolic link to `target.txt`.
4. Inspects both objects with `ls -li`.
5. Calls `stat()` on `target.txt`.
6. Calls `lstat()` on `target.txt`.
7. Calls `readlink()` on `target.txt`.
8. Calls `stat()` on `link.txt`.
9. Calls `lstat()` on `link.txt`.
10. Calls `readlink()` on `link.txt`.
11. Removes `target.txt`.
12. Inspects the remaining symbolic link.
13. Repeats `stat()`, `lstat()`, and `readlink()` on the broken symbolic link.

### Target File

`target.txt` is a regular file.

For the observed run:

```text
type: regular file
size: 25 bytes
links: 1
```

`stat()` and `lstat()` returned the same inode because the pathname refers directly to a regular file.

`readlink()` failed because the object is not a symbolic link.

### Symbolic Link While Target Exists

`link.txt` was created as a symbolic link to `target.txt`.

The target and symbolic link had different inode numbers.

Example:

```text
target.txt
inode: 397277
type: regular file

link.txt
inode: 397278
type: symbolic link
```

The exact inode numbers can change between runs.

The symbolic link reported a size of 10 bytes because its stored pathname was:

```text
target.txt
```

The size therefore represents the stored pathname length in this experiment, not the size of the target file.

### `stat()` Behavior

Calling:

```c
stat("link.txt", &st);
```

returned the metadata of `target.txt`.

The reported inode matched the target.

This demonstrates that `stat()` follows the final symbolic link.

Conceptually:

```text
link.txt
    |
    v
follow symbolic link
    |
    v
target.txt
    |
    v
return target metadata
```

### `lstat()` Behavior

Calling:

```c
lstat("link.txt", &st);
```

returned metadata for the symbolic link itself.

The reported inode differed from the target.

Conceptually:

```text
link.txt
    |
    v
inspect symbolic-link object
    |
    v
return symlink metadata
```

### `readlink()` Behavior

Calling:

```c
readlink("link.txt", buffer, ...);
```

returned:

```text
target.txt
```

`readlink()` reads the pathname stored by the symbolic link.

It does not follow the link to obtain the target's metadata.

Conceptually:

```text
link.txt
    |
    v
read stored pathname
    |
    v
"target.txt"
```

### Broken Symbolic Link

After `target.txt` was removed, `link.txt` remained.

`stat()` failed because following the link could no longer resolve the target.

`lstat()` continued to report the symbolic link itself.

`readlink()` continued to return:

```text
target.txt
```

This demonstrates that the symbolic link and its target are separate filesystem objects.

### Results

| Operation        | Target exists            | Target removed           |
| ---------------- | ------------------------ | ------------------------ |
| `stat(link)`     | follows target           | fails                    |
| `lstat(link)`    | returns symlink metadata | returns symlink metadata |
| `readlink(link)` | returns stored pathname  | returns stored pathname  |

### Filesystem Model

A symbolic link introduces another filesystem object:

```text
target.txt
    |
    +-- inode A
    +-- file data


link.txt
    |
    +-- inode B
    +-- stored pathname: "target.txt"
```

The target and symbolic link therefore have different object identities.

### Important Result

A symbolic link does not contain a copy of the target's file data.

It is a separate filesystem object containing a pathname that can be resolved to another object.

### Engineering Lessons

The experiment establishes the following behavior:

* `stat()` follows the final symbolic link,
* `lstat()` inspects the symbolic link itself,
* `readlink()` retrieves the stored target pathname,
* a symbolic link has its own inode,
* a symbolic link can exist even when its target does not,
* a broken symbolic link can therefore still be inspected with `lstat()` and `readlink()`.

---

## Current Filesystem Model

The experiments currently support the following model:

```text
pathname
    |
    v
path lookup
    |
    v
directory entry
    |
    v
filesystem object
    |
    +-- inode / metadata
    |
    +-- object data
```

Hard links demonstrate that multiple directory entries can refer to one filesystem object:

```text
pathname A ──┐
             |
             v
          object
             ^
             |
pathname B ──┘
```

Symbolic links introduce a separate filesystem object:

```text
pathname
    |
    v
symbolic-link object
    |
    +-- stored pathname
              |
              v
          target object
```

The distinction between pathname, directory entry, filesystem object, and object metadata is central to the project.

---

## Current System Call Model

The project is progressively moving from high-level command behavior toward direct Linux system interfaces.

Current filesystem interfaces investigated include:

```text
stat()
lstat()
readlink()
```

Planned interfaces include:

```text
opendir()
readdir()
closedir()

open()
read()
write()
close()
```

The purpose is to understand what the operating system actually provides to a userspace program.

---

## Experiment Design Principles

New experiments should follow these principles.

### 1. Start With a Question

Every experiment should answer a specific filesystem question.

### 2. Control the Initial State

Generated files and directories should be created explicitly where possible.

### 3. Observe Directly

Prefer direct system calls and standard Linux inspection tools when they help establish what actually happened.

### 4. Separate Observation From Interpretation

Record what happened before explaining why it happened.

### 5. Avoid Environment-Specific Conclusions

Values such as inode numbers and device numbers may change.

The experiment should focus on relationships and behavior.

### 6. Make Experiments Reproducible

An experiment should provide a script or clearly documented commands that allow the same observation to be reproduced.

### 7. Clean Up Generated State

Experiments should remove temporary objects when practical.

### 8. Document the Engineering Meaning

Each experiment should end with the filesystem concept or implementation decision that it establishes.

---

## Current Experiment Catalog

| Experiment | Subject                                           | Status   |
| ---------- | ------------------------------------------------- | -------- |
| 001        | `stat()` metadata                                 | Complete |
| 002        | Hard links and inode identity                     | Complete |
| 003        | Symbolic links, `stat()`, `lstat()`, `readlink()` | Complete |
| 004        | Permissions and ownership                         | Planned  |
| 005        | Timestamps                                        | Planned  |
| 006        | Directory traversal                               | Planned  |
| 007        | Recursive traversal                               | Planned  |
| 008        | Mount and filesystem boundaries                   | Planned  |
| 009        | Storage allocation                                | Planned  |
| 010        | Sparse files                                      | Planned  |
| 011        | File descriptors and open/unlinked files          | Planned  |
| 012        | Pseudo-filesystems                                | Planned  |
| 013        | Filesystem races and failure behavior             | Planned  |

---

## Relationship to `fswatch`

The experiments are used to establish the behavior that the main `fswatch` implementation should expose.

The current implementation uses:

```c
stat()
```

for filesystem inspection.

This means that a symbolic link pointing to a regular file currently appears as a regular file because `stat()` follows the final symbolic link.

The symbolic-link experiment demonstrates that this behavior is a consequence of the selected system call.

Before changing `fswatch`, the desired user-facing behavior must therefore be defined explicitly.

Potential designs include:

1. Report the target object by default.
2. Report the symbolic-link object by default.
3. Report both the symbolic-link object and the resolved target.

The implementation decision should be recorded in `docs/design-decisions.md` before the behavior is changed.

---

## Reproducibility

Run individual experiments from their respective directories.

For example:

```bash
./experiments/001-stat-metadata.sh
```

```bash
cd experiments/002-hard-links
```

```bash
cd experiments/003-symbolic-links
./run.sh
```

Experiments that create temporary filesystem objects should clean up their generated state automatically where practical.

The exact inode numbers, device numbers, timestamps, and other environment-dependent values may differ between runs.

The expected behavior and relationships should remain consistent.
