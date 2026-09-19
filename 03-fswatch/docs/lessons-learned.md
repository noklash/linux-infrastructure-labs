
# Lessons Learned

This document records the technical lessons discovered while building `fswatch`.

The goal is to preserve the reasoning behind the implementation rather than only documenting the final code.

---

## 1. A Pathname Is Not the Same as a Filesystem Object

One of the first important distinctions in the project is between a pathname and the filesystem object reached through that pathname.

For example:

```text
/home/gmma/file.txt
````

is a pathname.

It identifies a name in the filesystem namespace.

The underlying filesystem object has its own identity and metadata.

A simplified model is:

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

This distinction becomes especially important with hard links and symbolic links.

---

## 2. `struct stat` Provides a Metadata Boundary

The initial implementation of `fswatch` used `stat()` to obtain a `struct stat`.

The structure provides information such as:

```text
st_dev
st_ino
st_nlink
st_size
st_mode
```

These fields expose several important filesystem concepts:

* filesystem/device identity
* object identity
* number of hard links
* logical object size
* object type and permission bits

The project therefore uses `struct stat` as an important boundary between the userspace program and filesystem metadata.

---

## 3. Logical Size Is Not Recursive Size

The value reported by:

```c
st.st_size
```

does not mean:

```text
total storage consumed by everything related to this object
```

For a regular file, it normally represents the file's logical byte length.

For a directory, it represents the size of the directory object itself.

Therefore:

```text
./fswatch info .
```

does not calculate the total size of the contents below `.`.

This distinction prevents `st_size` from being incorrectly interpreted as recursive disk usage.

---

## 4. Device and Inode Form Useful Object Identity

The project observed that:

```text
st_dev
st_ino
```

can be used together to distinguish filesystem objects.

The hard-link experiment demonstrated this directly.

Two different pathnames produced the same values:

```text
original.txt
hardlink.txt
```

Both referred to the same object.

Conceptually:

```text
original.txt ----+
                 |
                 v
             inode/object
                 ^
                 |
hardlink.txt ----+
```

The pathnames are different.

The underlying object is the same.

---

## 5. Hard Links Are Multiple Names for One Object

The hard-link experiment created:

```text
original.txt
hardlink.txt
```

and showed that both pathnames had the same:

```text
st_dev
st_ino
```

The link count increased when the second hard link was created.

Writing through either pathname changed the same underlying file data.

This demonstrated that a hard link is not a copy of a file.

It is another directory entry referencing the same filesystem object.

---

## 6. Unlinking a Name Is Different from Destroying an Object

When one hard link was removed:

```text
rm hardlink.txt
```

the original pathname remained valid.

The link count changed from:

```text
2
```

to:

```text
1
```

The underlying object remained because another directory entry still referenced it.

This established an important distinction:

```text
pathname removal
        !=
immediate object destruction
```

The lifecycle of a filesystem object depends on its references and other filesystem semantics.

---

## 7. `stat()` Follows Symbolic Links

The symbolic-link experiment demonstrated that `stat()` behaves differently from what we observed with a hard link.

Suppose:

```text
link.txt -> target.txt
```

When:

```c
stat("link.txt", &st);
```

is called, the final symbolic link is followed.

The metadata returned describes the target object.

Conceptually:

```text
link.txt
    |
    | stat()
    v
target.txt
    |
    v
target metadata
```

Therefore, `stat()` answers a question related to the object reached after following the pathname.

---

## 8. `lstat()` Inspects the Symbolic Link Itself

The experiment showed that:

```c
lstat("link.txt", &st);
```

behaves differently.

Instead of following the final symbolic link, it reports the symbolic-link object itself.

Conceptually:

```text
link.txt
    |
    | lstat()
    v
symbolic-link object
```

This distinction is important when a program needs to inspect the object represented by the supplied pathname rather than automatically following symbolic links.

The current `fswatch` implementation therefore uses `lstat()` for its primary filesystem-object inspection.

---

## 9. Symbolic Links Have Their Own Filesystem Identity

The symbolic-link experiment showed that the symbolic link and its target have different inode identities.

For example:

```text
target.txt
    inode = 396988

link.txt
    inode = 397056
