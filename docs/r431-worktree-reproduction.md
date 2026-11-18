# R4.3.1 Worktree Reproduction

**Status: NOT RUN — awaiting explicit commit approval**

After approved freeze commit:

```bash
git worktree add --detach ../HaeunOS-r431-wt <freeze-sha>
cd ../HaeunOS-r431-wt
make clean && make all && make test
```

Record exit codes; do not copy primary `build/`. Then set status `WORKTREE_VERIFIED` if exit 0.
