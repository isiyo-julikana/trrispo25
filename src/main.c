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