```

The symbolic link is therefore not merely an alternate pathname for the target in the same way a hard link is.

It is a separate filesystem object.

Conceptually:

```text
symbolic-link object
        |
        | stores pathname
        v
    target pathname
        |
        v
target object
```

The two objects have independent identities.

---

## 10. A Symbolic Link Stores a Pathname

A symbolic link contains a target pathname.

For example:

```text
link.txt -> target.txt
```

The string:

```text
target.txt
```

is stored as the symbolic link's target.

It is not a copy of the target file's contents.

This is why the symbolic link can remain after the target has been removed.

---

## 11. `readlink()` Reads the Stored Target

The experiment showed that:

```c
readlink("link.txt", buffer, size);
```

retrieves the pathname stored inside the symbolic link.

It does not follow the link to obtain metadata about the target.

Conceptually:

```text
link.txt
    |
    | readlink()
    v
"target.txt"
```

This makes `readlink()` fundamentally different from `stat()` and `lstat()`.

---

## 12. Symbolic-Link Size Represents the Stored Target Pathname

The symbolic-link experiment produced:

```text
Type: symbolic link
Size: 10 bytes
```

for a link whose stored target was:

```text
target.txt
```

which contains 10 characters.

The important observation is that the symbolic link's `st_size` can represent the length of its stored target pathname rather than the size of the target object's contents.

Therefore:

```text
symbolic-link size
        !=
target file size
```

They describe different objects.

---

## 13. A Broken Symbolic Link Still Exists

After:

```text
target.txt
```

was removed, the symbolic link remained:

```text
link.txt -> target.txt
```

The target could no longer be resolved.

However:

```c
lstat("link.txt", &st);
```

still succeeded.

Likewise:

```c
readlink("link.txt", buffer, size);
```

still returned:

```text
target.txt
```

This demonstrates that the symbolic link and its target are separate filesystem objects.

The target can disappear while the symbolic-link object remains.

---

## 14. `stat()`, `lstat()`, and `readlink()` Answer Different Questions

The symbolic-link experiment made the purpose of the three interfaces clearer.

```text
stat()
    |
    +--> follows the final symbolic link
    |
    +--> reports metadata for the resolved object
```

```text
lstat()
    |
    +--> does not follow the final symbolic link
    |
    +--> reports metadata for the symbolic-link object
```

```text
readlink()
    |
    +--> does not resolve the link
    |
    +--> returns the stored target pathname
```

A useful mental model is:

```text
                pathname
                    |
        +-----------+-----------+
        |                       |
      stat()                  lstat()
        |                       |
        v                       v
   follow link             stop at link
        |                       |
        v                       v
 target object          symbolic-link object
                                |
                                |
                            readlink()
                                |
                                v
                         stored pathname
```

---

## 15. A Broken Link Separates Existence from Resolution

A broken symbolic link demonstrates that two different questions can be asked about a pathname:

```text
Does the symbolic-link object exist?
```

and:

```text
Can the symbolic link be resolved to its target?
```

For a broken link:

```text
lstat()
    -> succeeds

readlink()
    -> succeeds

stat()
    -> fails because target resolution fails
```

Therefore, failure to resolve a target does not necessarily mean that the pathname's final filesystem object does not exist.

---

## 16. The Filesystem Namespace Contains Relationships

The hard-link and symbolic-link experiments show two different relationships.

### Hard link

```text
name A ----+
           |
           v
       same object
           ^
           |
name B ----+
```

Multiple names reference the same object.

### Symbolic link

```text
name A
  |
  v
symbolic-link object
  |
  | stores pathname
  v
target object
```

The symbolic link is itself an object containing a reference to another pathname.

This distinction is fundamental to understanding filesystem namespace behavior.

---

## 17. System Calls Expose Different Layers of Filesystem Semantics

The project initially treated filesystem metadata as a simple lookup.

The experiments showed that different system calls expose different points in the pathname/object relationship.

For example:

```text
stat()
```

asks the kernel to resolve the pathname and report the resulting object's metadata.

```text
lstat()
```

stops at the symbolic-link object when the final component is a symbolic link.

```text
readlink()
```

retrieves the pathname stored inside that symbolic-link object.

The system call chosen therefore changes what the program observes.

---

## 18. `fswatch` Should Not Reimplement Kernel Path Resolution

The project does not attempt to write its own pathname resolver.

Instead, it uses Linux filesystem interfaces directly.

The simplified model is:

```text
fswatch
    |
    | lstat()
    v
