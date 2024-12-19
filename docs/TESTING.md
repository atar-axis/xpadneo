## Testing and Development

### Working with Pull Requests

If working with pull requests, do not clone a fork of the other repository.
The installer needs the original version tags to properly work and to have
correct version tags that have not been tampered with.

It is always recommended to add additional remotes for the forks to your
already existing local clone.

To do this, follow these steps. This example uses kakra/xpadneo as a fork:
```bash
# checkout a clone from the original repository
git clone https://github.com/atar-axis/xpadneo.git
cd xpadneo

# add the fork for the pull request as a clone
git remote add kakra https://github.com/kakra/xpadneo.git
git remote update

# option 1: switch to another branch within this directory
git switch -C local/branch/name kakra/remote/branch/name

# option 2: setup a git workdir to use an isolated directory
git worktree add ../xpadneo-kakra -B local/branch/name kakra/remote/branch/name

# to update to the latest version of a pull request
git remote update --prune

# then reset your outdated copy of the branch
git reset --merge kakra/remote/branch/name # tries keeping your local modifications
git reset --hard kakra/remote/branch/name  # discards your local modifications
```

