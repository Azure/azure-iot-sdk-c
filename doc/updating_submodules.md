# Updating submodules for the C SDK

## Commands

```bash
git checkout master
git pull

cd c-utility
                git checkout master
                git pull
                git submodule update --init --recursive
                cd ..

cd uamqp
                git checkout master
                git pull
                cd deps/azure-c-shared-utility/
                                git checkout master
                                git pull
                                git submodule update --init --recursive
                                cd ..

                git checkout -b SubmoduleUpdate
                git add c-utility
                git commit -m "Update azure-c-sharedutils reference"
                git push --set-upstream origin SubmoduleUpdate
                cd ..

cd umqtt
                git checkout master
                git pull
                cd deps/c-utility
                                git checkout master
                                git pull
                                git submodule update --init --recursive
                                cd ..

                git checkout -b SubmoduleUpdate
                git add c-utility
                git commit -m "Update azure-c-sharedutils reference"
                git push --set-upstream origin SubmoduleUpdate
                cd ..

cd deps/uhttp
                git checkout master
                git pull

                cd deps/c-utility
                                git checkout master
                                git pull
                                git submodule update --init --recursive
                                cd ....

                git checkout -b SubmoduleUpdate
                git add deps/c-utility
                git commit -m "Update azure-c-sharedutils reference"
                git push --set-upstream origin SubmoduleUpdate
                cd ....

cd provisioning_client/deps/utpm
                git checkout master
                git pull
                cd deps/c-utility
                                git checkout master
                                git pull
                                git submodule update --init --recursive
                                cd ....

                git checkout -b SubmoduleUpdate
                git add deps/c-utility
                git commit -m "Update azure-c-sharedutils reference"
                git push --set-upstream origin SubmoduleUpdate
                cd ......

git add c-utility
git add uamqp
git add umqtt
git add deps/uhttp
git add provisioning_client/deps/utpm
git commit -m "Update submodule references"
git push --set-upstream origin SubmoduleUpdate
```

## Testing

The following requires NodeJS and NPM.

`npm install check_submodules`

Run:

`/node_modules/.bin/check_submodules . master`
