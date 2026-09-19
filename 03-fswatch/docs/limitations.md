
# Filesystem Limitations

## Current Scope

`fswatch` is currently a small filesystem inspection program designed for learning Linux filesystem behavior.

It is not intended to replace standard Linux utilities such as:

```text
stat
ls
find
readlink
df
du
````

The purpose is to understand the interfaces and filesystem concepts underneath those tools.

---

## Current Implementation

The current `info` command reports:

```text
device
inode
link count
size
file type
```

The implementation currently uses `stat()` for filesystem metadata.

This means symbolic links are currently followed.

For example:

```text
link.txt -> target.txt
```

will cause:

```text
fswatch info link.txt
```

to report metadata for `target.txt` rather than the symbolic-link object itself.

This behavior has been experimentally verified.

The next implementation change will use `lstat()` for the primary object inspection and `readlink()` to expose the symbolic-link target.

---

## Symbolic Links

The project now has a reproducible experiment covering:

```text
stat()
lstat()
readlink()
```

The experiment demonstrated:

* `stat()` follows symbolic links
* `lstat()` inspects the symbolic link itself
* `readlink()` retrieves the stored target pathname
* a symbolic link has its own inode and metadata
* a broken symbolic link can still be inspected with `lstat()`
* `stat()` fails when the symbolic link target cannot be resolved

The experiment is located at:

```text
experiments/003-symbolic-links/
```

The implementation has not yet fully exposed these distinctions through `fswatch`.

---

## Metadata Not Yet Exposed

Although `struct stat` contains substantially more information, `fswatch` does not currently expose all of it.

Not yet exposed:

* owner UID
* group GID
* permissions
* access time
* modification time
* status-change time
* block count
* filesystem block size
* device numbers for special files

These will be added only when they serve the learning progression.

---

## Directory Traversal

`fswatch` does not currently traverse directories.

When given a directory, it reports metadata about the directory object itself.

It does not recursively calculate:

```text
total file size
total allocated storage
number of files
number of directories
```

This is intentional.

A directory's `st_size` should not be interpreted as the total size of its contents.

---

## Filesystem Storage

The current implementation does not distinguish between:

```text
logical file size
allocated disk blocks
filesystem capacity
```

The project has observed the distinction through `struct stat`, but has not yet implemented dedicated storage-accounting functionality.

In particular, the project has not yet investigated:

* sparse files
* block allocation
* filesystem free space
* filesystem usage
* `du`
* `df`

---

## File Descriptors and I/O

The project has not yet implemented direct file I/O using:

```text
open()
read()
write()
close()
```

The current implementation primarily operates through pathname-based metadata interfaces.

File descriptors and their relationship with open file descriptions will be investigated later.

This will also allow the project to investigate behavior such as:

```text
deleted-but-open files
file offsets
shared open file descriptions
```

---

## Mount Boundaries

`fswatch` does not currently identify filesystem mount boundaries.

The `st_dev` field has been observed and used as part of filesystem object identity, but mount traversal behavior has not yet been investigated.

Future experiments will examine:

```text
mount points
different filesystems
device boundaries
filesystem traversal
```

---

## Pseudo-Filesystems

Pseudo-filesystems such as:

```text
/proc
/sys
/dev
```

have not yet been explored as part of the project.

These will be important because they demonstrate that filesystem interfaces can expose objects that do not behave like ordinary persistent files on disk.

---

## Error Handling

The current program has basic error handling using:

```c
perror()
```

Error handling has not yet been developed into a structured error model.

The project will eventually investigate errors such as:

```text
ENOENT
EACCES
ENOTDIR
ELOOP
EIO
```

through controlled experiments.

Symbolic links have already demonstrated one important error case:

```text
stat()
```

can fail on a broken symbolic link even though:

```text
lstat()
readlink()
```

can still successfully inspect the link.

---

## Testing Limitations

The current automated test suite verifies basic `fswatch` behavior.

It currently covers:

* regular file detection
* regular file metadata
* directory detection
* directory metadata
* nonexistent paths

Symbolic-link behavior is being added as a separate implementation and test stage.

The test suite will eventually cover:

* valid symbolic links
* broken symbolic links
* file permissions
* ownership
* timestamps
* directory traversal
* filesystem boundaries
* I/O behavior
* relevant error conditions

---

## Scope Boundary

The project deliberately avoids becoming a clone of existing Linux utilities.

The goal is:

```text
observe Linux behavior
        ↓
understand the underlying model
        ↓
implement a small representation
        ↓
test the behavior
        ↓
document what was learned
```

Features are therefore added when they deepen understanding of Linux filesystem behavior rather than simply increasing the number of commands supported.

---

## Current Engineering Constraint

The implementation should remain small enough that the relationship between:

```text
Linux system call
        ↓
kernel behavior
        ↓
returned data
        ↓
fswatch output
```

remains visible.

Premature abstraction is therefore avoided.

The project is intentionally being built incrementally around concrete filesystem behavior.

---

## Current Limitation Summary

At the current stage, `fswatch` is intentionally incomplete.

It can inspect basic filesystem metadata, but it does not yet provide:

```text
complete stat information
symbolic-link-aware inspection
ownership inspection
permission analysis
timestamp analysis
directory traversal
recursive traversal
mount detection
filesystem capacity information
storage allocation analysis
file descriptor analysis
direct file I/O
pseudo-filesystem analysis
structured error reporting
```

These are not accidental omissions.

They represent the remaining filesystem concepts that will be investigated and implemented progressively.

