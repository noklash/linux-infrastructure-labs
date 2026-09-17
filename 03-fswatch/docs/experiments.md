
# Filesystem Experiments

This document records controlled experiments performed while developing `fswatch`.

The purpose of the experiments is to observe actual Linux filesystem behavior before implementing or documenting assumptions.

Each experiment follows this general process:

```text
question
   ↓
controlled filesystem state
   ↓
Linux observation
   ↓
interpretation
   ↓
filesystem model
   ↓
implementation
````

The experiments are separate from automated tests.

Tests verify that `fswatch` behaves as intended.

Experiments investigate how Linux itself behaves.

---

# Experiment 001: `stat()` Metadata

## Objective

Determine what filesystem metadata Linux exposes through `stat()` and compare it with the information currently reported by `fswatch`.

The experiment specifically investigates:

* logical file size
* file type
* inode identity
* filesystem/device identity
* link count
* ownership
* permissions
* timestamps
* allocated storage information
* directory metadata

---

## Setup

The experiment was performed inside the `03-fswatch` project.

The initial `fswatch` command was:

```bash
./fswatch info <path>
```

The corresponding Linux command was:

```bash
stat <path>
```

The project also contains:

```text
experiments/001-stat-metadata.sh
```

which runs both inspections against the same target.

---

## Regular File

The experiment was first run against:

```text
README.md
```

`fswatch` reported:

```text
Size: 23338 bytes
Type: regular file
```

Linux `stat` reported:

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

The logical size reported by `fswatch` matched `stat`.

This confirms that the current implementation is reading:

```c
st_size
```

from:

```c
struct stat
```

---

## Directory

The experiment was then run against:

```text
.
```

`fswatch` reported:

```text
Size: 4096 bytes
Type: directory
```

Linux `stat` reported:

```text
File: .
size: 4096
Blocks: 8
IO Block: 4096
directory

Device: 8,2
Inode: 401809
Links: 7
```

The important observation is that a directory also has a filesystem object size.

However:

```text
directory st_size
```

does not represent the total size of everything stored beneath that directory.

It represents the size associated with the directory filesystem object.

Recursive storage accounting is a separate problem.

---

## `struct stat`

The experiment showed that the current `fswatch` implementation uses only a small portion of the metadata available through `struct stat`.

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
st_atime
st_mtime
st_ctime
```

The current implementation initially used:

```text
st_size
st_mode
```

This establishes the foundation for later filesystem inspection features.

---

## Filesystem Identity

The experiment showed that both the regular file and the project directory were located on the same device:

```text
Device: 8,2
```

However, their inode numbers were different:

```text
README.md → inode 401824
.         → inode 401809
```

This demonstrates that device identity and inode identity are separate pieces of information.

A useful filesystem-level identity model is:

```text
(st_dev, st_ino)
```

This becomes particularly important when examining hard links.

---

## Logical Size Versus Allocated Storage

The experiment also exposed:

```text
st_size
st_blocks
st_blksize
```

These values represent different concepts.

For the regular file:

```text
size   = 23338 bytes
blocks = 48
IO     = 4096 bytes
```

Therefore:

```text
logical file size
```

and:

```text
physical storage allocation
```

should not be treated as the same measurement.

This distinction will be investigated more deeply with sparse files and storage experiments.

---

## Result

Experiment 001 established that:

* `stat()` exposes substantially more metadata than the current `fswatch` implementation reports.
* `st_size` provides the logical size associated with the object.
* `st_mode` provides file type and permission information.
* directories are filesystem objects and have their own metadata.
* directory `st_size` is not recursive content size.
* `st_dev` identifies the filesystem/device.
* `st_ino` identifies the inode within that filesystem.
* `st_nlink` reports link count.
* `st_blocks` relates to storage allocation.
* `struct stat` provides the foundation for filesystem inspection.

---

# Experiment 002: Inode Identity and Hard Links

## Objective

Determine whether two different pathnames can refer to the same underlying filesystem object.

The experiment investigates:

* inode identity
* filesystem/device identity
* hard links
* link counts
* shared file data
* the relationship between directory entries and filesystem objects
* what happens when one hard link is removed

---

## Setup

Create the experiment directory:

```bash
mkdir -p experiments/002-hard-links
cd experiments/002-hard-links
```

Create the original file:

```bash
printf 'filesystem identity experiment\n' > original.txt
```

Create a hard link:

```bash
ln original.txt hardlink.txt
```

---

## Initial Inspection

Inspect both names:

```bash
ls -li original.txt hardlink.txt
```

Observed:

