# Base32 Requirements

================

## Overview

This module is used to encode an array of unsigned chars using the standard base32 encoding stream.

## References

[IETF RFC 4648](https://tools.ietf.org/html/rfc4648)

## Exposed API

```c
extern STRING_HANDLE Base32_Encode(BUFFER_HANDLE input);
extern char* Base32_Encode_Bytes(const unsigned char* source, size_t size);
BUFFER_HANDLE Base32_Decode(STRING_HANDLE handle);
BUFFER_HANDLE Base32_Decode_String(const char* source);
```

### Base32_Encode

```c
extern STRING_HANDLE Base32_Encode(BUFFER_HANDLE source);
```

**SRS_BASE32_07_001: [** If `source` is NULL `Base32_Encode` shall return NULL. **]**

**SRS_BASE32_07_002: [** If successful `Base32_Encode` shall return the base32 value of source. **]**

**SRS_BASE32_07_015: [** If `source` length is 0 `Base32_Encode` shall return an empty string. **]**

**SRS_BASE32_07_003: [** `Base32_Encode` shall call into `Base32_Encode_impl` to encode the source data. **]**

**SRS_BASE32_07_013: [** `Base32_Encode` shall wrap the `Base32_Encode_impl` result into a `STRING_HANDLE`. **]**

**SRS_BASE32_07_014: [** Upon failure `Base32_Encode` shall return NULL. **]**

### Base32_Encode_Bytes

```c
extern char* Base32_Encode_Bytes(const unsigned char* source, size_t size);
```

**SRS_BASE32_07_004: [** If `source` is NULL `Base32_Encode_Bytes` shall return NULL. **]**

**SRS_BASE32_07_005: [** If `size` is 0 `Base32_Encode_Bytes` shall return an empty string. **]**

**SRS_BASE32_07_006: [** If successful `Base32_Encode_Bytes` shall return the base32 value of input. **]**

**SRS_BASE32_07_007: [** `Base32_Encode_Bytes` shall call into `Base32_Encode_impl` to encode the source data. **]**

**SRS_BASE32_07_014: [** Upon failure `Base32_Encode_Bytes` shall return NULL. **]**

### base32_encode_impl

```c
static char* base32_encode_impl(const unsigned char* source, size_t src_size)
```

**SRS_BASE32_07_009: [** `base32_encode_impl` shall allocate the buffer to the size of the encoding value. **]**

**SRS_BASE32_07_010: [** `base32_encode_impl` shall look through `source` and separate each block into 5 bit chunks **]**

**SRS_BASE32_07_011: [** `base32_encode_impl` shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. **]**

**SRS_BASE32_07_012: [** If the `src_size` is not divisible by 8 and less than the 40-bits, `base32_encode_impl` shall pad the remaining places with `=`. **]**

### Base32_Decode_String

```c
extern BUFFER_HANDLE Base32_Decode_String(const char* source);
```

**SRS_BASE32_07_008: [** If source is NULL `Base32_Decode_String` shall return NULL. **]**

**SRS_BASE32_07_019: [** On success `Base32_Decode_String` shall return a BUFFER_HANDLE that contains the decoded bytes for source. **]**

**SRS_BASE32_07_020: [** `Base32_Decode_String` shall call `base32_decode_impl` to decode the base64 value. **]**

### Base32_Decode

```c
extern BUFFER_HANDLE Base32_Decode(STRING_HANDLE source);
```

**SRS_BASE32_07_016: [** If source is NULL `Base32_Decode` shall return NULL. **]**

**SRS_BASE32_07_017: [** On success `Base32_Decode` shall return a `BUFFER_HANDLE` that contains the decoded bytes for source. **]**

**SRS_BASE32_07_018: [** `Base32_Decode` shall call `base32_decode_impl` to decode the base64 value **]**

**SRS_BASE32_07_027: [** If the string in source value is NULL, `Base32_Decode` shall return NULL. **]**

### base32_decode_impl

```c
static BUFFER_HANDLE base32_decode_impl(const char* source)
```

**SRS_BASE32_07_021: [** If the source length is not evenly divisible by 8, `base32_decode_impl` shall return NULL. **]**

**SRS_BASE32_07_022: [** `base32_decode_impl` shall allocate a temp buffer to store the in process value. **]**

**SRS_BASE32_07_023: [** If an error is encountered, `base32_decode_impl` shall return NULL. **]**

**SRS_BASE32_07_024: [** `base32_decode_impl` shall loop through and collect 8 characters from the source variable. **]**

**SRS_BASE32_07_025: [** `base32_decode_impl` shall group 5 bytes at a time into the temp buffer. **]**

**SRS_BASE32_07_026: [** Once `base32_decode_impl` is complete it shall create a BUFFER with the temp buffer. **]**