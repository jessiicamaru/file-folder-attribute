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
#include "ls_l.h"

static void display_permissions(mode_t mode) {
    // Loại file
    printf((S_ISDIR(mode)) ? "d" : 
           (S_ISLNK(mode)) ? "l" : 
           (S_ISCHR(mode)) ? "c" : 
           (S_ISBLK(mode)) ? "b" : 
           (S_ISFIFO(mode)) ? "p" : 
           (S_ISSOCK(mode)) ? "s" : "-");

    // Owner permissions
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    // Group permissions
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    // Others permissions
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

static void display_file_info(const char *filename, struct stat *st) {
    struct passwd *pw = getpwuid(st->st_uid);
    struct group *gr = getgrgid(st->st_gid);
    char timebuf[80];
    struct tm *tm = localtime(&st->st_mtime);

    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);  // Định dạng như ls: Tháng Ngày Giờ:Phút

    display_permissions(st->st_mode);
    printf(" %2lu ", (unsigned long)st->st_nlink);  // Số links
    printf("%-8s ", pw ? pw->pw_name : "unknown");  // Owner
    printf("%-8s ", gr ? gr->gr_name : "unknown");  // Group
    printf("%8lld ", (long long)st->st_size);       // Size
    printf("%s ", timebuf);                         // Thời gian modify
    printf("%s\n", filename);                       // Tên file
}

void do_ls_l(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(st.st_mode)) {
        // Nếu là thư mục, liệt kê nội dung
        DIR *dir = opendir(path);
        if (!dir) {
            perror(path);
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;  // Bỏ . và .. (như ls mặc định, không -a)
            }

            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            if (lstat(fullpath, &st) == -1) {
                perror(fullpath);
                continue;
            }

            display_file_info(entry->d_name, &st);
        }

        closedir(dir);
    } else {
        // Nếu là file, hiển thị info
        display_file_info(path, &st);
    }
}