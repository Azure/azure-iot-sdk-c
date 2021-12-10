# master to main branch rename instructions

The Azure IoT C SDK initially used a branch named `master` as its primary branch.  This was renamed to `main` on December 1, 2021.

## Implications to tools and scripts referencing this repo
When a web request to an older link to this repo that contains `master` in it - for instance [https://github.com/Azure/azure-iot-sdk-c/blob/**master**/doc/master_to_main_rename.md](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/master_to_main_rename.md) - GitHub will automatically redirect this to `main`.

GitHub will **not** redirect tools, scripts, command line requests, etc. that explicitly reference the `master` branch, however.  These will instead fail accessing the deleted branch.  You will need to change these to reference this repo's `main` branch.

## Implications to local clones of this repo

**If you cloned this repo after December 1, 2021, you can ignore the rest of this document.**
  * Git clones after the default rename will use `main` and do the right thing.

**If you cloned this repo before December 1, 2021, You need to make changes so that your local copy of the repo will use `main` instead of `master`.**
  * You can just run a new `git clone` and delete your previous clone.  The new clone will use `main` as its primary branch.
  * Or if you want to keep your existing copy of the repo, then you will need to run these commands
  ```script
    git branch -m master main
    git fetch origin
    git branch -u origin/main main
    git remote set-head origin -a
  ```