kernel
    |
    v
VFS
    |
    v
filesystem
    |
    v
filesystem object
```

This keeps the implementation small and makes the operating-system behavior visible.

---

## 19. Experiments and Tests Have Different Purposes

An experiment answers:

```text
What does Linux actually do?
```

A test answers:

```text
Does our program still behave according to its intended interface?
```

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

checks that `fswatch` behaves as designed.

The distinction matters because experimental output can change as understanding improves, while application tests should protect deliberate behavior.

---

## 20. Tests Should Control Their Own Inputs

The original tests depended on repository state.

For example, the regular-file test depended on the size of:

```text
README.md
```

That made the test fragile.

The improved test suite creates its own temporary filesystem objects:

```text
temporary directory
    |
    +-- regular file
    |
    +-- directory
    |
    +-- symbolic link
    |
    +-- broken symbolic link
```

This makes the tests:

* reproducible
* isolated
* independent of documentation changes
* easier to understand
* safer to run repeatedly

The tests currently validate:

```text
18 passed
0 failed
```

---

## 21. Implementation Should Follow Observation

The project follows this sequence:

```text
question
    |
    v
experiment
    |
    v
observation
    |
    v
mental model
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

The symbolic-link work followed this process directly.

First, the behavior of:

```text
stat()
lstat()
readlink()
```

was observed independently.

Then the implementation decision was made.

The resulting `fswatch` behavior uses:

```text
lstat()
    |
    +--> metadata

if symbolic link:
    |
    +--> readlink()
            |
            +--> target pathname
```

The automated tests then encoded the intended behavior.

---

## 22. Small Implementations Make Behavior Easier to See

The current implementation remains deliberately small.

It does not introduce:

* a large filesystem abstraction layer
* object-oriented wrappers
* unnecessary helper libraries
* configuration systems
* plugins
* recursive traversal

The core behavior can still be traced directly to the underlying system calls.

This makes the relationship between the Linux interface and the program's output easier to understand.

---

## 23. Documentation Is Part of the Engineering Work

The project treats documentation as part of implementation rather than an afterthought.

Each major filesystem concept should leave behind some combination of:

```text
experiment
implementation
test
design decision
lesson learned
```

For symbolic links, this produced:

```text
experiments/003-symbolic-links/
        |
        +--> observed stat()
        +--> observed lstat()
        +--> observed readlink()
        +--> observed broken-link behavior

src/main.c
        |
        +--> lstat()
        +--> readlink()

tests/test_info.sh
        |
        +--> valid symbolic link
        +--> broken symbolic link

docs/
        |
        +--> filesystem model
        +--> experiments
        +--> design decision
        +--> limitations
        +--> lessons learned
```

This creates a traceable relationship between what was learned and what was implemented.

---

## 24. Filesystem Identity Is Different from Path Identity

The project repeatedly encountered the difference between:

```text
path identity
```

and:

```text
object identity
```

A pathname identifies a location in the filesystem namespace.

An object can be identified through metadata such as:

```text
device
inode
```

Hard links demonstrated:

```text
different paths
        |
        v
same object
```

Symbolic links demonstrated:

```text
path
 |
 v
symbolic-link object
 |
 v
another pathname
 |
 v
target object
```

This distinction becomes increasingly important when studying traversal, caching, race conditions, and filesystem monitoring.

---

## 25. A Filesystem Is More Than Files and Directories

The early mental model of a filesystem can be too simple:

```text
files
directories
```

The experiments expanded that model to include:

```text
files
directories
hard links
symbolic links
directory entries
inodes
metadata
pathnames
filesystem objects
```

These concepts are related but are not interchangeable.

Understanding the relationships is more important than memorizing individual commands.

---

## 26. Symbolic Links Add Another Layer to Path Resolution

Without a symbolic link, the simplified path-resolution model is:

```text
pathname
    |
    v
directory entry
    |
    v
filesystem object
```

With a symbolic link:

```text
pathname
    |
    v
directory entry
    |
    v
symbolic-link object
    |
    v
stored target pathname
    |
    v
target object
```

Whether the target is reached depends on the operation being performed.

This explains why:

```text
stat()
```

