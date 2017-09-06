# Base32 Requirements

================

## Overview

This module is used to encode an array of unsigned chars using the standard base32 encoding stream.

## References

[IETF RFC 4648](https://tools.ietf.org/html/rfc4648)

## Exposed API

```c
extern STRING_HANDLE Base32_Encoder(BUFFER_HANDLE input);
extern char* Base32_Encode_Bytes(const unsigned char* source, size_t size);
extern BUFFER_HANDLE Base32_Decoder(const char* source);
```

### Base32_Encoder

```c
extern STRING_HANDLE Base32_Encoder(BUFFER_HANDLE source);
```

**SRS_BASE32_07_001: [** If `source` is NULL `Base32_Encoder` shall return NULL. **]**

**SRS_BASE32_07_002: [** If successful `Base32_Encoder` shall return the base32 value of source. **]**

**SRS_BASE32_07_015: [** If `source` length is 0 `Base32_Encoder` shall return an empty string. **]**

**SRS_BASE32_07_003: [** `Base32_Encoder` shall call into `base32_encoder_impl` to encode the source data. **]**

**SRS_BASE32_07_013: [** `Base32_Encoder` shall wrap the `base32_encoder_impl` result into a `STRING_HANDLE`. **]**

**SRS_BASE32_07_014: [** Upon failure `Base32_Encoder` shall return NULL. **]**

### Base32_Encode_Bytes

```c
extern char* Base32_Encode_Bytes(const unsigned char* source, size_t size);
```

**SRS_BASE32_07_004: [** If `source` is NULL `Base32_Encode_Bytes` shall return NULL. **]**

**SRS_BASE32_07_005: [** If `size` is 0 `Base32_Encode_Bytes` shall return an empty string. **]**

**SRS_BASE32_07_006: [** If successful `Base32_Encode_Bytes` shall return the base32 value of input. **]**

**SRS_BASE32_07_007: [** `Base32_Encode_Bytes` shall call into `base32_encoder_impl` to encode the source data. **]**

**SRS_BASE32_07_014: [** Upon failure `Base32_Encode_Bytes` shall return NULL. **]**

### base32_encode_impl

```c
static char* base32_encode_impl(const unsigned char* source, size_t src_size)
```

**SRS_BASE32_07_009: [** `base32_encode_impl` shall allocate the buffer to the size of the encoding value. **]**

**SRS_BASE32_07_010: [** `base32_encode_impl` shall look through `source` and separate each block into 5 bit chunks **]**

**SRS_BASE32_07_011: [** `base32_encode_impl` shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. **]**

**SRS_BASE32_07_012: [** If the `src_size` is not divisible by 8 and less than the 40-bits, `base32_encode_impl` shall pad the remaining places with `=`. **]**

### Base32_Decoder

```c
extern BUFFER_HANDLE Base32_Decoder(const char* source);
```

**SRS_BASE32_07_008: [** Is not currently implemented. **]**
