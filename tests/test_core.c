#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        goto fail; \
    } \
} while (0)

/* ------------------------------------------------------------------ */
/* Отдельные проверки                                                 */
/* ------------------------------------------------------------------ */

/* (1) core_max_level() > 0 */
static int test_max_level(void) {
    int result = 1;
    CHECK(core_max_level() > 0);
    result = 0;
fail:
    return result;
}

/* (2) Сжатие непустого буфера на уровне 1: rc=0, dst != NULL, dst_size > 0,
 *     elapsed_ns > 0, сжатый меньше исходного (текст сжимаемый). */
static int test_compress_basic(void) {
    const char *text =
        "The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. "
        "Pack my box with five dozen liquor jugs.";
    size_t len = strlen(text);

    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer(text, len, 1, &dst, &dst_size, &elapsed) == 0);
    CHECK(dst != NULL);
    CHECK(dst_size > 0);
    CHECK(elapsed > 0);
    CHECK(dst_size < len);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (3) Уровень 3 даёт результат не больше, чем уровень 1,
 *     на хорошо сжимаемых данных. */
static int test_higher_level_not_larger(void) {
    static char data[16384];
    for (size_t i = 0; i < sizeof data; ++i) {
        data[i] = (char)('a' + (i % 26));
    }

    void *d1 = NULL, *d3 = NULL;
    size_t s1 = 0, s3 = 0;
    uint64_t e1 = 0, e3 = 0;
    int result = 1;

    CHECK(core_compress_buffer(data, sizeof data, 1, &d1, &s1, &e1) == 0);
    CHECK(core_compress_buffer(data, sizeof data, 3, &d3, &s3, &e3) == 0);
    CHECK(s1 > 0);
    CHECK(s3 > 0);
    CHECK(s3 <= s1);
    result = 0;

fail:
    free(d1);
    free(d3);
    return result;
}

/* (4) Максимальный уровень: сжатие проходит успешно. */
static int test_compress_max_level(void) {
    const char *text = "sample text for max level compression";
    size_t len = strlen(text);

    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer(text, len, core_max_level(),
                               &dst, &dst_size, &elapsed) == 0);
    CHECK(dst != NULL);
    CHECK(dst_size > 0);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (5) Уровень 0 — ошибка (код 2). */
static int test_invalid_level_low(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("x", 1, 0, &dst, &dst_size, &elapsed) == 2);
    CHECK(dst == NULL);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (6) Уровень core_max_level() + 1 — ошибка (код 2). */
static int test_invalid_level_high(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("x", 1, core_max_level() + 1,
                               &dst, &dst_size, &elapsed) == 2);
    CHECK(dst == NULL);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (7) src == NULL — ошибка (код 1). */
static int test_null_src(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer(NULL, 10, 1, &dst, &dst_size, &elapsed) == 1);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (8) dst == NULL — ошибка (код 1). */
static int test_null_dst(void) {
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("x", 1, 1, NULL, &dst_size, &elapsed) == 1);
    result = 0;

fail:
    return result;
}

/* (9) dst_size == NULL — ошибка (код 1). */
static int test_null_dst_size(void) {
    void *dst = NULL;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("x", 1, 1, &dst, NULL, &elapsed) == 1);
    result = 0;

fail:
    return result;
}

/* (10) elapsed_ns == NULL — ошибка (код 1). */
static int test_null_elapsed(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    int result = 1;

    CHECK(core_compress_buffer("x", 1, 1, &dst, &dst_size, NULL) == 1);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (11) Пустой вход: src_size == 0, src != NULL — успех,
 *      zstd пишет как минимум заголовок фрейма, значит dst_size > 0. */
static int test_empty_input(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("", 0, 3, &dst, &dst_size, &elapsed) == 0);
    CHECK(dst != NULL);
    CHECK(dst_size > 0);
    CHECK(elapsed > 0);
    result = 0;

fail:
    free(dst);
    return result;
}

/* (12) Освобождение dst после успешного сжатия — проверяется ASan:
 *      если память валидна, free() не падает и утечек нет. */
static int test_free_after_success(void) {
    void *dst = NULL;
    size_t dst_size = 0;
    uint64_t elapsed = 0;
    int result = 1;

    CHECK(core_compress_buffer("payload", 7, 5, &dst, &dst_size, &elapsed) == 0);
    CHECK(dst != NULL);
    result = 0;

fail:
    free(dst);
    return result;
}

/* ------------------------------------------------------------------ */
/* Раннер                                                             */
/* ------------------------------------------------------------------ */

typedef int (*test_fn)(void);

int main(void) {
    static const struct {
        const char *name;
        test_fn fn;
    } tests[] = {
        {"max_level",               test_max_level},
        {"compress_basic",          test_compress_basic},
        {"higher_level_not_larger", test_higher_level_not_larger},
        {"compress_max_level",      test_compress_max_level},
        {"invalid_level_low",       test_invalid_level_low},
        {"invalid_level_high",      test_invalid_level_high},
        {"null_src",                test_null_src},
        {"null_dst",                test_null_dst},
        {"null_dst_size",           test_null_dst_size},
        {"null_elapsed",            test_null_elapsed},
        {"empty_input",             test_empty_input},
        {"free_after_success",      test_free_after_success},
    };

    size_t total = sizeof tests / sizeof tests[0];
    size_t failed = 0;

    for (size_t i = 0; i < total; ++i) {
        printf("[ RUN  ] %s\n", tests[i].name);
        int rc = tests[i].fn();
        if (rc == 0) {
            printf("[ PASS ] %s\n", tests[i].name);
        } else {
            printf("[ FAIL ] %s\n", tests[i].name);
            ++failed;
        }
    }

    printf("\n%zu/%zu tests passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}
