#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ls_l.h"
#include "stat_cmd.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage:\n"
            "  %s stat <path>\n"
            "  %s ls -l [-i] [-h] [-a] <path>\n",
            argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    /* stat */
    if (strcmp(argv[1], "stat") == 0 && argc == 3) {
        do_stat(argv[2]);
        return 0;
    }

    /* ls -l */
    if (strcmp(argv[1], "ls") == 0 && strcmp(argv[2], "-l") == 0) {
        int show_inode = 0, human = 0, show_all = 0;

        int i;
        for (i = 3; i < argc - 1; i++) {
            if (strcmp(argv[i], "-i") == 0) show_inode = 1;
            else if (strcmp(argv[i], "-h") == 0) human = 1;
            else if (strcmp(argv[i], "-a") == 0) show_all = 1;
            else {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        }

        const char *path = argv[argc - 1];
        do_ls_l(path, show_inode, human, show_all);
        return 0;
    }

    fprintf(stderr, "Invalid command\n");
    exit(EXIT_FAILURE);
}
