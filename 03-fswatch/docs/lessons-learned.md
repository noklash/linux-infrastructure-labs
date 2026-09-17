
# Lessons Learned

This document records the important engineering lessons discovered while building and experimenting with `fswatch`.

The goal is to capture the mental models behind the implementation rather than only recording commands or code.

---

## 1. A Pathname Is Not the Filesystem Object

A pathname is a way to locate an object through the filesystem namespace.

The simplified model is:

```text
pathname
    ↓
path lookup
    ↓
directory entry
    ↓
filesystem object
````

This distinction became important during the hard-link experiment.

Two different pathnames can refer to the same underlying object.

Therefore:

```text
pathname != filesystem object identity
```

---

## 2. Filesystem Objects Have Identity

The `stat()` interface exposes:

```c
st_dev
st_ino
```

These values provide a useful filesystem-level identity:

```text
(st_dev, st_ino)
```

During Experiment 002:

```text
device = 2050
inode  = 397192
```

Both:

```text
original.txt
hardlink.txt
```

reported the same values.

This showed that both pathnames referred to the same filesystem object.

---

## 3. Hard Links Are Not Copies

Creating:

```bash
ln original.txt hardlink.txt
```

does not create a second copy of the file's data.

Instead, it creates another directory entry referring to the same filesystem object.

The model is:

```text
original.txt ──┐
               ├──→ inode 397192
hardlink.txt ──┘
```

A copy would produce separate objects:

```text
original.txt → inode A
copy.txt     → inode B
```

This distinction is important for understanding filesystem identity and storage.

---

## 4. Multiple Names Can Reference One Object

Experiment 002 demonstrated that two different pathnames can reference the same object.

Initially:

```text
original.txt
hardlink.txt
```

both referenced:

```text
inode = 397192
```

Both names therefore exposed the same:

* file contents
* file size
* inode
* device identity
* link count

This is a fundamental filesystem concept.

---

## 5. `st_nlink` Represents Hard-Link References

The `st_nlink` field reports the number of hard-link references to the object.

After creating the hard link:

```text
st_nlink = 2
```

After removing one pathname:

```text
st_nlink = 1
```

The inode itself remained:

```text
397192
```

This demonstrates that the inode can remain after one pathname is removed.

---

## 6. Removing a Name Does Not Necessarily Destroy the Object

The hard-link experiment demonstrated an important distinction.

Running:

```bash
rm hardlink.txt
```

removed the pathname:

```text
hardlink.txt
```

It did not destroy the underlying object because:

```text
original.txt
```

still referenced it.

The simplified model is:

```text
remove pathname
       ↓
remove directory entry
       ↓
decrease link count
       ↓
references remain
       ↓
object remains
```

The relationship between this behavior and open file descriptors will be investigated later.

---

## 7. File Data Belongs to the Object

Data was appended through:

```text
hardlink.txt
```

The new data was immediately visible through:

```text
original.txt
```

This happened because both names referred to the same object.

The important model is:

```text
pathname
    ↓
directory entry
    ↓
same filesystem object
    ↓
same data
```

The pathname does not own an independent copy of the data.

---

## 8. Directories Are Filesystem Objects

A directory is also a filesystem object.

It has metadata including:

```text
inode
permissions
ownership
timestamps
size
link count
```

Experiment 001 showed that the project directory had:

```text
size = 4096 bytes
```

This does not mean that all files beneath the directory occupy only 4096 bytes.

The directory's `st_size` describes the directory object itself.

Therefore:

```text
directory object size
```

and:

```text
recursive contents size
```

are different measurements.

---

## 9. `st_size` Is Not Physical Storage Usage

The `st_size` field represents logical size.

The filesystem also exposes:

```text
st_blocks
```

which relates to allocated storage.

Therefore:

```text
logical size
    ≠
