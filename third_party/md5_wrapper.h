#ifndef MD5_WRAPPER_H
#define MD5_WRAPPER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MD5_DIGEST_LENGTH 16

void md5_hash(const void* data, size_t length, unsigned char* digest);

#ifdef __cplusplus
}
#endif

#endif // MD5_WRAPPER_H
