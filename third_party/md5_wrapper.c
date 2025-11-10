#include "picohash.h"

void md5_hash(const void* data, size_t length, unsigned char* digest) {
    _picohash_md5_ctx_t ctx;
    _picohash_md5_init(&ctx);
    _picohash_md5_update(&ctx, data, length);
    _picohash_md5_final(&ctx, digest);
}