and:

```text
lstat()
```

can produce different results for the same pathname.

---

## 27. The Current Implementation Reflects a Deliberate Semantic Choice

The current `fswatch info <path>` behavior is:

```text
inspect the filesystem object represented by the supplied pathname
```

For a symbolic link, that means:

```text
lstat()
    |
    v
symbolic-link object
```

followed by:

```text
readlink()
    |
    v
target pathname
```

This was chosen because it makes the distinction between a symbolic link and its target explicit.

The implementation does not attempt to hide this distinction.

---

## 28. The Project Has a Useful Engineering Feedback Loop

The current development process has become:

```text
learn
  |
  v
ask a precise question
  |
  v
build a controlled experiment
  |
  v
observe Linux
  |
  v
record the result
  |
  v
make an implementation decision
  |
  v
write the smallest implementation
  |
  v
automate the behavior
  |
  v
document the reasoning
```

This is more useful than simply implementing a feature and then trying to explain it afterward.

---

## 29. Filesystem Errors Are Part of the Model

An error is not merely something the program needs to handle.

It can reveal how the filesystem operation works.

For example:

```text
stat(broken_link)
    -> failure
```

initially looks like a missing-file problem.

But comparing it with:

```text
lstat(broken_link)
    -> success
```

reveals that the symbolic-link object exists while its target cannot be resolved.

The difference between successful and failed operations therefore provides information about the filesystem model.

---

## 30. Direct System Calls Reduce Hidden Behavior

Using interfaces such as:

```text
lstat()
readlink()
```

directly keeps the behavior visible.

A higher-level library could hide some of the distinctions that are important for learning.

At this stage, the direct approach is preferable because the goal is to understand:

```text
userspace program
        |
        v
system call
        |
        v
kernel filesystem layer
        |
        v
filesystem object
```

before introducing additional abstractions.

---

## 31. The Current Filesystem Mental Model

The current project can now be summarized as:

```text
PATHNAME
    |
    v
NAMESPACE LOOKUP
    |
    v
DIRECTORY ENTRY
    |
    v
FILESYSTEM OBJECT
    |
    +----------------------+
    |                      |
    v                      v
METADATA              OBJECT CONTENT
    |
    +--> device
    +--> inode
    +--> link count
    +--> size
    +--> type
```

Hard links create:

```text
multiple pathnames
        |
        v
same filesystem object
```

Symbolic links create:

```text
pathname
    |
    v
symbolic-link object
    |
    v
stored target pathname
    |
    v
target object
```

The selected system call determines which part of this relationship is observed.

---

## 32. Questions Answered So Far

### Does `stat()` follow a symbolic link?

Yes.

It follows the final symbolic link and reports metadata for the resolved object.

### Does `lstat()` follow a symbolic link?

No.

It reports metadata for the symbolic-link object itself.

### Can a symbolic link have its own inode?

Yes.

The symbolic link is itself a filesystem object.

### Can a symbolic link exist without its target?

Yes.

The link can remain after the target has been removed.

### Can a broken symbolic link be inspected?

Yes.

`lstat()` and `readlink()` can still operate on the link itself.

### Does `readlink()` return the target object's metadata?

No.

It returns the pathname stored inside the symbolic link.

### Does a symbolic link contain a copy of the target?

No.

It stores a pathname reference.

---

## 33. Current `fswatch` Behavior

The current command:

```text
./fswatch info <path>
```

uses:

```text
lstat()
```

for primary inspection.

For a normal object:

```text
lstat()
    |
    +--> metadata output
```

For a symbolic link:

```text
lstat()
    |
    +--> symbolic-link metadata
    |
    +--> readlink()
            |
            +--> target pathname
```

A broken symbolic link is therefore still inspectable.

This behavior is covered by the automated test suite.

---

## 34. Current Learning Boundary

The project has established a foundation for deeper filesystem work.

The next concepts can build on the current model:

* directory entries
* `opendir()`
* `readdir()`
* file descriptors
* `open()`
* `read()`
* `write()`
* permissions
* ownership
* timestamps
* path traversal
* filesystem mounts
* filesystem capacity
* filesystem events
* race conditions
* caching

These should continue to be approached through the same cycle:

```text
concept
    |
    v
experiment
    |
    v
observation
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


