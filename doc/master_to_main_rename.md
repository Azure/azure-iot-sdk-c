# master to main branch rename instructions

The Azure IoT C SDK initially used a branch named `master` as its primary branch.  This was renamed to `main` in November, 2021.

**If you cloned this repo after November, 2021, you can ignore the rest of this document.**
  * Git clones after the default rename will use `main` and do the right thing.

**If you cloned this repo before November, 2021, You need to make changes so that your local copy of the repo will use `main` instead of `master`.**
  * You can just run a new `git clone` and delete your previous clone.  The new clone will use `main` as its primary branch.
  * Or if you want to keep your existing copy of the repo, then you will need to run these commands
  ```script
	git branch -m master main
	git fetch origin
	git branch -u origin/main main
    git remote set-head origin -a
  ```
