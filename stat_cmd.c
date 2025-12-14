#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "stat_cmd.h"

void do_stat(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);

    // Định dạng thời gian giống stat (YYYY-MM-DD HH:MM:SS.000000000 +0000)
    char atimebuf[64], mtimebuf[64], ctimebuf[64];
    struct tm *tm;

    tm = localtime(&st.st_atime);
    strftime(atimebuf, sizeof(atimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    tm = localtime(&st.st_mtime);
    strftime(mtimebuf, sizeof(mtimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    tm = localtime(&st.st_ctime);
    strftime(ctimebuf, sizeof(ctimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    // Output giống stat
    printf("  File: %s\n", path);
    printf("  Size: %-10lld\tBlocks: %-10lld IO Block: %-6ld %s file\n",
           (long long)st.st_size, (long long)st.st_blocks, (long)st.st_blksize,
           S_ISDIR(st.st_mode) ? "directory" : "regular");
    printf("Device: %lxh/%ldd\tInode: %-10lu  Links: %lu\n",
           (unsigned long)st.st_dev, (long)st.st_dev, (unsigned long)st.st_ino, (unsigned long)st.st_nlink);
    printf("Access: (%04o/", st.st_mode & 07777);
    if (S_ISDIR(st.st_mode)) printf("d");
    else if (S_ISLNK(st.st_mode)) printf("l");
    else if (S_ISCHR(st.st_mode)) printf("c");
    else if (S_ISBLK(st.st_mode)) printf("b");
    else if (S_ISFIFO(st.st_mode)) printf("p");
    else if (S_ISSOCK(st.st_mode)) printf("s");
    else printf("-");
    printf("%c%c%c%c%c%c%c%c%c)  Uid: ( %5u/%8s)   Gid: ( %5u/%8s)\n",
           (st.st_mode & S_IRUSR) ? 'r' : '-', (st.st_mode & S_IWUSR) ? 'w' : '-', (st.st_mode & S_IXUSR) ? 'x' : '-',
           (st.st_mode & S_IRGRP) ? 'r' : '-', (st.st_mode & S_IWGRP) ? 'w' : '-', (st.st_mode & S_IXGRP) ? 'x' : '-',
           (st.st_mode & S_IROTH) ? 'r' : '-', (st.st_mode & S_IWOTH) ? 'w' : '-', (st.st_mode & S_IXOTH) ? 'x' : '-',
           st.st_uid, pw ? pw->pw_name : "unknown", st.st_gid, gr ? gr->gr_name : "unknown");
    printf("Access: %s\n", atimebuf);
    printf("Modify: %s\n", mtimebuf);
    printf("Change: %s\n", ctimebuf);
    printf(" Birth: -\n");  // Birth time không hỗ trợ trên tất cả filesystem, bỏ qua
}