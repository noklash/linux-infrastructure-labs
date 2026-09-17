# Changelog

All meaningful project changes are recorded here.

## Unreleased

### Added

- Initial project structure.
- Initial filesystem inspection command.
- Reproducible Makefile build.
- Automated filesystem inspection tests.
- Filesystem engineering documentation.
- Filesystem metadata inspection for device, inode, link count, size, and type.
- Controlled filesystem experiments covering `stat()` metadata and hard links.
- Engineering documentation covering filesystem identity, hard links, and filesystem object semantics.

### Changed

- Expanded `fswatch info` to report `st_dev`, `st_ino`, and `st_nlink`.
- Expanded automated tests to validate filesystem identity metadata.
- Expanded project documentation to record filesystem experiments, lessons learned, architecture, design decisions, limitations, and troubleshooting.

### Planned

- Symbolic link inspection using `lstat()`.
- Ownership and permission inspection.
- Timestamp inspection.
- Directory traversal.
- Recursive filesystem inspection.
- Mount and filesystem boundary analysis.
- Storage allocation analysis.
- File descriptor and low-level I/O experiments.
- Pseudo-filesystem investigation.