#ifndef _SHA1_H
#define _SHA1_H

/*
  https://github.com/clibs/sha1

   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

#include "stdint.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} _SHA1_CTX;

void _SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void _SHA1Init(
    _SHA1_CTX * context
    );

void _SHA1Update(
    _SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void _SHA1Final(
    unsigned char digest[20],
    _SHA1_CTX * context
    );

void _SHA1(
    char *hash_out,
    const char *str,
    uint32_t len);

#if defined(__cplusplus)
}
#endif

#endif /* _SHA1_H */
