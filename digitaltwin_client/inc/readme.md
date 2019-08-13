# Digital Twin Client header files

This folder contains the public and internal header files for the Digital Twin Client for the C SDK.

## Do not use the internal folder
The [internal](./internal) folder is only intended for building the Digital Twin SDK itself.  Its contents are not for public consumption and the headers may change without notice.  Your application code should not include any internal header directly.

## What are these MOCKABLE_FUNCTION and MU_DEFINE_ENUM things?
The `MOCKABLE_FUNCTION` references throughout the header files are used by the Digital Twin SDK unit testing framework.  Digital Twin is [extensively tested](../tests).  For production code (which is the default unless you're building a unit test), the MOCKABLE_FUNCTION effectively becomes a no-op which just produces a standard C function declaration.

`MU_DEFINE_ENUM` is a helper macro that is used to define both enum values and stringified values of these enums.  For instance:

```c
#define DIGITALTWIN_CLIENT_RESULT_VALUES                        \
        DIGITALTWIN_CLIENT_OK,                                  \
        ...

MU_DEFINE_ENUM(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
```

Creates a type `DIGITALTWIN_CLIENT_RESULT` with one possible value being `DIGITALTWIN_CLIENT_OK`.  Applications may directly translate this into a string via the `MU_ENUM_TO_STRING` macro, e.g.:

```c
DIGITALTWIN_CLIENT_RESULT result = DoSomething();
printf("Return Code=%s\n", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
```
