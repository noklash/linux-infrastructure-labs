
---

**`docs/design-decisions.md`**

```markdown
# Design Decisions

This document records significant technical decisions made during the development of `fswatch`.

The purpose is to preserve reasoning, not merely record what the code currently does.

---

## Decision 001: Keep the Initial Implementation Small

### Decision

Keep the initial implementation in `src/main.c`.

### Reason

The program currently has very few responsibilities.

Splitting the implementation into multiple files at this stage would create structure without a corresponding engineering need.

### Consequence

The project can be reorganized later when responsibilities become sufficiently independent.

---

## Decision 002: Use Linux System Interfaces Directly

### Decision

Use Linux and POSIX filesystem interfaces directly rather than shelling out to existing utilities.

Examples include:

- `stat()`
- `lstat()`
- `opendir()`
- `readdir()`
- `closedir()`
- `open()`
- `read()`
- `write()`
- `close()`

### Reason

The purpose of the project is to understand how Linux filesystem operations work.

Calling utilities such as `ls`, `find`, `du`, or `stat` from inside the program would hide the mechanisms we are trying to understand.

### Consequence

More implementation work is required, but the resulting project provides stronger evidence of systems-level understanding.

---

## Decision 003: Separate Experiments from Tests

### Decision

Keep controlled experiments under `experiments/` and automated verification under `tests/`.

### Reason

They answer different questions.

A test verifies program behavior.

An experiment investigates system behavior.

For example:

```text
Test:
Does fswatch correctly identify a directory?

Experiment:
Why does stat() report a symbolic link as the type of its target?