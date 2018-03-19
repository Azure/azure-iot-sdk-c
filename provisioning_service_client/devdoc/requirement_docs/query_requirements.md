#Query Requirements

##Overview

This module is used to manage data related to Provisioning Service queries

## Exposed API

```c
void queryResponse_free(PROVISIONING_QUERY_RESPONSE* query_resp);
```

## queryResponse_free

```c
void queryResponse_free(PROVISIONING_QUERY_RESPONSE* query_resp);
```

**SRS_PROV_QUERY_22_001: [** `queryResponse_free` shall free all memory in the structure pointed to by `query_resp` **]**