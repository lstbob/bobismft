#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "commands.h"

#define VERSION "0.1.0"

static void print_usage(void) {
    printf("ohbobi -- diet planner v%s\n", VERSION);
    printf("Usage:\n");
    printf("  ohbobi nmp          New meal plan (default: tomorrow)\n");
    printf("  ohbobi nmp -w       New weekly meal plan\n");
    printf("  ohbobi smp          Show meal plan grid\n");
    printf("  ohbobi -h, --help   Show this help\n");
    printf("  ohbobi -v, --version Show version\n");
}

int main(int argc, char *argv[]) {
    if (!storage_init()) {
        fprintf(stderr, "error: could not initialize data storage\n");
        return 1;
    }

    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("ohbobi v%s\n", VERSION);
        return 0;
    }

    if (strcmp(argv[1], "nmp") == 0) {
        return cmd_nmp(argc, argv);
    }

    if (strcmp(argv[1], "smp") == 0) {
        return cmd_smp(argc, argv);
    }

    print_usage();
    return 1;
}
