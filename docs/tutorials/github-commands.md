# Git and GitHub commands

> This is a general command cheat sheet. It does not define the project's branch,
> review, or release policy.

## Branching
```
git branch → list branches

git branch <branch> → create a new branch

git checkout <branch> → switch to a branch

git checkout -b <branch> → create & switch to a new branch

git merge <branch> → merge another branch into current

git branch -d <branch> → delete a branch locally

git push origin --delete <branch> → delete branch on GitHub



```

## To push code

To push code, please run the following three lines of commands
```
git add <file> → stage a file.

git add . → stage all files.

git commit -m "message" → save staged changes with a message, you can write whatever message you want.
```

### `CRLF will be replaced by LF` warning

This warning means Git found Windows-style `CRLF` line endings in the working
file, while this repository's `.gitattributes` requires `LF` line endings in
Git. When the file is staged or next rewritten by Git, only the line-ending
representation is normalized; it is not a compiler error and does not mean the
program logic changed.

For this project, do not bulk-rewrite CubeMX-generated or frozen low-level
files merely to remove the warning. Before committing, use `git diff --cached`
to confirm that the staged change is only the expected line-ending
normalization.


## Pulling
```
git fetch → download all branches and updates

git pull → fetch and merge latest changes from remote
```
