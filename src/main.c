#include <stdio.h>
#include <string.h>
#include "core.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

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
