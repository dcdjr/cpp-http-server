# Git Cheatsheet (Local Reference)

This file is for personal use only.  
Keep it in your project folder, but add it to `.gitignore` so it never gets committed.

---

## BASIC WORKFLOW

### Initialize a new repo
```
git init
```

### Check status
```
git status
```

### Add files to staging
```
git add <file>
git add .        # add everything
```

### Commit
```
git commit -m "message"
```

### Push (first time)
```
git push -u origin main
```

### Push (after first time)
```
git push
```

### Pull latest changes
```
git pull
```

---

## REMOTES

### Add remote (GitHub)
```
git remote add origin <url>
```

### Check remotes
```
git remote -v
```

---

## BRANCHES

### Create a new branch
```
git checkout -b my-branch
```

### Switch branches
```
git checkout main
git checkout my-branch
```

### List branches
```
git branch
```

### Merge branch into current branch
```
git merge my-branch
```

### Delete branch
```
git branch -d my-branch
```

---

## VIEWING CHANGES

### See unstaged changes
```
git diff
```

### See staged changes
```
git diff --cached
```

### See commit history
```
git log
git log --oneline
git log --graph --oneline --decorate
```

---

## UNDO / FIX MISTAKES

### Unstage a file (undo `git add`)
```
git restore --staged <file>
```

### Restore a file to last commit
```
git restore <file>
```

### Undo last commit but keep changes
```
git reset --soft HEAD~1
```

### Undo last commit AND discard changes (dangerous)
```
git reset --hard HEAD~1
```

---

## STASH (TEMP SAVE)

### Save work without committing
```
git stash
```

### Reapply stashed work
```
git stash pop
```

### List stashes
```
git stash list
```

---

## CLONING

### Clone a repo
```
git clone <url>
```

---

## TAGS (OPTIONAL)

### Create a tag
```
git tag v1.0
```

### Push tags
```
git push --tags
```

---

## COMMON .gitignore ENTRIES
```
# Compiled
*.o
*.out
*.exe

# OS junk
.DS_Store
Thumbs.db

# Editors
.vscode/
.idea/

# Build directories
/build/
bin/

# Personal reference files
git_cheatsheet.md
```

---

## TL;DR WORKFLOW (MOST COMMON)
```
git add .
git commit -m "msg"
git push
```

