#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include "ls_l.h"

/* ===== helper ===== */

static void print_human_size(off_t size) {
    if (size < 1024)
        printf("%8ld ", (long)size);
    else if (size < 1024 * 1024)
        printf("%7.1fK ", size / 1024.0);
    else if (size < 1024LL * 1024 * 1024)
        printf("%7.1fM ", size / (1024.0 * 1024));
    else
        printf("%7.1fG ", size / (1024.0 * 1024 * 1024));
}

static void display_permissions(mode_t mode) {
    printf(S_ISDIR(mode) ? "d" :
           S_ISLNK(mode) ? "l" :
           S_ISCHR(mode) ? "c" :
           S_ISBLK(mode) ? "b" :
           S_ISFIFO(mode)? "p" :
           S_ISSOCK(mode)? "s" : "-");

    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

static void display_file_info(
    const char *name,
    const char *fullpath,
    struct stat *st,
    int show_inode,
    int human
) {
    struct passwd *pw = getpwuid(st->st_uid);
    struct group  *gr = getgrgid(st->st_gid);

    char timebuf[64];
    struct tm *tm = localtime(&st->st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);

    if (show_inode)
        printf("%8lu ", (unsigned long)st->st_ino);

    display_permissions(st->st_mode);
    printf(" %2lu ", (unsigned long)st->st_nlink);
    printf("%-8s %-8s ",
        pw ? pw->pw_name : "unknown",
        gr ? gr->gr_name : "unknown");

    if (human)
        print_human_size(st->st_size);
    else
        printf("%8lld ", (long long)st->st_size);

    printf("%s ", timebuf);

    /* symlink */
    if (S_ISLNK(st->st_mode)) {
        char target[PATH_MAX];
        ssize_t len = readlink(fullpath, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            printf("%s -> %s\n", name, target);
            return;
        }
    }

    printf("%s\n", name);
}

/* ===== main ls -l ===== */

void do_ls_l(const char *path, int show_inode, int human, int show_all) {
    struct stat st;

    if (lstat(path, &st) == -1) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    if (!S_ISDIR(st.st_mode)) {
        display_file_info(path, path, &st, show_inode, human);
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    char fullpath[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (!show_all &&
            (strcmp(entry->d_name, ".") == 0 ||
             strcmp(entry->d_name, "..") == 0))
            continue;

        snprintf(fullpath, sizeof(fullpath),
                 "%s/%s", path, entry->d_name);

        if (lstat(fullpath, &st) == -1) {
            perror(fullpath);
            continue;
        }

        display_file_info(entry->d_name, fullpath,
                          &st, show_inode, human);
    }

    closedir(dir);
}

