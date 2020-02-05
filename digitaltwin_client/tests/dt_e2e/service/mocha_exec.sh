#!/bin/bash
npm install
pwd

./node_modules/.bin/tsc dt_e2e_test.ts iothub_test_core.ts

node_modules/.bin/mocha dt_e2e_test.js --connectionString $IOTHUB_CONNECTION_STRING --e "../device/dt_e2e_exe" --timeout 120000 