```text
397192 -rw-rw-r-- 2 gmma gmma 31 Sep 17 08:54 hardlink.txt
397192 -rw-rw-r-- 2 gmma gmma 31 Sep 17 08:54 original.txt
```

Both pathnames reported:

```text
inode = 397192
links = 2
size = 31 bytes
```

The relevant metadata was then inspected directly:

```bash
stat -c 'name=%n dev=%d inode=%i links=%h size=%s' original.txt hardlink.txt
```

Observed:

```text
name=original.txt dev=2050 inode=397192 links=2 size=31
name=hardlink.txt dev=2050 inode=397192 links=2 size=31
```

---

## Observation 1: Same Inode

Both pathnames reported:

```text
inode = 397192
```

This means both names refer to the same inode.

They are not two independent filesystem objects containing identical data.

The relationship is:

```text
directory
├── original.txt ──────┐
│                      │
└── hardlink.txt ──────┤
                       ↓
                  inode 397192
```

---

## Observation 2: Same Device

Both pathnames reported:

```text
dev = 2050
```

The combination:

```text
(st_dev, st_ino)
```

therefore identified the same filesystem object:

```text
(2050, 397192)
```

This demonstrates why filesystem identity cannot be based on the pathname alone.

---

## Observation 3: Link Count

Both names reported:

```text
links = 2
```

The inode had two hard-link references:

```text
inode 397192
    │
    ├── original.txt
    └── hardlink.txt
```

---

## Observation 4: Shared Data

Data was appended through:

```bash
printf 'changed through hardlink\n' >> hardlink.txt
```

The contents of both names were then inspected.

`original.txt` produced:

```text
filesystem identity experiment
changed through hardlink
```

`hardlink.txt` produced the same contents.

This demonstrates that both pathnames reference the same underlying object.

---

## Observation 5: Shared Metadata

The metadata was inspected again:

```bash
stat -c '%n inode=%i links=%h size=%s' original.txt hardlink.txt
```

Observed:

```text
original.txt inode=397192 links=2 size=56
hardlink.txt inode=397192 links=2 size=56
```

The size increased from:

```text
31 bytes
```

to:

```text
56 bytes
```

Both pathnames reported the same size because the underlying filesystem object was the same.

---

## Observation 6: Removing One Hard Link

The second pathname was removed:

```bash
rm hardlink.txt
```

The remaining pathname was inspected:

```bash
stat -c '%n inode=%i links=%h size=%s' original.txt
```

Observed:

```text
original.txt inode=397192 links=1 size=56
```

The inode remained:

```text
397192
```

The size remained:

```text
56 bytes
```

The link count changed:

```text
2 → 1
```

The filesystem object therefore remained accessible through `original.txt`.

---

## Filesystem Model

Before removing the hard link:

```text
directory
├── original.txt ──────┐
│                      │
└── hardlink.txt ──────┤
                       ↓
                  inode 397192
                       │
                    file data
```

After removing the hard link:

```text
directory
│
└── original.txt ──────→ inode 397192
                              │
                           file data
```

This demonstrates that:

```text
pathname != filesystem object
```

A pathname provides a way to locate an object.

It does not necessarily uniquely identify the object.

---

## Hard Link Versus Copy

A hard link is not a file copy.

A copy produces separate filesystem objects:

```text
original.txt → inode A
copy.txt     → inode B
```

A hard link produces another reference to the same object:

```text
original.txt ──┐
               ├──→ inode A
hardlink.txt ──┘
```

Writing through either hard-link pathname modifies the same filesystem object.

---

## Removing a Hard Link

Removing a hard link removes a directory entry.

Conceptually:

```text
remove pathname
       ↓
remove directory entry
       ↓
decrease link count
       ↓
references remain?
   ├── yes → object remains
   └── no  → object may be reclaimed
```

This distinction will become important later when the project investigates open file descriptors and deleted-but-open files.

---

## Result

Experiment 002 confirms:

```text
different pathnames
        ↓
can reference
        ↓
the same filesystem object
```

The observed identity was:

```text
device = 2050
inode  = 397192
```

The experiment established that:

* hard links share an inode
* hard links share file contents
* hard links share object metadata
* `st_nlink` represents the number of hard-link references
* removing one pathname decreases the link count
* the underlying object remains while references to it remain
* pathname identity and filesystem object identity are different concepts

---

# Experiment 003: Planned

The next filesystem experiment will investigate the relationship between:

```text
symbolic link
stat()
lstat()
```

The experiment will determine:

* what object `stat()` reports when given a symbolic link
* what object `lstat()` reports
* how the inode of a symbolic link differs from its target
* how symbolic links differ from hard links
* how `fswatch` should represent symbolic-link metadata
