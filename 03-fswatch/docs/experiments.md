

````markdown
# Filesystem Experiments

This document records controlled experiments performed during the development of `fswatch`.

The purpose of these experiments is to understand Linux filesystem behavior from first principles and compare the behavior observed by `fswatch` with native Linux utilities and system interfaces.

Experiments are separate from automated tests.

- Tests verify that `fswatch` behaves as expected.
- Experiments investigate how Linux behaves.
- Observations are recorded from the actual system.
- Conclusions are based on observed behavior and documented system interfaces.

---

## Experiment 001: `stat()` Metadata

### Objective

Understand what information Linux exposes about a filesystem object through the `stat()` system interface.

The experiment compares the metadata reported by:

```text
fswatch
````

and:

```text
stat
```

The experiment uses both a regular file and a directory.

---

### System Under Test

Project:

```text
03-fswatch
```

Program:

```text
./fswatch
```

Linux utility:

```text
stat
```

Primary C interface:

```c
stat()
```

Primary data structure:

```c
struct stat
```

---

## Experiment Setup

The experiment script is:

```text
experiments/001-stat-metadata.sh
```

Make the script executable:

```bash
chmod +x experiments/001-stat-metadata.sh
```

Run it against a regular file:

```bash
./experiments/001-stat-metadata.sh README.md
```

Run it against the current directory:

```bash
./experiments/001-stat-metadata.sh .
```

---

## Expected Questions

The experiment is intended to answer the following questions:

1. Does `fswatch` report the same logical file size as Linux `stat`?
2. How does Linux represent a directory?
3. Is a directory's `st_size` the total size of everything inside it?
4. What is the relationship between `st_dev` and `st_ino`?
5. What other metadata exists inside `struct stat`?
6. Does a pathname directly represent the filesystem object?
7. What information is currently missing from `fswatch`?

---

# Regular File

## Target

```text
README.md
```

## Observed Metadata

```text
st_size    = 23338 bytes
st_blocks  = 48
st_blksize = 4096
st_dev     = 8,2
st_ino     = 401824
st_nlink   = 1
st_mode    = 0664
```

The filesystem utility reports the same logical size as `fswatch`.

```text
fswatch -> 23338 bytes
stat    -> 23338 bytes
```

The complete `stat` output observed during the experiment was:

```text
File: README.md
size: 23338
Blocks: 48
IO Block: 4096
regular file
Device: 8,2
Inode: 401824
Links: 1
Access: (0664/-rw-rw-r--)
Uid: (1000/gmma)
Gid: (1000/gmma)
```

Observed timestamps included:

```text
Access: 2026-09-16 07:28:56.479725662 +0100
Modify: 2026-09-16 07:28:56.436145447 +0100
Change: 2026-09-16 07:28:56.436145447 +0100
Birth: 2026-09-15 08:37:45.809904023 +0100
```

---

# Directory

## Target

```text
.
```

## Observed Metadata

```text
st_size    = 4096 bytes
st_blocks  = 8
st_blksize = 4096
st_dev     = 8,2
st_ino     = 401809
st_nlink   = 7
st_mode    = 0775
```

The complete `stat` output observed during the experiment was:

```text
File: .
size: 4096
Blocks: 8
IO Block: 4096
directory
Device: 8,2
Inode: 401809
Links: 7
Access: (0775/drwxrwxr-x)
Uid: (1000/gmma)
Gid: (1000/gmma)
```

Observed timestamps included:

```text
Access: 2026-09-17 07:48:26.933950504 +0100
Modify: 2026-09-16 07:00:34.921309440 +0100
Change: 2026-09-16 07:00:34.921309440 +0100
Birth: 2026-09-15 08:37:10.549737167 +0100
```

---

# Observation 1: `st_size`

For the regular file:

```text
st_size = 23338 bytes
```

For the directory:

```text
st_size = 4096 bytes
```

The important distinction is that the directory's `st_size` does **not** represent the combined size of all files and directories beneath it.

It represents the size of the directory filesystem object itself.

Therefore:

```text
directory st_size != recursive directory contents size
```

This demonstrates that `st_size` must be interpreted according to the type and semantics of the filesystem object being inspected.

---

# Observation 2: Logical Size vs Allocated Storage

The regular file reported:

```text
st_size   = 23338 bytes
st_blocks = 48
```

These values represent different concepts.

`st_size` describes the logical size of the file.

`st_blocks` describes filesystem storage allocation in units defined by the interface.

Therefore:

```text
logical file size != allocated storage
```

This distinction will become more important when investigating:

* sparse files
* block allocation
* filesystem overhead
* `du`
* disk usage
* storage accounting

The current version of `fswatch` reports only `st_size`.

---

# Observation 3: Filesystem Identity

Both the regular file and directory reported:

```text
st_dev = 8,2
```

This indicates that both objects currently belong to the same device/filesystem identity as represented by `struct stat`.

The objects still have different inode numbers:

```text
README.md -> st_ino = 401824
.         -> st_ino = 401809
```

Therefore, within this observation:

```text
same st_dev
different st_ino
```

The combination of device identity and inode identity will become important when investigating whether two different pathnames refer to the same underlying filesystem object.

---

# Observation 4: Inode Identity

The regular file reported:

```text
st_ino = 401824
```

The directory reported:

```text
st_ino = 401809
```

The inode number is metadata associated with the filesystem object.

This provides an important distinction:

```text
pathname != inode
```

A pathname is a name used during filesystem path resolution.

The inode represents the underlying filesystem object within the filesystem's namespace and metadata model.

This distinction will be investigated further with hard links.

---

# Observation 5: Link Count

The regular file reported:

```text
st_nlink = 1
```

The directory reported:

```text
st_nlink = 7
```

The link count represents the number of hard links associated with the filesystem object.

The regular file currently has one directory entry referring to it.

The directory has a higher link count because directory link semantics include relationships involving directory entries and parent/child directory structure.

The exact semantics of directory link counts will be investigated separately rather than assumed from this single observation.

---

# Observation 6: File Type

`fswatch` currently classifies filesystem objects using the file type information stored in `st_mode`.

For the regular file:

```text
Type: regular file
```

For the directory:

```text
Type: directory
```

The underlying C checks use macros such as:

```c
S_ISREG()
S_ISDIR()
S_ISLNK()
S_ISCHR()
S_ISBLK()
S_ISFIFO()
S_ISSOCK()
```

This demonstrates that file type is represented as metadata associated with the filesystem object.

---

# Observation 7: Permissions

The regular file reported:

```text
st_mode = 0664
```

The directory reported:

```text
st_mode = 0775
```

The human-readable permission representations were:

```text
README.md -> -rw-rw-r--
.         -> drwxrwxr-x
```

This demonstrates that `st_mode` contains both file type information and permission information.

The current `fswatch` implementation uses `st_mode` to determine file type but does not yet expose permission bits.

Permissions will be investigated as a separate filesystem metadata milestone.

---

# Observation 8: `struct stat` Contains More Information Than `fswatch` Currently Uses

The current implementation uses:

```text
st_size
st_mode
```

However, the same `struct stat` instance also provides information such as:

```text
st_dev
st_ino
st_nlink
st_uid
st_gid
st_size
st_blksize
st_blocks
timestamps
```

This means the current `fswatch` implementation is only exposing a small portion of the metadata available through `stat()`.

Future milestones will expose additional fields as each concept is studied and experimentally verified.

---

# Observation 9: Directory Objects

The directory itself has metadata:

```text
Type: directory
Size: 4096 bytes
Inode: 401809
Device: 8,2
```

This reinforces an important filesystem concept:

```text
A directory is itself a filesystem object.
```

A directory is not simply an abstract container implemented outside the filesystem.

The filesystem stores metadata for the directory object, including its inode identity, permissions, ownership, timestamps, and size.

The directory also contains mappings between names and filesystem objects.

This distinction becomes important when studying:

```text
directory entry
        |
        v
