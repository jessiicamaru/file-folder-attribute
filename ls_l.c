/* ========================================================= */
/* PHẦN 1: KHAI BÁO THƯ VIỆN (HEADER FILES)                  */
/* ========================================================= */
#include <stdio.h>      // Thư viện chuẩn vào/ra (printf, snprintf, perror...)
#include <stdlib.h>     // Thư viện chuẩn (exit, malloc...)
#include <sys/stat.h>   // QUAN TRỌNG: Chứa struct stat và các macro kiểm tra file (S_ISDIR...)
#include <sys/types.h>  // Định nghĩa các kiểu dữ liệu hệ thống (off_t, mode_t...)
#include <pwd.h>        // Dùng để lấy thông tin User từ UID (getpwuid)
#include <grp.h>        // Dùng để lấy thông tin Group từ GID (getgrgid)
#include <time.h>       // Xử lý thời gian (localtime, strftime)
#include <dirent.h>     // Xử lý thư mục (opendir, readdir, closedir)
#include <string.h>     // Xử lý chuỗi (strcmp, strlen...)
#include <errno.h>      // Xử lý mã lỗi hệ thống
#include <unistd.h>     // Các system call cơ bản (readlink, lstat...)
#include <limits.h>     // Chứa hằng số giới hạn hệ thống (ví dụ: PATH_MAX - độ dài đường dẫn tối đa)
#include "ls_l.h"       // Header file tự định nghĩa của project này

/* ========================================================= */
/* PHẦN 2: CÁC HÀM HELPER (HÀM PHỤ TRỢ)                      */
/* ========================================================= */

/* Hàm chuyển đổi kích thước byte sang dạng đọc được (K, M, G) */
static void print_human_size(off_t size) {
    if (size < 1024)
        // Nếu nhỏ hơn 1KB, in số byte bình thường
        printf("%8ld ", (long)size);
    else if (size < 1024 * 1024)
        // Nếu nhỏ hơn 1MB, chia cho 1024 để ra KB. %.1f là in 1 số lẻ thập phân.
        printf("%7.1fK ", size / 1024.0);
    else if (size < 1024LL * 1024 * 1024)
        // Nếu nhỏ hơn 1GB. Lưu ý 1024LL (Long Long) để tránh tràn số khi nhân.
        printf("%7.1fM ", size / (1024.0 * 1024));
    else
        // Nếu lớn hơn hoặc bằng 1GB
        printf("%7.1fG ", size / (1024.0 * 1024 * 1024));
}

/* Hàm hiển thị quyền hạn (vd: drwxr-xr-x) */
static void display_permissions(mode_t mode) {
    // Ký tự đầu tiên xác định loại file
    printf(S_ISDIR(mode) ? "d" :   // Directory (Thư mục)
           S_ISLNK(mode) ? "l" :   // Symbolic Link (Liên kết mềm)
           S_ISCHR(mode) ? "c" :   // Character device (Thiết bị ký tự, vd: bàn phím)
           S_ISBLK(mode) ? "b" :   // Block device (Thiết bị khối, vd: ổ cứng)
           S_ISFIFO(mode)? "p" :   // FIFO/Pipe (Đường ống)
           S_ISSOCK(mode)? "s" :   // Socket (Kết nối mạng/IPC)
           "-");                   // Regular file (File thường)

    // 3 bit tiếp theo: Quyền của chủ sở hữu (User)
    // Dùng phép AND bit (&) để kiểm tra bit tương ứng có bật không
    printf((mode & S_IRUSR) ? "r" : "-"); // Read
    printf((mode & S_IWUSR) ? "w" : "-"); // Write
    printf((mode & S_IXUSR) ? "x" : "-"); // Execute

    // 3 bit tiếp theo: Quyền của nhóm (Group)
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    // 3 bit cuối: Quyền của người khác (Others)
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

/* Hàm hiển thị toàn bộ thông tin của 1 file */
static void display_file_info(
    const char *name,       // Tên hiển thị (chỉ tên file)
    const char *fullpath,   // Đường dẫn đầy đủ (để dùng cho readlink nếu cần)
    struct stat *st,        // Con trỏ chứa thông tin file đã lấy được từ lstat
    int show_inode,         // Cờ: có hiện inode không?
    int human               // Cờ: có hiện size dạng human-readable không?
) {
    // Lấy thông tin User từ User ID (st_uid)
    struct passwd *pw = getpwuid(st->st_uid);
    // Lấy thông tin Group từ Group ID (st_gid)
    struct group  *gr = getgrgid(st->st_gid);

    // Xử lý thời gian sửa đổi lần cuối (mtime)
    char timebuf[64];
    // Chuyển timestamp (giây) sang struct tm (ngày tháng năm giờ phút)
    struct tm *tm = localtime(&st->st_mtime);
    // Format thành chuỗi dạng "Tháng Ngày Giờ:Phút" (vd: Dec 23 10:00)
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);

    // Nếu cờ -i được bật, in số Inode
    if (show_inode)
        printf("%8lu ", (unsigned long)st->st_ino);

    // Gọi hàm in quyền hạn đã viết ở trên
    display_permissions(st->st_mode);

    // In số lượng Hard Link (liên kết cứng)
    printf(" %2lu ", (unsigned long)st->st_nlink);

    // In tên User và Group. Nếu không tìm thấy tên (NULL), in "unknown"
    printf("%-8s %-8s ",
        pw ? pw->pw_name : "unknown",
        gr ? gr->gr_name : "unknown");

    // In kích thước file
    if (human)
        print_human_size(st->st_size); // Dùng hàm helper nếu có cờ -h
    else
        printf("%8lld ", (long long)st->st_size); // In byte thuần

    // In thời gian đã format
    printf("%s ", timebuf);

    /* Xử lý đặc biệt cho Symbolic Link */
    if (S_ISLNK(st->st_mode)) {
        char target[PATH_MAX]; // Buffer chứa đường dẫn đích
        // readlink đọc nội dung của symlink (đường dẫn mà nó trỏ tới)
        ssize_t len = readlink(fullpath, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0'; // Kết thúc chuỗi thủ công vì readlink không tự thêm \0
            printf("%s -> %s\n", name, target); // In dạng: link -> file_goc
            return; // Xong rồi thì thoát hàm
        }
    }

    // Nếu không phải symlink hoặc lỗi readlink, chỉ in tên file
    printf("%s\n", name);
}

