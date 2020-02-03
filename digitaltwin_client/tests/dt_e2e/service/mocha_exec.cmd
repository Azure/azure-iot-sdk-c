
call npm init -y
call npm install typescript
call npm install chai
call npm install mocha
call npm install @types/mocha
call npm install @types/node
call npm install yargs
call npm install azure-iot-common
call npm install azure-iothub
call npm install @azure/ms-rest-js
call npm install terminate
call npm install @azure/event-hubs@^2.1.1
call npm install azure-iot-digitaltwins-service@pnp-preview
call dir

call node_modules\.bin\tsc.cmd dt_e2e_test.ts iothub_test_core.ts
IF EXIST ..\device\Release ( node_modules\.bin\mocha.cmd dt_e2e_test.js --connectionString %IOTHUB_CONNECTION_STRING% --e "..\\device\\Release\\dt_e2e_exe.exe" --timeout 120000 
) ELSE ( node_modules\.bin\mocha.cmd dt_e2e_test.js --connectionString %IOTHUB_CONNECTION_STRING% --e "..\\device\\Debug\\dt_e2e_exe.exe" --timeout 120000 )
)
