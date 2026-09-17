
# Linux Filesystem Model

## Purpose

This document defines the filesystem model used by `fswatch`.

The goal is to understand how Linux represents files and directories internally before building higher-level filesystem inspection features.

The model is based on direct observation through Linux commands, C system interfaces, and controlled experiments.

---

## 1. The Basic Mental Model

A pathname is not the filesystem object itself.

A useful simplified model is:

```text
pathname
    ↓
path lookup
    ↓
directory entry
    ↓
filesystem object
    ↓
inode + metadata + data
````

For a regular file, the filesystem object contains information such as:

* file type
* permissions
* ownership
* timestamps
* inode number
* filesystem/device identity
* link count
* file size
* storage allocation information

The pathname provides a way to locate the object through the directory hierarchy.

---

## 2. Filesystem Objects

Linux filesystems represent different kinds of objects.

Examples include:

* regular files
* directories
* symbolic links
* character devices
* block devices
* FIFOs
* sockets

The object type can be determined from the `st_mode` field returned by `stat()`.

Example:

```c
struct stat st;

if (stat(path, &st) == -1) {
    /* handle error */
}
```

The type can then be inspected using macros such as:

```c
S_ISREG(st.st_mode)
S_ISDIR(st.st_mode)
S_ISLNK(st.st_mode)
S_ISCHR(st.st_mode)
S_ISBLK(st.st_mode)
S_ISFIFO(st.st_mode)
S_ISSOCK(st.st_mode)
```

---

## 3. `stat()` and `struct stat`

The Linux `stat()` system interface retrieves metadata about a filesystem object.

A simplified view of the process is:

```text
pathname
    ↓
stat()
    ↓
kernel
    ↓
filesystem
    ↓
struct stat
```

The resulting `struct stat` contains information about the object.

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

The exact available fields and timestamp representation depend on the platform and API variant.

---

## 4. Filesystem Identity

A pathname alone is not sufficient to identify a filesystem object.

Two different pathnames can refer to the same object.

A useful filesystem-level identity is:

```text
(st_dev, st_ino)
```

Where:

```text
st_dev = filesystem/device identity
st_ino = inode number
```

For example, Experiment 002 produced:

```text
st_dev = 2050
st_ino = 397192
```

for both:

```text
original.txt
hardlink.txt
```

Therefore both pathnames referred to the same filesystem object.

---

## 5. Inodes

An inode represents a filesystem object and stores metadata associated with that object.

The inode is not the filename.

A simplified model is:

```text
inode
├── object metadata
├── ownership
├── permissions
├── timestamps
├── link count
└── references to file data
```

The exact internal implementation depends on the filesystem.

For filesystem inspection purposes, the important point is that an inode represents an object while directory entries provide names used to reach that object.

---

## 6. Directory Entries

Directories are themselves filesystem objects.

A directory contains mappings between names and filesystem objects.

Conceptually:

```text
directory
├── name A → object A
├── name B → object B
└── name C → object C
```

This means a filename is associated with a directory entry.

The directory entry allows path lookup to find the corresponding filesystem object.

This distinction becomes especially important when hard links are introduced.

---

## 7. Hard Links

A hard link creates another directory entry referring to an existing filesystem object.

For example:

```bash
ln original.txt hardlink.txt
```

The resulting relationship is:

```text
directory
├── original.txt ──────┐
│                      │
└── hardlink.txt ──────┤
                       ↓
                  inode 397192
```

The two pathnames are different.

The underlying filesystem object is the same.

---

## 8. Hard Links and Inode Identity

Experiment 002 showed:

```text
name=original.txt dev=2050 inode=397192 links=2 size=31
name=hardlink.txt dev=2050 inode=397192 links=2 size=31
```

The important observation is:

```text
same device
same inode
same link count
same object
```

The pathnames therefore do not identify independent filesystem objects.

Instead:

```text
original.txt ──┐
               ├──→ inode 397192
hardlink.txt ──┘
```

This demonstrates:

```text
pathname != filesystem object identity
```

---

## 9. Link Count

The `st_nlink` field reports the number of hard-link references to the filesystem object.

After creating one hard link:

```text
original.txt
hardlink.txt
```

the observed link count was:

```text
st_nlink = 2
```

Conceptually:

```text
inode 397192
    │
    ├── original.txt
    └── hardlink.txt

st_nlink = 2
```

After:

```bash
rm hardlink.txt
```

the remaining object reported:

```text
inode=397192
links=1
```

The inode number did not change.

The file data did not disappear.

Only one directory reference was removed.

---

## 10. Hard Links Share Data

Experiment 002 appended data through:

```text
hardlink.txt
```

The contents were then visible through:

```text
original.txt
```

Both names subsequently reported:

```text
inode=397192
size=56
```

This demonstrates that both names reference the same object.

The data does not belong independently to each pathname.

Instead:

```text
original.txt ──┐
               ├──→ same filesystem object
hardlink.txt ──┘
```

A write through either pathname modifies the same object.

---

## 11. Removing a Hard Link

Removing a pathname does not necessarily destroy the filesystem object.

For example:

```bash
rm hardlink.txt
```

removed the directory entry for `hardlink.txt`.

The remaining pathname:

```text
original.txt
```

still referenced inode `397192`.

The link count changed:

```text
2 → 1
```

The simplified model is:

```text
remove pathname
       ↓
remove directory entry
       ↓
decrease link count
       ↓
references remain?
       │
       ├── yes → object remains
       │
       └── no  → object may be reclaimed
