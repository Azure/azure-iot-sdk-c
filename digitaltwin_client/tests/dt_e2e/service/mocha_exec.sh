#!/bin/bash
npm install
pwd

./node_modules/.bin/tsc dt_e2e_test.ts iothub_test_core.ts

if [ -d ../device/Release ]; then 
    node_modules/.bin/mocha dt_e2e_test.js --connectionString $IOTHUB_CONNECTION_STRING --e "../device/Release/dt_e2e_exe" --timeout 120000 
elif [ -d ../device/Debug ]; then
    node_modules/.bin/mocha dt_e2e_test.js --connectionString $IOTHUB_CONNECTION_STRING --e "../device/Debug/dt_e2e_exe" --timeout 120000
else
    node_modules/.bin/mocha dt_e2e_test.js --connectionString $IOTHUB_CONNECTION_STRING --e "../device/dt_e2e_exe" --timeout 120000 
fi

