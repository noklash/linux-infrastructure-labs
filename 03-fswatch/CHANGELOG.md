# Changelog

All notable changes to `fswatch` are documented here.

The project is being developed as a filesystem learning tool. Each meaningful change should connect an observed Linux behavior to an implementation, test, experiment, or engineering decision.

---

## Unreleased

### Planned

- Make `fswatch info` inspect symbolic links with `lstat()`
- Report symbolic-link targets with `readlink()`
- Test valid symbolic links
- Test broken symbolic links
- Ownership and group information
- Permission and mode inspection
- Timestamp inspection
- Directory traversal
- Directory stream handling
- File descriptor and I/O experiments
- `open()`, `read()`, `write()`, and `close()`
- Deleted-but-open file behavior
- Filesystem and mount boundaries
- Pseudo-filesystems
- Allocated storage and block usage
- Sparse-file behavior
- Filesystem capacity
- Additional filesystem error handling

---

## 2026-09-18

### Added

- Symbolic-link filesystem experiment under `experiments/003-symbolic-links/`
- Direct comparison of `stat()`, `lstat()`, and `readlink()`
- Reproducible symbolic-link experiment
- Reproducible broken-symbolic-link experiment
- Documentation of symbolic-link behavior
- Documentation of the difference between a symbolic-link object and its target object

### Experiment Results

The symbolic-link experiment established:

```text
stat()
    → follows the symbolic link
    → reports target metadata

lstat()
    → does not follow the symbolic link
    → reports symbolic-link metadata

readlink()
    → reads the stored target pathname
    → does not follow the link