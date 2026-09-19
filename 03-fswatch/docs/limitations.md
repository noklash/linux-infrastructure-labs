
# Limitations

This document records the current boundaries of `fswatch`.

The purpose is to make the current scope explicit rather than imply capabilities that the program does not yet have.

---

## 1. Current Scope

The current implementation provides a small filesystem inspection command:

```text
./fswatch info <path>
````

It reports basic metadata about the filesystem object represented by the supplied pathname.

Current metadata includes:

* device identifier
* inode number
* link count
* logical size
* object type

For symbolic links, the program also reports the stored target pathname.

The implementation intentionally remains small. It is designed to expose filesystem behavior clearly rather than reproduce the feature set of standard utilities such as `stat`, `ls`, `find`, or `file`.

---

## 2. No Recursive Traversal

`fswatch` currently inspects only the pathname supplied by the user.

For example:

```text
./fswatch info directory/
```

reports metadata about the directory object itself.

It does not:

* enumerate directory contents
* recursively descend into subdirectories
* calculate recursive directory sizes
* produce a filesystem tree
* search for matching files

Recursive traversal will be considered only after the underlying directory interfaces and traversal behavior have been studied directly.

---

## 3. Directory Contents Are Not Reported

A directory is itself a filesystem object.

Its `st_size` value therefore describes metadata associated with the directory object rather than the total size of every file contained within it.

For example:

```text
./fswatch info .
```

does not mean:

```text
total size of everything below .
```

It means:

```text
metadata about the directory object represented by .
```

The current implementation intentionally does not attempt to reinterpret directory size as recursive content size.

---

## 4. No Permission or Ownership Display

The current output does not report:

* file permissions
* user ID
* group ID
* access control lists
* extended attributes
* security labels

These values are available through other filesystem interfaces and metadata fields, but they are outside the current implementation scope.

The project may study these concepts later as filesystem fundamentals progress.

---

## 5. No Timestamp Display

The current implementation does not display:

* access time
* modification time
* metadata-change time
* creation/birth time where supported

The `struct stat` interface exposes several timestamps, but the project currently focuses on understanding filesystem identity, object type, size, and link relationships.

---

## 6. Symbolic Links

`fswatch info <path>` uses `lstat()` for primary filesystem-object inspection.

This means that when the supplied pathname refers to a symbolic link, `fswatch` reports the symbolic-link object itself rather than automatically following the link.

For symbolic links, the implementation then uses `readlink()` to retrieve the stored target pathname.

The current behavior is:

```text
regular file
    |
    +--> lstat()
          |
          +--> metadata

directory
    |
    +--> lstat()
          |
          +--> metadata

symbolic link
    |
    +--> lstat()
    |      |
    |      +--> symbolic-link metadata
    |
    +--> readlink()
           |
           +--> stored target pathname
```

A broken symbolic link can still be inspected because `lstat()` operates on the symbolic-link object itself and `readlink()` reads the stored target pathname without resolving it.

The implementation deliberately does not resolve the target and display its metadata.

That would introduce another layer of behavior that is not currently required by the project.

The symbolic-link experiment demonstrates the difference between the relevant interfaces:

```text
stat()
    -> follows the final symbolic link

lstat()
    -> reports the symbolic link itself

readlink()
    -> reads the stored target pathname
```

This distinction is part of the filesystem model being developed by the project.

---

## 7. Broken Symbolic Links

A broken symbolic link is a symbolic link whose stored target cannot currently be resolved.

For example:

```text
link.txt -> target.txt
```

where:

```text
target.txt
```

no longer exists.

The pathname `link.txt` still identifies a filesystem object.

Therefore:

```text
lstat("link.txt")
```

can succeed.

Likewise:

```text
readlink("link.txt")
```

can return:

```text
target.txt
```

while:

```text
stat("link.txt")
```

can fail because `stat()` attempts to follow the link to its target.

This behavior is intentionally supported by `fswatch`.

A broken symbolic link should still produce output such as:

```text
Device: ...
Inode: ...
Links: 1
Size: ...
Type: symbolic link
Target: target.txt
```

The current implementation does not attempt to determine whether the target exists after retrieving the stored pathname.

---

## 8. No Target Metadata for Symbolic Links

When `fswatch` encounters a symbolic link, it reports metadata belonging to the link itself.

For example:

```text
Type: symbolic link
```

and:

```text
Target: target.txt
```

It does not additionally report:

```text
target inode
target size
target type
target permissions
```

This is intentional.

There are two separate filesystem objects involved:

```text
symbolic-link object
        |
        | stores pathname
        v
    target object
