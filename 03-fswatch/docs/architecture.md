# Architecture

## Purpose

`fswatch` is a small Linux filesystem inspection utility written in C.

The project is intentionally developed incrementally so that each implementation feature corresponds to a filesystem concept that has been studied, implemented, tested, and documented.

## Current architecture

```text
User
  |
  | fswatch info <path>
  v
src/main.c
  |
  | stat()/lstat()
  v
Linux userspace API
  |
  v
Linux kernel
  |
  v
Filesystem
  |
  v
struct stat
  |
  +-- st_mode
  +-- st_size
  +-- st_ino
  +-- st_dev
  +-- st_uid
  +-- st_gid
  +-- ...