# Bulk Operation Requirements

##Overview

This module is used to manage data for Bulk Operations, used with the Provisioning Service

##Exposed API

```c
void bulkOperationResult_free(PROVISIONING_BULK_OPERATION_RESULT* bulk_op_result);
```


### bulkOperationResult_free

```c
void bulkOperationResult_free(PROVISIONING_BULK_OPERATION_RESULT* bulk_op_result);
```

**SRS_PROV_BULK_OPERATION_22_001: [** `bulkOperationResult_free` shall free all memory in the structure pointed to by `bulk_op_result`**]**