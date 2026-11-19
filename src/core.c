#include "core.h"
#include <stdlib.h>
#include <time.h>
#include <zstd.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int core_max_level(void) {
    return ZSTD_maxCLevel();
}

int core_compress_buffer(const void *src, size_t src_size, int level,
                         void **dst, size_t *dst_size,
                         uint64_t *elapsed_ns) {
    if (!src || !dst || !dst_size || !elapsed_ns) return 1;
    if (level < 1 || level > ZSTD_maxCLevel()) return 2;

    size_t const bound = ZSTD_compressBound(src_size);
    void *buf = malloc(bound);
    if (!buf) return 3;

    uint64_t const t0 = now_ns();
    size_t const c_size = ZSTD_compress(buf, bound, src, src_size, level);
    uint64_t const t1 = now_ns();

    if (ZSTD_isError(c_size)) {
        free(buf);
        return 4;
    }

    *dst = buf;
    *dst_size = c_size;
    *elapsed_ns = t1 - t0;
    return 0;
}
