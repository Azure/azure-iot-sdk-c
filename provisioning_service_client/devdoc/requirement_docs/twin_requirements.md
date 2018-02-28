# Twin Requirements

## Overview

This module is used to manage data related to the Initial Twin model, used with the Provisioning Service.

## Exposed API

```c
typedef struct INITIAL_TWIN_TAG* INITIAL_TWIN_HANDLE;

INITIAL_TWIN_HANDLE initialTwin_create(const char* tags, const char* desired_properties);
void initialTwin_destroy(INITIAL_TWIN_HANDLE twin);

const char* initialTwin_getTags(INITIAL_TWIN_HANDLE twin);
int initialTwin_setTags(INITIAL_TWIN_HANDLE twin, const char* tags);
const char* initialTwin_getDesiredProperties(INITIAL_TWIN_HANDLE twin);
int initialTwin_setDesiredProperties(INITIAL_TWIN_HANDLE twin, const char* desired_properties);
```


## initialTwin_create

```c
INITIAL_TWIN_HANDLE initialTwin_create(const char* tags, const char* desired_properties);
```

**SRS_PROV_TWIN_22_001: [** If both `tags` and `desired_properties` are `NULL`, `initialTwin_create` shall fail and return `NULL` **]**

**SRS_PROV_TWIN_22_002: [** Passing a value of `"{}"` for `tags` or `desired_properties` is the same as passing `NULL` **]**

**SRS_PROV_TWIN_22_003: [** If allocating memory for the new initialTwin fails, `initialTwin_create` shall fail and return `NULL` **]**

**SRS_PROV_TWIN_22_004: [** Upon successful creation of a new initialTwin, `initialTwin_create` shall return a `INITIAL_TWIN_HANDLE` to access the model **]**


## initialTwin_destroy

```c
void initialTwin_destroy(INITIAL_TWIN_HANDLE twin);
```

**SRS_PROV_TWIN_22_005: [** `initialTwin_destroy` shall free all memory contained within the `twin` handle **]**


## initialTwin_getTags

```c
const char* initialTwin_getTags(INITIAL_TWIN_HANDLE twin);
``` 

**SRS_PROV_TWIN_22_006: [** If the given handle, `twin`, is `NULL`, or there are no tags in `twin`, `initialTwin_getTags` shall return `NULL` **]**

**SRS_PROV_TWIN_22_007: [** Otherwise, `initialTwin_getTags` shall return the tags contained in `twin` **]**


## initialTwin_setTags

```c
int initialTwin_setTags(INITIAL_TWIN_HANDLE twin, const char* tags);
```

**SRS_PROV_TWIN_22_008: [** If the given handle, `twin` is `NULL`, `initialTwin_setTags` shall fail and return a non-zero value **]**

**SRS_PROV_TWIN_22_009: [** The tags value of `twin` shall be set to the given value `tags` **]**

**SRS_PROV_TWIN_22_010: [** Passing a value of `"{}"` for `tags` is the same as passing `NULL`, which will clear any already set value **]**

**SRS_PROV_TWIN_22_011: [** If allocating memory in `twin` for the new value fails, `initialTwin_setTags` shall fail and return a non-zero value **]**

**SRS_PROV_TWIN_22_012: [** Upon success, `initialTwin_setTags` shall return `0` **]**


## initialTwin_getDesiredProperties

```c
const char* initialTwin_getDesiredProperties(INITIAL_TWIN_HANDLE twin);
``` 

**SRS_PROV_TWIN_22_013: [** If the given handle, `twin`, is `NULL`, or there are no desired properties in `twin`, `initialTwin_getDesiredProperties` shall return `NULL` **]**

**SRS_PROV_TWIN_22_014: [** Otherwise, `initialTwin_getDesiredProperties` shall return the desired properties contained in `twin` **]**


## initialTwin_setDesiredProperties

```c
int initialTwin_setDesiredProperties(INITIAL_TWIN_HANDLE twin, const char* desired_properties);
``` 

**SRS_PROV_TWIN_22_015: [** If the given handle, `twin` is `NULL`, `initialTwin_setDesiredProperties` shall fail and return a non-zero value **]**

**SRS_PROV_TWIN_22_016: [** The desired properties value of `twin` shall be set to the given value `desired_properties` **]**

**SRS_PROV_TWIN_22_017: [** Passing a value of `"{}"` for `desired_properties` is the same as passing `NULL`, which will clear any already set value **]**

**SRS_PROV_TWIN_22_018: [** If allocating memory in `twin` for the new value fails, `initialTwin_setDesiredProperties` shall fail and return a non-zero value **]**

**SRS_PROV_TWIN_22_019: [** Upon success, `initialTwin_setDesiredProperties` shall return `0` **]**