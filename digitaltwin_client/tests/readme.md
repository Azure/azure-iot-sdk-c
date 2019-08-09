# Testing Digital Twin C SDK

In general these tests are intended for those developing the core Digital Twin SDK, but may be run by anyone.  

These tests are not built by default.  To build them, you'll need the `cmake` options `-Drun_e2e_tests=ON -Drun_unittests=ON`.  On Linux systems with Valgrind tools installed, `-Drun_valgrind:BOOL=ON` enables additional checking via Valgrind.