Absolutely. That is better for this project.

We'll do **one file at a time**, and for every Markdown file I'll give you the **complete replacement file in one standalone code block**. No mixing multiple docs in one response.

Let's start with the documentation closest to the experiment itself:

## `experiments/002-hard-links/README.md`

Replace/create the file with this:

````markdown
# Experiment 002: Inode Identity and Hard Links

## Objective

Determine whether two different pathnames can refer to the same underlying filesystem object.

This experiment investigates:

- inode identity
- filesystem/device identity
- hard links
- link counts
- shared file data
- the relationship between directory entries and filesystem objects
- what happens when one hard link is removed

The experiment is performed using standard Linux commands before changing `fswatch`.

---

## Setup

Create the experiment directory:

```bash
mkdir -p experiments/002-hard-links
cd experiments/002-hard-links
````

Create the original file:

```bash
printf 'filesystem identity experiment\n' > original.txt
```

Create a hard link:

```bash
ln original.txt hardlink.txt
```

A hard link creates another directory entry that refers to the existing filesystem object.

---

## Initial Inspection

Inspect both files with `ls`:

```bash
ls -li original.txt hardlink.txt
```

Observed:

```text
397192 -rw-rw-r-- 2 gmma gmma 31 Sep 17 08:54 hardlink.txt
397192 -rw-rw-r-- 2 gmma gmma 31 Sep 17 08:54 original.txt
```

Both pathnames have:

* inode: `397192`
* link count: `2`
* size: `31` bytes

Inspect the relevant metadata directly:

```bash
stat -c 'name=%n dev=%d inode=%i links=%h size=%s' original.txt hardlink.txt
```

Observed:

```text
name=original.txt dev=2050 inode=397192 links=2 size=31
name=hardlink.txt dev=2050 inode=397192 links=2 size=31
```

---

## Observation 1: Both Pathnames Have the Same Inode

Both pathnames report:

```text
inode = 397192
```

This means they refer to the same inode.

They are therefore not two independent files that happen to contain identical data.

The relationship is:

```text
directory
├── original.txt ──────┐
│                      │
└── hardlink.txt ──────┤
                       ↓
                  inode 397192
```

The two names are different, but the underlying filesystem object is the same.

---

## Observation 2: Both Pathnames Have the Same Device Identity

Both pathnames report:

```text
dev = 2050
```

The device value identifies the filesystem/device containing the inode.

The combination of device and inode provides a useful filesystem-level identity:

```text
(device, inode)
(2050, 397192)
```

Both pathnames resolve to the same identity.

This gives us a stronger concept of object identity than the pathname alone.

---

## Observation 3: The Link Count Is Two

Both pathnames report:

```text
links = 2
```

The inode currently has two hard-link references.

Conceptually:

```text
inode 397192
    │
    ├── original.txt
    └── hardlink.txt

link count = 2
```

The two directory entries reference the same filesystem object.

---

## Observation 4: Both Names Share the Same Data

Append data through `hardlink.txt`:

```bash
printf 'changed through hardlink\n' >> hardlink.txt
```

Read `original.txt`:

```bash
cat original.txt
```

Read `hardlink.txt`:

```bash
cat hardlink.txt
```

Observed:

```text
filesystem identity experiment
changed through hardlink
```

Both pathnames show the same contents.

This demonstrates that the file data belongs to the underlying filesystem object rather than independently belonging to each pathname.

---

## Observation 5: Both Names Share the Same Metadata

Inspect both pathnames again:

```bash
stat -c '%n inode=%i links=%h size=%s' original.txt hardlink.txt
```

Observed:

```text
original.txt inode=397192 links=2 size=56
hardlink.txt inode=397192 links=2 size=56
```

The file size increased from:

```text
31 bytes
```

to:

```text
56 bytes
```

Both pathnames report the same size because they refer to the same filesystem object.

The inode also remains:

```text
397192
```

---

## Observation 6: Removing One Hard Link

Remove `hardlink.txt`:

```bash
rm hardlink.txt
```

Inspect the remaining pathname:

```bash
stat -c '%n inode=%i links=%h size=%s' original.txt
```

Observed:

```text
original.txt inode=397192 links=1 size=56
```

The inode remains:

```text
397192
```

The file size remains:

```text
56
```

The link count changes:

```text
2 → 1
```

The underlying filesystem object therefore remains because `original.txt` still references it.

---

## Filesystem Model

Before removing the hard link:

```text
Directory
│
├── original.txt ──────┐
│                      │
└── hardlink.txt ──────┤
                       ↓
                  inode 397192
                       │
                  file data
```

After removing `hardlink.txt`:

```text
Directory
│
└── original.txt ──────→ inode 397192
                              │
                          file data
```

The pathname `hardlink.txt` was removed.

The inode and its data remained because `original.txt` still referenced the object.

---

## Important Distinction

This experiment demonstrates an important difference:

```text
removing a pathname
        ≠
immediately destroying the filesystem object
```

Removing a hard link removes one directory entry and decreases the inode's link count.

Conceptually:

```text
remove pathname
       ↓
remove directory entry
       ↓
decrease link count
       ↓
are references remaining?
    ├── yes → object remains
    └── no  → object can be reclaimed
```

There are additional reference mechanisms, such as open file descriptors. Those will be investigated later.

---

## Pathname Versus Filesystem Object

The experiment changes the simple mental model:

```text
filename → file
```

into:

```text
pathname
    ↓
directory entry
    ↓
inode / filesystem object
    ↓
metadata + data
```

A pathname is therefore a way to locate a filesystem object.

It is not necessarily the unique identity of that object.

Multiple pathnames can refer to the same filesystem object.

---

## Hard Link Versus File Copy

A hard link is not a copy.

A copy creates a separate filesystem object:

```text
original.txt → object A

copy.txt     → object B
```

A hard link creates another directory entry pointing to the existing object:

```text
original.txt ──┐
               ├──→ object A
hardlink.txt ──┘
```

Writing through either hard-link pathname modifies the same object.

---

## Engineering Conclusions

This experiment established the following:

1. Different pathnames can refer to the same filesystem object.

2. Hard links share the same inode.

3. Hard links share the same file data.

4. Hard links share the same metadata stored in the underlying object.

5. `st_nlink` reflects the number of hard-link references.

6. `st_dev` and `st_ino` provide useful filesystem-level object identity.

7. Removing one hard link removes one pathname while leaving the object accessible through remaining links.

8. A pathname should not be treated as the unique identity of a filesystem object.

---

## Relationship to `fswatch`

The current `fswatch info` command reports:

```text
Size
Type
```

Experiment 002 shows that filesystem inspection also needs to expose object identity information.

The next implementation stage will add:

```text
Device
Inode
Links
```

using these fields from `struct stat`:

```c
st_dev
st_ino
st_nlink
```

The implementation will be followed by automated tests and another controlled experiment.

---

## Limitations

This experiment does not yet investigate:

* symbolic links
* `stat()` versus `lstat()`
* open file descriptors
* deleted-but-open files
* cross-filesystem hard-link restrictions
* inode allocation
* directory link semantics
* filesystem-specific inode behavior
* mount namespaces
* overlay filesystems

These behaviors will be investigated separately when they become relevant to the filesystem model.

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

The observed filesystem identity was:

```text
device = 2050
inode  = 397192
```

The experiment also demonstrated:

```text
create hard link
       ↓
link count increases

write through either pathname
       ↓
same filesystem object changes

remove one pathname
       ↓
link count decreases

remaining pathname
       ↓
object remains accessible
```