```

The current command focuses on the first object.

The symbolic-link experiment exists separately to demonstrate what changes when the second object is resolved.

---

## 9. No Hard-Link Creation or Management

`fswatch` does not create, remove, or modify hard links.

The project has studied hard links experimentally to establish that multiple pathnames can refer to the same filesystem object.

The important identity fields are:

```text
st_dev
st_ino
```

The current program only reports these fields.

It does not attempt to discover every pathname that references the same inode.

---

## 10. No Symbolic-Link Creation or Modification

`fswatch` does not create or modify symbolic links.

It only inspects them.

The following operations are currently outside the program:

```text
symlink()
unlink()
rename()
link()
```

These operations may become useful in later filesystem experiments, but they are not required for the current inspection tool.

---

## 11. No File Content Inspection

`fswatch` does not read the contents of regular files.

For example:

```text
./fswatch info README.md
```

does not open `README.md` and inspect its contents.

The program obtains metadata through filesystem interfaces.

This keeps the distinction clear between:

```text
filesystem metadata
```

and:

```text
file data
```

---

## 12. No File Modification

The current command is read-only with respect to the filesystem objects it inspects.

It does not:

* modify file contents
* truncate files
* create files
* delete files
* rename files
* change permissions
* change ownership
* create links

This makes the tool suitable for controlled filesystem observation.

---

## 13. No Filesystem Mount Information

The current implementation does not inspect:

* mount points
* mount options
* filesystem type
* filesystem capacity
* free space
* block-device relationships
* `/proc/mounts`
* `/proc/self/mountinfo`

The `st_dev` field provides a device identifier, but that does not by itself provide a complete mount or filesystem description.

These topics belong to a later stage of filesystem study.

---

## 14. No `/proc` or `/sys` Integration

The program currently operates through normal filesystem interfaces.

It does not directly parse:

```text
/proc
/sys
```

or other kernel-provided pseudo-filesystems.

Those interfaces will be studied separately when the project reaches the relevant Linux subsystems.

---

## 15. No Error Classification

The current implementation reports syscall failures using `perror()`.

For example:

```text
lstat: No such file or directory
```

The program does not yet provide structured error categories such as:

```text
NOT_FOUND
PERMISSION_DENIED
INVALID_ARGUMENT
LOOP_DETECTED
IO_ERROR
```

The underlying `errno` information exists, but the current command exposes the operating-system error message directly.

---

## 16. No Path Normalization

The program does not implement its own pathname normalization.

It relies on the Linux filesystem interfaces to interpret the supplied pathname.

Therefore paths containing components such as:

```text
.
..
```

are handled by the operating system rather than by a custom path-processing layer inside `fswatch`.

---

## 17. No Pathname Resolution Engine

`fswatch` does not implement its own pathname resolver.

The kernel performs pathname lookup when `lstat()` or `readlink()` is called.

The project therefore treats pathname resolution as a kernel/filesystem behavior to observe rather than something to duplicate in userspace.

This distinction is important because pathname lookup involves multiple layers:

```text
pathname
    |
    v
directory lookup
    |
    v
directory entry
    |
    v
filesystem object
    |
    v
