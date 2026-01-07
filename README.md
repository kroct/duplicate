# duplicate

Find all duplicate files in a directory and optionally delete them

## FAST

```
$ find ~/dir/ -type f | wc -l
4698
$ time duplicate ~/dir/ >/dev/null
real	0m 0.78s
user	0m 0.10s
sys	0m 0.68s
```

## Example

```
# find all duplicates
$ duplicate ~/dir/
# delete all duplicates
$ duplicate ~/dir/ | sh
```

dependencies
- uthash
- xxhash
