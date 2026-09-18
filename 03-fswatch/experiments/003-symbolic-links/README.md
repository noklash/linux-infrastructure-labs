# Experiment 003: Symbolic Links

## Objective

Understand how Linux represents symbolic links and how `stat()`, `lstat()`, and `readlink()` behave when operating on them.

The experiment also examines what happens when the target of a symbolic link is removed.

## Question

What is the difference between:

* the symbolic link itself,
* the object referenced by the symbolic link,
* and the pathname stored inside the symbolic link?

## Files

* `compare.c` - C program that directly calls `stat()`, `lstat()`, and `readlink()`.
* `run.sh` - reproducible experiment runner.

Generated during the experiment:

* `target.txt` - regular file used as the symbolic-link target.
* `link.txt` - symbolic link pointing to `target.txt`.

Generated files are removed automatically when the experiment finishes.

## Method

The experiment performs these steps:

1. Compile `compare.c`.
2. Create `target.txt`.
3. Create `link.txt` as a symbolic link to `target.txt`.
4. Inspect both filesystem objects with `ls -li`.
5. Call `stat()`, `lstat()`, and `readlink()` on `target.txt`.
6. Call `stat()`, `lstat()`, and `readlink()` on `link.txt`.
7. Remove `target.txt`.
8. Inspect `link.txt` again.
9. Repeat `stat()`, `lstat()`, and `readlink()` on the broken symbolic link.
10. Remove generated files during script cleanup.

## Observations

### Target file

`target.txt` was created as a regular file.

The experiment reported:

* type: regular file
* size: 25 bytes
* link count: 1

`stat()` and `lstat()` returned the same inode because the pathname refers directly to a regular file.

`readlink()` failed with `Invalid argument` because `target.txt` is not a symbolic link.

### Symbolic link while the target exists

`link.txt` was created as a symbolic link to `target.txt`.

The experiment showed different inode numbers for the target and the symbolic link.

Example:

```text
target.txt
inode: 397277
type: regular file

link.txt
inode: 397278
type: symbolic link
```

The exact inode numbers are expected to change between runs.

The symbolic link had a size of 10 bytes because its stored target pathname was:

```text
target.txt
```

which is 10 bytes long.

### `stat()` on the symbolic link

`stat("link.txt", ...)` returned the metadata of `target.txt`.

The reported inode matched the target file:

```text
stat(link.txt)

inode: 397277
type: regular file
size: 25 bytes
```

This demonstrates that `stat()` follows the final symbolic link.

### `lstat()` on the symbolic link

`lstat("link.txt", ...)` returned metadata for the symbolic link itself:

```text
lstat(link.txt)

inode: 397278
type: symbolic link
size: 10 bytes
```

The inode differed from the target file.

This demonstrates that `lstat()` does not follow the final symbolic link.

### `readlink()` on the symbolic link

`readlink("link.txt", ...)` returned:

```text
target.txt
```

with a length of 10 bytes.

`readlink()` reads the pathname stored by the symbolic link. It does not follow the link to obtain the target's metadata.

### Broken symbolic link

After removing `target.txt`, `link.txt` still existed:

```text
397278 lrwxrwxrwx 1 gmma gmma 10 ... link.txt -> target.txt
```

`stat()` then failed:

```text
No such file or directory
```

because it attempted to follow the symbolic link to a target that no longer existed.

`lstat()` continued to succeed and reported the symbolic link's inode and metadata.

`readlink()` also continued to return:

```text
target.txt
```

This demonstrates that removing the target does not remove the symbolic-link object.

## Results

| Operation        | Target exists            | Target removed           |
| ---------------- | ------------------------ | ------------------------ |
| `stat(link)`     | follows target           | fails                    |
| `lstat(link)`    | returns symlink metadata | returns symlink metadata |
| `readlink(link)` | returns stored pathname  | returns stored pathname  |

## Filesystem Model

A symbolic link introduces an additional filesystem object:

```text
target.txt
    |
    +-- inode A
    +-- file data


link.txt
    |
    +-- inode B
    +-- stored pathname: "target.txt"
```

The two objects have different inode identities.

When `stat()` is used:

```text
link.txt
    |
    v
follow symbolic link
    |
    v
target.txt
    |
    v
return target metadata
```

When `lstat()` is used:

```text
link.txt
    |
    v
inspect symbolic-link object
    |
    v
return symlink metadata
```

When `readlink()` is used:

```text
link.txt
    |
    v
read stored pathname
    |
    v
"target.txt"
```

## Conclusion

A symbolic link is a filesystem object with its own inode and metadata.

It does not contain a copy of the target file's data. Instead, it stores a pathname referring to another object.

`stat()` follows the symbolic link and reports information about the target.

`lstat()` reports information about the symbolic link itself.

`readlink()` retrieves the pathname stored by the symbolic link.

A symbolic link can continue to exist after its target has been removed. In that state it becomes a broken symbolic link. `stat()` fails because the target cannot be resolved, while `lstat()` and `readlink()` can still operate on the symbolic link itself.

## Reproducibility

Run:

```bash
./run.sh
```

The experiment creates and removes its own temporary filesystem objects.

No manually created state is required.
