
---

# 10. `docs/lessons-learned.md`

**`docs/lessons-learned.md`**

```markdown
# Lessons Learned

This document records important technical lessons discovered during the project.

The goal is to preserve understanding rather than simply record implementation changes.

---

## Lesson 001: A Pathname Is Not the Object

A pathname is a mechanism for locating a filesystem object.

The pathname itself is not the file, directory, inode, or data.

This distinction becomes important when reasoning about links and pathname resolution.

---

## Lesson 002: Directories Are Filesystem Objects

A directory is not merely an abstract container provided by the shell.

It is a filesystem object with metadata.

Therefore it has properties such as:

- type
- permissions
- ownership
- timestamps
- size
- inode identity

---

## Lesson 003: `stat()` Retrieves Metadata

The `stat()` interface allows a userspace program to request metadata about a filesystem object.

The result is represented by:

```c
struct stat