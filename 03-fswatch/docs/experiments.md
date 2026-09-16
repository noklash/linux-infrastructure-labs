
---

# 9. `docs/experiments.md`

**`docs/experiments.md`**

```markdown
# Filesystem Experiments

This document indexes controlled experiments performed during the development of `fswatch`.

Experiments are used to investigate Linux behavior.

They are not substitutes for automated tests.

---

## Experiment Format

Each experiment should record:

1. Question
2. Hypothesis
3. Setup
4. Commands
5. Observation
6. Explanation
7. Conclusion

The goal is to distinguish what was observed from what was inferred.

---

## Experiment 001: Regular File vs Directory Metadata

### Question

How does `stat()` describe a regular file compared with a directory?

### Setup

Use:

```text
README.md
.