/* ========================================================= */
/* PHẦN 3: LOGIC CHÍNH CỦA ls -l                             */
/* ========================================================= */

void do_ls_l(const char *path, int show_inode, int human, int show_all) {
    struct stat st;

    // Gọi lstat để lấy thông tin file tại đường dẫn 'path'.
    // Dùng lstat (thay vì stat) để nếu path là symlink, ta lấy info của chính symlink đó.
    if (lstat(path, &st) == -1) {
        perror(path); // Nếu lỗi (vd: file không tồn tại), in lỗi ra stderr
        exit(EXIT_FAILURE);
    }

    // TRƯỜNG HỢP 1: path KHÔNG phải thư mục (là file thường, link, v.v.)
    if (!S_ISDIR(st.st_mode)) {
        // Chỉ việc in thông tin của nó và kết thúc
        display_file_info(path, path, &st, show_inode, human);
        return;
    }

    // TRƯỜNG HỢP 2: path LÀ thư mục
    // Mở thư mục để đọc nội dung bên trong
    DIR *dir = opendir(path);
    if (!dir) {
        perror(path); // Lỗi (vd: không có quyền đọc thư mục)
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;    // Con trỏ chứa thông tin từng file khi duyệt
    char fullpath[PATH_MAX]; // Buffer để ghép đường dẫn đầy đủ

    // Vòng lặp đọc từng file trong thư mục cho đến khi hết (NULL)
    while ((entry = readdir(dir)) != NULL) {
        // Nếu không có cờ -a (show_all), bỏ qua các file ẩn (bắt đầu bằng dấu chấm)
        // Đặc biệt là "." (thư mục hiện tại) và ".." (thư mục cha)
        if (!show_all && entry->d_name[0] == '.')
            continue;

        // Tạo đường dẫn đầy đủ: path + "/" + tên_file
        // Vd: path="home", file="test" -> fullpath="home/test"
        // Cần làm vậy vì entry->d_name chỉ là tên file, lstat cần đường dẫn cụ thể
        snprintf(fullpath, sizeof(fullpath),
                 "%s/%s", path, entry->d_name);

        // Lấy thông tin của file con vừa tìm thấy
        if (lstat(fullpath, &st) == -1) {
            perror(fullpath); // Nếu lỗi thì báo lỗi và bỏ qua file này, tiếp tục file khác
            continue;
        }

        // Hiển thị thông tin file con
        display_file_info(entry->d_name, fullpath,
                          &st, show_inode, human);
    }

    // Đóng thư mục sau khi duyệt xong để giải phóng tài nguyên
    closedir(dir);
}