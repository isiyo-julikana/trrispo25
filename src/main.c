#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

//Справка
static void print_usage(FILE *out, const char *prog) {
    fprintf(out,
            "Usage:\n"
            "  %s --help\n"
            "  %s --version\n"
            "  %s compress <file> [--level N] [--output <path>]\n"
            "  %s benchmark <file> [--max-level N]\n"
            "\n"
            "Commands:\n"
            "  compress     Сжать <file> на заданном уровне zstd.\n"
            "               Уровень по умолчанию — из " DEFAULT_CONFIG_PATH ".\n"
            "  benchmark    Сжать <file> на каждом уровне 1..N и напечатать\n"
            "               таблицу «уровень — размер — время».\n"
            "\n"
            "Options:\n"
            "  --level N       Уровень сжатия для compress (1..%d).\n"
            "  --max-level N   Максимальный уровень для benchmark (по умолчанию %d).\n"
            "  --output PATH   Выходной файл для compress (по умолчанию <file>.zst).\n"
            "  --help          Показать справку и выйти.\n"
            "  --version       Показать версию и выйти.\n",
            prog, prog, prog, prog,
            core_max_level(), core_max_level());
}

//Файломый ввод-вывод
static int read_file(const char *path, unsigned char **data, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "trrispo25: cannot open '%s': %s\n",
                path, strerror(errno));
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "trrispo25: cannot seek '%s': %s\n",
                path, strerror(errno));
        fclose(f);
        return 1;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fprintf(stderr, "trrispo25: cannot tell '%s': %s\n",
                path, strerror(errno));
        fclose(f);
        return 1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "trrispo25: cannot seek '%s': %s\n",
                path, strerror(errno));
        fclose(f);
        return 1;
    }

    unsigned char *buf = NULL;
    if (sz > 0) {
        buf = malloc((size_t)sz);
        if (!buf) {
            fprintf(stderr, "trrispo25: out of memory reading '%s'\n", path);
            fclose(f);
            return 1;
        }
        size_t got = fread(buf, 1, (size_t)sz, f);
        if (got != (size_t)sz) {
            fprintf(stderr, "trrispo25: short read on '%s'\n", path);
            free(buf);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    *data = buf;
    *size = (size_t)sz;
    return 0;
}

static int write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "trrispo25: cannot create '%s': %s\n",
                path, strerror(errno));
        return 1;
    }
    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        if (written != size) {
            fprintf(stderr, "trrispo25: write error on '%s': %s\n",
                    path, strerror(errno));
            fclose(f);
            return 1;
        }
    }
    if (fclose(f) != 0) {
        fprintf(stderr, "trrispo25: close error on '%s': %s\n",
                path, strerror(errno));
        return 1;
    }
    return 0;
}


//
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("trrispo25 %s\n", APP_VERSION);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: trrispo25 --help | --version\n");
        return 0;
    }
    fprintf(stderr, "Use --help\n");
    return 1;
}