allocated storage
```

This distinction will become important when investigating:

* sparse files
* filesystem blocks
* storage accounting
* `du`
* filesystem-specific allocation behavior

---

## 10. `struct stat` Exposes More Than One Property

The initial `fswatch` implementation only used:

```c
st_size
st_mode
```

Experiment 001 showed that `struct stat` provides substantially more information.

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

This makes `struct stat` a fundamental interface for filesystem inspection.

---

## 11. File Type Is Stored in Metadata

`st_mode` contains file type information.

Linux provides macros such as:

```c
S_ISREG()
S_ISDIR()
S_ISLNK()
S_ISCHR()
S_ISBLK()
S_ISFIFO()
S_ISSOCK()
```

This means the type of a filesystem object can be determined from metadata returned by the filesystem.

The current `fswatch` implementation uses these macros to classify objects.

---

## 12. `stat()` Follows Symbolic Links

A symbolic link introduces another layer into filesystem path resolution.

The simplified model is:

```text
symbolic link
      ↓
target pathname
      ↓
target object
```

`stat()` follows the symbolic link and normally reports metadata for the target.

`lstat()` can inspect the symbolic link itself.

Therefore:

```text
stat()
    → target metadata

lstat()
    → link metadata
```

This behavior has not yet been explored experimentally in this project.

A dedicated symbolic-link experiment is planned.

---

## 13. Filesystem Identity Is Different From Path Identity

A pathname can change while the underlying object remains the same.

Hard links make this obvious:

```text
original.txt ──┐
               ├──→ same object
hardlink.txt ──┘
```

After:

```bash
rm hardlink.txt
```

the object remains accessible through:

```text
original.txt
```

The filesystem object therefore has an identity independent of the specific pathname used to reach it.

This is an important mental model for filesystem tools.

---

## 14. The Kernel Interface Should Drive the Tool

`fswatch` should be built around the actual Linux interfaces that expose filesystem behavior.

The current implementation uses:

```c
stat()
```

and:

```c
struct stat
```

Future filesystem functionality will use interfaces such as:

```c
lstat()
opendir()
readdir()
closedir()
open()
read()
write()
close()
```

The project should first understand what these interfaces actually provide before creating abstractions around them.

---

## 15. Experiments and Tests Have Different Jobs

Experiments answer questions about Linux behavior.

For example:

```text
Can two pathnames refer to the same inode?
```

Tests answer questions about `fswatch` behavior.

For example:

```text
Does fswatch correctly report an inode?
```

Therefore:

```text
experiments
    → investigate system behavior

tests
    → verify implementation behavior
```

Keeping these concerns separate makes the project easier to reason about.

---

## 16. Implementation Should Follow Observation

The project follows this sequence:

```text
observe Linux behavior
        ↓
understand the filesystem model
        ↓
document the result
        ↓
implement the feature
        ↓
test the implementation
        ↓
compare implementation with system behavior
```

This prevents the tool from becoming a collection of assumptions.

---

## 17. Current Filesystem Mental Model

The current model is:

```text
                    pathname
                        │
                        ↓
                 path resolution
                        │
                        ↓
                directory entry
                        │
                        ↓
               filesystem object
                        │
                      inode
                        │
          ┌─────────────┼─────────────┐
          │             │             │
       metadata       links          data
          │             │             │
       st_dev       st_nlink       contents
       st_ino
       st_mode
       st_uid
       st_gid
       st_size
       timestamps
```

Multiple directory entries can reference the same filesystem object.

That is the key concept established by the hard-link experiment.

---

## 18. Current Engineering Direction

The current `fswatch` implementation reports:

```text
Size
Type
```

The next implementation stage will add:

```text
Device
Inode
Links
```

using:

```c
st_dev
st_ino
st_nlink
```

The implementation will then be tested against controlled filesystem states.

Future experiments will investigate:

* symbolic links
* `stat()` versus `lstat()`
* ownership and permissions
* timestamps
* directory traversal
* recursive traversal
* filesystem boundaries
* mount points
* storage allocation
* sparse files
* file descriptors
* deleted-but-open files

---

## 19. Core Lesson

The most important lesson so far is that the Linux filesystem is not simply:

```text
filename → file
```

A more accurate model is:

```text
pathname
    ↓
directory entry
    ↓
filesystem object
    ↓
inode + metadata + data
```

Multiple pathnames can reference the same object.

Understanding this relationship is necessary before building reliable filesystem inspection tools.