```

The interaction between link counts and open file descriptors will be investigated later.

---

## 12. Hard Link Versus Copy

A hard link is different from copying a file.

A copy creates a separate filesystem object:

```text
original.txt → inode A
copy.txt     → inode B
```

A hard link creates another name for the same object:

```text
original.txt ──┐
               ├──→ inode A
hardlink.txt ──┘
```

Therefore:

```text
copy
    → separate object

hard link
    → same object
```

This distinction matters for filesystem inspection and storage accounting.

---

## 13. Symbolic Links

A symbolic link is different from a hard link.

A symbolic link is a filesystem object containing a pathname reference to another object.

Conceptually:

```text
symbolic-link
    ↓
stored pathname
    ↓
target object
```

For example:

```text
link.txt → original.txt
```

The symbolic link and its target are different filesystem objects.

This is different from a hard link:

```text
hard link
    ↓
same filesystem object
```

---

## 14. `stat()` Versus `lstat()`

`stat()` follows symbolic links.

For example:

```text
symbolic link
      ↓
target
      ↓
stat()
      ↓
target metadata
```

Therefore, when `stat()` is used on a symbolic link, the returned metadata normally describes the target rather than the link itself.

`lstat()` can be used to inspect the symbolic link itself:

```text
symbolic link
      ↓
lstat()
      ↓
link metadata
```

This distinction will become important when `fswatch` adds symbolic-link inspection.

---

## 15. File Size

`st_size` reports the logical size associated with the filesystem object.

For a regular file, this normally corresponds to the number of bytes in the file's logical contents.

For a directory, `st_size` represents the size of the directory object itself.

It does not represent the combined size of every file contained inside the directory.

For example, the project directory previously reported:

```text
Size: 4096 bytes
Type: directory
```

That does not mean the entire project consumes only 4096 bytes.

Recursive storage accounting is a separate problem.

---

## 16. Logical Size Versus Allocated Storage

The following fields describe different concepts:

```text
st_size
st_blocks
```

`st_size` describes logical file size.

`st_blocks` relates to filesystem storage allocation.

These values can differ.

This becomes especially important with:

* sparse files
* filesystem block sizes
* delayed allocation
* filesystem-specific storage behavior

The project will investigate storage allocation separately.

---

## 17. Ownership and Permissions

`struct stat` also provides ownership and permission information.

Important fields include:

```text
st_uid
st_gid
st_mode
```

`st_mode` contains both:

* file type information
* permission bits

For example:

```text
-rw-rw-r--
```

contains permission information for:

```text
owner
group
others
```

Ownership and permission behavior will be investigated in a later filesystem experiment.

---

## 18. Timestamps

`struct stat` provides filesystem timestamps.

Important timestamps include:

```text
st_atime
st_mtime
st_ctime
```

These represent different filesystem events.

They should not automatically be interpreted as:

```text
created
modified
changed
```

without understanding the specific semantics.

Birth/creation time availability also varies by filesystem and platform.

Timestamp behavior will be investigated separately.

---

## 19. Directory Objects

Directories are filesystem objects themselves.

A directory therefore has:

```text
inode
permissions
ownership
timestamps
size
link count
```

A directory's `st_size` is not the total size of everything beneath it.

The directory stores filesystem namespace information that allows path lookup to resolve names.

This explains why:

```text
directory size
```

and:

```text
recursive directory contents
```

are different measurements.

---

## 20. Pathname Resolution

A simplified path lookup model is:

```text
/path/to/file
     ↓
/
     ↓
path
     ↓
to
     ↓
file
     ↓
directory entry
     ↓
filesystem object
```

Each path component is resolved through the directory hierarchy.

The final directory entry identifies the filesystem object associated with the requested pathname.

Hard links demonstrate that multiple directory entries can resolve to the same object.

---

## 21. Current `fswatch` Model

The current implementation uses:

```c
stat(argv[2], &st);
```

and currently exposes:

```text
Size
Type
```

The next implementation stage will expose filesystem identity information:

```text
Device
Inode
Links
Size
Type
```

using:

```c
st_dev
st_ino
st_nlink
st_size
st_mode
```

This keeps the tool close to the underlying Linux filesystem interface instead of hiding the model behind unnecessary abstractions.

---

## 22. Current Mental Model

The current filesystem model is:

```text
                         pathname
                             │
                             ↓
                      path resolution
                             │
                             ↓
                     directory entry
                             │
                  ┌──────────┴──────────┐
                  │                     │
              one name              another name
                  │                     │
                  └──────────┬──────────┘
                             ↓
                     filesystem object
                             │
                           inode
                             │
             ┌───────────────┼───────────────┐
             │               │               │
          metadata          links           data
             │               │               │
        permissions      st_nlink         contents
        ownership
        timestamps
        st_dev
        st_ino
        st_size
```

The important distinction is:

```text
pathname
    ≠
filesystem object
```

Multiple pathnames can reference one filesystem object.

---

## 23. Engineering Principle

`fswatch` should model the filesystem as Linux actually exposes it.

The implementation should therefore:

1. Observe real filesystem behavior.
2. Identify the kernel interface responsible for that behavior.
3. Build the smallest useful implementation.
4. Test the implementation against controlled filesystem states.
5. Document observations and limitations.
6. Add abstractions only when the implementation develops a real need for them.

The purpose of the project is understanding the system, not reproducing the interface of an existing filesystem utility.

````

After replacing the file, run:

```bash
git diff --check
````