filesystem object
        |
        v
inode + metadata
```

---

# Observation 10: Pathname vs Filesystem Object

The experiment started with pathnames:

```text
README.md
.
```

Linux resolved those pathnames and returned metadata describing the corresponding filesystem objects.

Therefore, the experiment supports the following model:

```text
pathname
    |
    v
path resolution
    |
    v
directory entries
    |
    v
filesystem object
    |
    v
inode + metadata
```

The pathname is therefore a way to locate an object.

It is not the object itself.

---

# Result

The experiment confirms that `stat()` provides a metadata view of a filesystem object through `struct stat`.

The experiment established the following observations:

* `fswatch` reports the same logical `st_size` as Linux `stat`.
* Regular files and directories are both filesystem objects with metadata.
* A directory's `st_size` is not the recursive size of its contents.
* `st_size` and `st_blocks` represent different storage-related concepts.
* `st_dev` identifies the device/filesystem associated with the object.
* `st_ino` provides inode identity.
* `st_nlink` provides hard-link count information.
* `st_mode` contains file type and permission information.
* `struct stat` exposes substantially more metadata than the current `fswatch` output.
* A pathname is used to locate a filesystem object but is not the object itself.

---

# Engineering Conclusions

The current implementation is intentionally small.

At this stage:

```text
fswatch info <path>
```

answers:

```text
What is this filesystem object?
What is its logical size?
```

It does not yet answer:

```text
Which inode is this?
Which filesystem is it on?
How many hard links refer to it?
Who owns it?
What permissions does it have?
When was it accessed or modified?
How much storage is allocated?
Is it a symbolic link?
```

Those questions will be implemented only after the underlying filesystem concepts have been studied and experimentally verified.

This keeps the project aligned with the first-principles learning goal rather than turning `fswatch` into a collection of unrelated `stat()` fields.

---

# Next Experiment

The next experiment will investigate inode identity and hard links.

The experiment will create two different pathnames referring to the same filesystem object:

```text
original.txt
hardlink.txt
```

The following metadata will then be compared:

```text
st_dev
st_ino
st_nlink
st_size
```

The primary question will be:

```text
Can two different pathnames refer to the same underlying filesystem object?
```

This experiment will establish the relationship between:

```text
pathname
directory entry
inode
hard link
filesystem identity
```


```
```
