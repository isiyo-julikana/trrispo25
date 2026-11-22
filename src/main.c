#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

#define DEFAULT_CONFIG_PATH "/etc/trrispo25/trrispo25.conf"
#define DEFAULT_LEVEL 3

//Справка
static void print_usage(FILE *out, const char *prog) {
    fprintf(out,
            "Usage:\n"
            "  %s --help\n"
            "  %s --version\n"
            "  %s compress <file> [--level N] [--output <path>]\n"
            "  %s benchmark <file> [--max-level N]\n"
            "  %s serve [port]      Запустить HTTP-сервер (по умолчанию порт 8080).\n"
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

//Конфигурация
static int parse_config_level(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return DEFAULT_LEVEL;

    int level = DEFAULT_LEVEL;
    int max_allowed = core_max_level();
    char line[256];

    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';

        char *key = p;
        char *key_end = key + strlen(key);
        while (key_end > key && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
            *--key_end = '\0';
        }
        if (strcmp(key, "level") != 0) continue;

        char *val = eq + 1;
        while (*val == ' ' || *val == '\t') ++val;

        char *end = NULL;
        long parsed = strtol(val, &end, 10);
        if (end == val) continue;
        if (parsed < 1 || parsed > max_allowed) continue;
        level = (int)parsed;
    }
    fclose(f);
    return level;
}

//Доп.команды
static int cmd_compress(const char *input, int level, const char *output) {
    unsigned char *data = NULL;
    size_t data_size = 0;
    if (read_file(input, &data, &data_size) != 0) return 1;

    /* Пустой файл: передаём гарантированно непустой указатель. */
    const void *src = data ? (const void *)data : (const void *)"";

    void *cdata = NULL;
    size_t cdata_size = 0;
    uint64_t elapsed_ns = 0;

    int rc = core_compress_buffer(src, data_size, level,
                                  &cdata, &cdata_size, &elapsed_ns);
    free(data);
    if (rc != 0) {
        fprintf(stderr,
                "trrispo25: compression failed (level %d, rc=%d)\n",
                level, rc);
        return 1;
    }

    if (write_file(output, cdata, cdata_size) != 0) {
        free(cdata);
        return 1;
    }
    free(cdata);

    printf("%s -> %s: %zu -> %zu bytes (level %d, %.2f ms)\n",
           input, output, data_size, cdata_size, level,
           (double)elapsed_ns / 1e6);
    return 0;
}

static int cmd_benchmark(const char *input, int max_level) {
    unsigned char *data = NULL;
    size_t data_size = 0;
    if (read_file(input, &data, &data_size) != 0) return 1;

    const void *src = data ? (const void *)data : (const void *)"";

    int abs_max = core_max_level();
    if (max_level < 1) max_level = 1;
    if (max_level > abs_max) max_level = abs_max;

    printf("File: %s (%zu bytes)\n", input, data_size);
    printf("%-8s %-14s %-10s\n", "Level", "Size", "Time,ms");
    printf("%-8s %-14s %-10s\n", "------", "--------------", "----------");

    for (int level = 1; level <= max_level; ++level) {
        void *cdata = NULL;
        size_t cdata_size = 0;
        uint64_t elapsed_ns = 0;

        int rc = core_compress_buffer(src, data_size, level,
                                      &cdata, &cdata_size, &elapsed_ns);
        if (rc != 0) {
            fprintf(stderr, "trrispo25: level %d failed (rc=%d)\n", level, rc);
            continue;
        }
        printf("%-8d %-14zu %-10.2f\n",
               level, cdata_size, (double)elapsed_ns / 1e6);
        free(cdata);
    }

    free(data);
    return 0;
}

//Аргументы доп.команд
static int do_compress(int argc, char **argv) {
    const char *input = NULL;
    const char *output = NULL;
    int level = -1;  /* -1 = не задан на CLI, возьмём из конфига */

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "trrispo25: --level requires an argument\n");
                return 1;
            }
            char *end = NULL;
            long v = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || v < 1) {
                fprintf(stderr, "trrispo25: invalid level '%s'\n", argv[i]);
                return 1;
            }
            level = (int)v;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "trrispo25: --output requires an argument\n");
                return 1;
            }
            output = argv[++i];
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "trrispo25: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (!input) {
            input = argv[i];
        } else {
            fprintf(stderr, "trrispo25: unexpected argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!input) {
        fprintf(stderr, "trrispo25: compress: missing input file\n");
        return 1;
    }

    if (level < 0) level = parse_config_level(DEFAULT_CONFIG_PATH);
    if (level < 1 || level > core_max_level()) {
        fprintf(stderr, "trrispo25: level %d out of range (1..%d)\n",
                level, core_max_level());
        return 1;
    }

    char *default_output = NULL;
    if (!output) {
        size_t n = strlen(input) + sizeof(".zst");  /* + NUL */
        default_output = malloc(n);
        if (!default_output) {
            fprintf(stderr, "trrispo25: out of memory\n");
            return 1;
        }
        snprintf(default_output, n, "%s.zst", input);
        output = default_output;
    }

    int rc = cmd_compress(input, level, output);
    free(default_output);
    return rc;
}

static int do_benchmark(int argc, char **argv) {
    const char *input = NULL;
    int max_level = core_max_level();

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--max-level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "trrispo25: --max-level requires an argument\n");
                return 1;
            }
            char *end = NULL;
            long v = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || v < 1) {
                fprintf(stderr, "trrispo25: invalid max-level '%s'\n",
                        argv[i]);
                return 1;
            }
            max_level = (int)v;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "trrispo25: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (!input) {
            input = argv[i];
        } else {
            fprintf(stderr, "trrispo25: unexpected argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!input) {
        fprintf(stderr, "trrispo25: benchmark: missing input file\n");
        return 1;
    }
    return cmd_benchmark(input, max_level);
}

//Вход
int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(stderr, argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(stdout, argv[0]);
        return 0;
    }
    if (strcmp(cmd, "--version") == 0) {
        printf("trrispo25 %s\n", APP_VERSION);
        return 0;
    }
    if (strcmp(cmd, "compress") == 0) {
        return do_compress(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "benchmark") == 0) {
        return do_benchmark(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "serve") == 0) {
    int port = 8080;
        if (argc >= 3) {
        port = atoi(argv[2]);
        	if (port <= 0) port = 8080;
    }
    return run_server(port);
}
    fprintf(stderr, "trrispo25: unknown command '%s'\n", cmd);
    print_usage(stderr, argv[0]);
    return 1;
}
