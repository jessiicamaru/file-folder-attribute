#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ls_l.h"
#include "stat_cmd.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [ls -l | stat] <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "ls") == 0 && argc == 4 && strcmp(argv[2], "-l") == 0) {
        // Chạy ls -l <path>
        do_ls_l(argv[3]);
    } else if (strcmp(argv[1], "stat") == 0 && argc == 3) {
        // Chạy stat <path>
        do_stat(argv[2]);
    } else {
        fprintf(stderr, "Invalid command. Use 'ls -l <path>' or 'stat <path>'\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}