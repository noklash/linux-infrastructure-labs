
---

# 8. `docs/filesystem-model.md`

**`docs/filesystem-model.md`**

```markdown
# Linux Filesystem Model

This document records the filesystem concepts required to understand `fswatch`.

The purpose is to establish the system model before adding more functionality.

---

## 1. A Pathname Is Not the Filesystem Object

A pathname is a name used to locate an object through the filesystem namespace.

For example:

```text
/home/gmma/project/README.md