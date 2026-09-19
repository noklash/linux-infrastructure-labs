
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

The initial `fswatch` implementation used:

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

Experiment 003 demonstrated that symbolic links introduce another layer into filesystem path resolution.

The setup was:

```text
target.txt
link.txt -> target.txt
```

When:

```c
stat("link.txt", &st);
```

was called, `stat()` followed the symbolic link and reported metadata for the target.

The target was reported as:

```text
type: regular file
inode: 396988
size: 25 bytes
```

The symbolic link itself had a different inode:

```text
inode: 397056
type: symbolic link
size: 10 bytes
```

The important model is:

```text
link.txt
    ↓
follow symbolic link
    ↓
target.txt
    ↓
target object
```

Therefore:

```text
stat()
    → target metadata
```

---

## 13. `lstat()` Inspects the Symbolic Link Itself

Experiment 003 showed that `lstat()` behaves differently from `stat()`.

For:

```text
link.txt -> target.txt
```

calling:

```c
lstat("link.txt", &st);
```

returned metadata for the symbolic link itself.

The experiment reported:

```text
inode: 397056
type: symbolic link
size: 10 bytes
```

The model is:

```text
lstat()
    ↓
inspect pathname's object
    ↓
symbolic-link object
```

Therefore:

```text
stat()
    → target object

lstat()
    → symbolic-link object
```

This distinction is important when a program needs to inspect the object represented by the supplied pathname rather than automatically following symbolic links.

---

## 14. Symbolic Links Have Their Own Filesystem Identity

The symbolic-link experiment showed that the link itself has its own:

```text
device
inode
link count
size
type
```

For example:

```text
link.txt
    inode = 397056
    type  = symbolic link
    size  = 10 bytes
```

The target had a different identity:

```text
target.txt
    inode = 396988
    type  = regular file
    size  = 25 bytes
```

Therefore:

```text
symbolic link identity
    ≠
target identity
```

A symbolic link is a filesystem object that contains a reference to another pathname.

---

## 15. `readlink()` Reads the Stored Symbolic-Link Target

A symbolic link stores a target pathname.

`readlink()` retrieves that stored pathname without following it.

For:

```text
link.txt -> target.txt
```

the experiment returned:

```text
target.txt
```

with:

```text
length = 10 bytes
```

The model is:

```text
symbolic-link object
        |
        +── stores "target.txt"
```

`readlink()` exposes the stored target.

It does not return metadata for the target.

---

## 16. A Broken Symbolic Link Still Exists

The experiment then removed:

```text
target.txt
```

The symbolic link remained:

```text
link.txt -> target.txt
```

`lstat()` still succeeded:

```text
type: symbolic link
inode: 397056
size: 10 bytes
```

`readlink()` still returned:

```text
target.txt
```

But:

```c
stat("link.txt", &st);
```

failed with:

```text
No such file or directory
```

because the target could no longer be resolved.

This demonstrates an important distinction:

```text
broken symbolic link
    ≠
nonexistent pathname
```

The link object still exists.

Only the object it points to is missing.

---

## 17. `stat()`, `lstat()`, and `readlink()` Answer Different Questions

The symbolic-link experiment made the purpose of the three interfaces clearer.

```text
stat()
    → What object do I reach by following this pathname?

lstat()
    → What object is represented by this pathname itself?

readlink()
    → What target pathname is stored in this symbolic link?
```

For:

```text
link.txt -> target.txt
```

the relationship can be visualized as:

```text
                    link.txt
                       |
                       v
                symbolic-link
                  object
                       |
                       | stores
                       v
                  target.txt
                       |
                       v
                 target object
```

`lstat()` stops at the symbolic-link object.

`stat()` follows the link.

`readlink()` reads the stored target pathname.

---

## 18. Filesystem Identity Is Different From Path Identity

A pathname can change while the underlying object remains the same.

Hard links make this obvious:

```text
original.txt ──┐
               ├──→ same object
hardlink.txt ──┘
```

Symbolic links show another side of the same idea:

```text
link.txt
    ↓
target.txt
```

The pathname `link.txt` identifies a symbolic-link object, while the pathname stored inside that object refers to another object.

The filesystem therefore contains relationships between objects and names rather than simply a flat collection of filenames.

---

## 19. The Kernel Interface Should Drive the Tool

`fswatch` should be built around the actual Linux interfaces that expose filesystem behavior.

The current implementation uses:

```c
stat()
```

and:

```c
struct stat
```

The experiments have now also established the behavior of:

```c
lstat()
readlink()
```

Future filesystem functionality will use interfaces such as:

```c
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

## 20. Experiments and Tests Have Different Jobs

Experiments answer questions about Linux behavior.

For example:

```text
Can two pathnames refer to the same inode?
```

or:

```text
Does stat() follow a symbolic link?
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

## 21. Implementation Should Follow Observation

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

The symbolic-link work followed this process directly:

```text
experiment
    ↓
stat() / lstat() / readlink()
    ↓
understand the difference
    ↓
decide fswatch behavior
    ↓
implement
    ↓
test
```

---

## 22. Current Filesystem Mental Model

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

For hard links:

```text
pathname A ──┐
             ├──→ same filesystem object
pathname B ──┘
```

For symbolic links:

```text
pathname
    ↓
symbolic-link object
    ↓
stored target pathname
    ↓
target object
```

Multiple directory entries can reference the same filesystem object.

A symbolic link is itself an object that stores a pathname pointing somewhere else.

---

## 23. Current `fswatch` Direction

The current `fswatch info <path>` implementation uses `stat()`.

That means a symbolic link currently behaves differently from a regular filesystem object:

```text
fswatch info link.txt
        ↓
      stat()
        ↓
  target metadata
```

The symbolic-link experiment showed that this is not the only possible behavior.

The next implementation decision is to make `fswatch info` inspect the supplied pathname itself.

That means:

```text
fswatch info link.txt
        ↓
      lstat()
        ↓
symbolic-link metadata
        ↓
readlink()
        ↓
target pathname
```

The intended behavior is:

* use `lstat()` for primary metadata
* report `Type: symbolic link`
* use `readlink()` to show the stored target
* allow broken symbolic links to be inspected
* do not yet report the target object's metadata

This keeps the implementation small while making the distinction between a symbolic link and its target visible.

---

## 24. Core Lesson

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

A symbolic link is itself a filesystem object that contains a target pathname.

`stat()` follows that relationship.

`lstat()` inspects the link itself.

`readlink()` reads the stored target.

Understanding these relationships is necessary before building reliable filesystem inspection tools.

---

## 25. Next Topics

The filesystem topics still to investigate include:

* ownership
* permissions
* timestamps
* directory traversal
* directory streams
* file descriptors
* `open()`
* `read()`
* `write()`
* `close()`
* deleted-but-open files
* filesystem boundaries
* mount points
* pseudo-filesystems
* allocated storage
* sparse files
* filesystem capacity

Each topic should be investigated through a controlled experiment before being added to `fswatch`.


