#ifndef TRRISPO25_CORE_H
#define TRRISPO25_CORE_H

#include <stddef.h>
#include <stdint.h>

int core_compress_buffer(const void *src, size_t src_size, int level,
                         void **dst, size_t *dst_size,
                         uint64_t *elapsed_ns);

/* Максимальный поддерживаемый уровень сжатия в данной сборке zstd. */
int core_max_level(void);

#endif