metadata
```

Symbolic links introduce another resolution step when an interface such as `stat()` follows the final link.

---

## 18. No Race-Free Snapshot Guarantee

Filesystem metadata can change between system calls.

For example, another process could:

1. create a file
2. remove it
3. replace a symbolic link
4. change file metadata

between two operations performed by `fswatch`.

The program does not attempt to provide a transactional or race-free filesystem snapshot.

This is expected for a small inspection utility.

The current goal is understanding the semantics of individual filesystem operations.

---

## 19. Symbolic-Link Loops Are Not Resolved by the Program

The program does not manually follow symbolic links.

This means it does not implement its own symbolic-link traversal algorithm and therefore does not need to detect arbitrary symbolic-link cycles during normal inspection.

For example:

```text
a -> b
b -> a
```

can exist as filesystem objects.

`fswatch` uses `lstat()` to inspect the supplied pathname and therefore reports the link itself rather than recursively resolving the target.

---

## 20. Limited Output Format

The output is intended for human inspection and learning.

It is not currently designed as a stable machine-readable interface.

For example:

```text
Device: ...
Inode: ...
Links: ...
Size: ...
Type: ...
```

There is currently no:

```text
--json
--csv
--machine-readable
```

output mode.

A structured output format would require an explicit interface decision and automated compatibility tests.

---

## 21. Linux/POSIX Scope

The project currently targets Linux and uses standard POSIX/Linux filesystem interfaces.

The current implementation should therefore not be treated as a portable filesystem abstraction for every operating system.

The code intentionally favors direct interaction with the underlying operating-system interfaces because the educational goal is to understand those interfaces rather than hide them behind a portability layer.

---

## 22. No Performance Benchmarking Yet

The project currently focuses on correctness and filesystem semantics.

It does not yet measure:

* syscall latency
* throughput
* metadata lookup cost
* cache effects
* filesystem differences
* performance under contention
* performance across large directory trees

Performance experiments will be useful later, but they are intentionally separated from the current filesystem model work.

---

## 23. Experiments Are Separate from Automated Tests

The project deliberately separates:

```text
experiments/
```

from:

```text
tests/
```

Experiments are used to investigate Linux behavior.

Tests are used to verify that `fswatch` continues to behave according to its intended interface.

For example:

```text
experiments/003-symbolic-links/
```

investigates:

```text
stat()
lstat()
readlink()
```

while:

```text
tests/test_info.sh
```

verifies the behavior implemented by `fswatch`.

This distinction prevents exploratory observations from being confused with application-level acceptance tests.

---

## 24. Current Known Error Cases

The current implementation explicitly exercises several filesystem error conditions.

### Missing pathname

```text
lstat()
    |
    +--> failure
```

`fswatch` returns a failure status.

### Broken symbolic link

```text
lstat()
    |
    +--> succeeds

readlink()
    |
    +--> succeeds
```

The broken link itself remains inspectable.

### Non-symbolic-link object passed to `readlink()`

The implementation only calls `readlink()` after `lstat()` identifies the object as a symbolic link.

The standalone experiment demonstrates that `readlink()` is not a general metadata interface and should not be used to inspect arbitrary filesystem objects.

---

## 25. Current Architecture Boundary

The current implementation intentionally keeps the architecture simple:

```text
command line
     |
     v
argument validation
     |
     v
lstat()
     |
     v
struct stat
     |
     +---- metadata output
     |
     +---- symbolic link?
                |
                v
            readlink()
                |
                v
          target output
```

There is no separate:

```text
parser
filesystem abstraction layer
object model
plugin system
configuration system
```

This is deliberate.

The project is currently using the smallest architecture that makes the filesystem behavior visible.

---

## 26. Future Scope

Potential future filesystem capabilities include:

* directory entry enumeration
* `opendir()` / `readdir()` experiments
* file descriptor operations
* `open()` / `read()` / `write()` behavior
* permissions and ownership
* timestamps
* extended attributes
* filesystem capacity
* mount information
* path traversal
* recursive traversal
* race conditions
* filesystem caching
* inotify
* file-event monitoring
* performance measurements
* additional filesystem types

These are future investigation areas, not current guarantees.

---

## 27. Current Project Boundary

At the current stage, `fswatch` should be understood as:

```text
A small Linux filesystem metadata inspection tool
built to understand filesystem behavior through direct
system-call experiments and controlled implementation.
```

It is not intended to replace:

```text
stat
ls
find
file
du
readlink
```

The value of the project is the engineering understanding produced while building it:

```text
observation
    |
    v
experiment
    |
    v
implementation
    |
    v
test
    |
    v
documentation
```

That workflow is the primary purpose of the project.

