/* ========================================================= */
/* PHẦN 1: CÁC KHAI BÁO CẤU HÌNH VÀ THƯ VIỆN                 */
/* ========================================================= */

// [QUAN TRỌNG] Dòng này bắt buộc phải ở dòng đầu tiên của file.
// Nó báo cho trình biên dịch bật các tính năng mở rộng của GNU/Linux.
// Nếu thiếu dòng này, trình biên dịch sẽ báo lỗi không tìm thấy hàm 'statx'.
#define _GNU_SOURCE 

#include <stdio.h>      // Thư viện nhập xuất chuẩn (printf, perror...)
#include <stdlib.h>     // Thư viện chuẩn (exit, malloc...)
#include <sys/stat.h>   // Chứa struct stat, hàm stat()
#include <sys/types.h>  // Các kiểu dữ liệu hệ thống (uid_t, gid_t...)
#include <pwd.h>        // Dùng để tra cứu tên User từ UID (getpwuid)
#include <grp.h>        // Dùng để tra cứu tên Group từ GID (getgrgid)
#include <time.h>       // Xử lý và định dạng thời gian (localtime, strftime)
#include <string.h>     // Xử lý chuỗi
#include <errno.h>      // Mã lỗi hệ thống
#include <fcntl.h>      // [MỚI] Cần thiết cho các cờ của statx (ví dụ: AT_FDCWD)
#include "stat_cmd.h"   // Header file của module này

/* ========================================================= */
/* PHẦN 2: HÀM XỬ LÝ CHÍNH (DO_STAT)                         */
/* ========================================================= */

void do_stat(const char *path) {
    /* * 1. LẤY THÔNG TIN CƠ BẢN (Dùng stat truyền thống)
     * struct stat là cấu trúc cũ, tương thích mọi hệ thống Linux/Unix.
     * Nó chứa 90% thông tin ta cần (trừ Birth Time).
     */
    struct stat st;

    // Gọi stat(). Lưu ý: stat() sẽ follow symlink (đi xuyên qua shortcut để lấy info file gốc).
    if (stat(path, &st) == -1) {
        perror(path);       // In lỗi nếu không tìm thấy file hoặc không có quyền
        exit(EXIT_FAILURE);
    }

    /* * 2. TRA CỨU TÊN NGƯỜI DÙNG VÀ NHÓM
     * Hệ thống lưu chủ sở hữu dưới dạng số ID (vd: 1000).
     * Cần tra cứu trong /etc/passwd và /etc/group để lấy tên (vd: "ubuntu").
     */
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);

    /* * 3. XỬ LÝ 3 MỐC THỜI GIAN CHUẨN (Access, Modify, Change)
     */
    char atimebuf[64], mtimebuf[64], ctimebuf[64];
    struct tm *tm;

    // a. Access Time (atime): Lần cuối file được đọc/mở
    tm = localtime(&st.st_atime);
    strftime(atimebuf, sizeof(atimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    // b. Modify Time (mtime): Lần cuối nội dung file bị thay đổi
    tm = localtime(&st.st_mtime);
    strftime(mtimebuf, sizeof(mtimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    // c. Change Time (ctime): Lần cuối metadata (quyền, tên, owner) bị thay đổi
    tm = localtime(&st.st_ctime);
    strftime(ctimebuf, sizeof(ctimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm);

    /* * 4. XỬ LÝ BIRTH TIME (Ngày tạo file) - DÙNG STATX
     * Đây là kỹ thuật mới (Linux Kernel > 4.11).
     * struct stat cũ không có chỗ chứa Birth Time, nên phải dùng struct statx.
     */
    char btimebuf[64] = "-"; // Mặc định là gạch ngang nếu hệ thống không hỗ trợ
    struct statx stx;        // Cấu trúc dữ liệu "xịn" hơn stat thường

    // Gọi statx:
    // - AT_FDCWD: Tìm file dựa trên thư mục hiện tại (nếu path là tương đối)
    // - AT_STATX_SYNC_AS_STAT: Đồng bộ dữ liệu mới nhất (không dùng cache cũ)
    // - STATX_BTIME: Cái "Mặt nạ" (Mask) yêu cầu hệ thống lấy Birth Time
    if (statx(AT_FDCWD, path, AT_STATX_SYNC_AS_STAT, STATX_BTIME, &stx) == 0) {
        
        // Kiểm tra lại xem Filesystem có thực sự trả về BTIME hay không
        // (Vì một số ổ cứng định dạng cũ như ext3 sẽ không lưu ngày tạo)
        if (stx.stx_mask & STATX_BTIME) {
            // stx.stx_btime.tv_sec là số giây (time_t) từ năm 1970
            time_t b_sec = stx.stx_btime.tv_sec;
            struct tm *tm_b = localtime(&b_sec);
            strftime(btimebuf, sizeof(btimebuf), "%Y-%m-%d %H:%M:%S.000000000 %z", tm_b);
        }
    }

    /* * 5. HIỂN THỊ THÔNG TIN (FORMAT GIỐNG LỆNH STAT CỦA UBUNTU)
     */
    
    // Dòng 1: Tên file
    printf("  File: %s\n", path);

    // Dòng 2: Size và Blocks
    // - Size: Kích thước nội dung thực (bytes).
    // - Blocks: Số khối 512-byte chiếm dụng trên đĩa (thường lớn hơn Size).
    // - IO Block: Kích thước tối ưu khi đọc/ghi (thường là 4096 bytes).
    printf("  Size: %-10lld\tBlocks: %-10lld IO Block: %-6ld %s file\n",
           (long long)st.st_size, 
           (long long)st.st_blocks, 
           (long)st.st_blksize,
           S_ISDIR(st.st_mode) ? "directory" : "regular"); // Kiểm tra nhanh loại file

    // Dòng 3: Device và Inode
    // - st_dev: ID của thiết bị (ổ cứng) chứa file.
    // - st_ino: Số Inode (chứng minh thư nhân dân của file).
    // - st_nlink: Số lượng Hard Link trỏ tới file này.
    printf("Device: %lxh/%ldd\tInode: %-10lu  Links: %lu\n",
           (unsigned long)st.st_dev, (long)st.st_dev, 
           (unsigned long)st.st_ino, (unsigned long)st.st_nlink);

    // Dòng 4: Quyền hạn (Access)
    // st_mode chứa gộp cả (Loại file) và (Quyền hạn).
    // Phép (st.st_mode & 07777) dùng để lọc bỏ thông tin loại file, chỉ lấy quyền hạn rwx.
    printf("Access: (%04o/", st.st_mode & 07777); // In dạng số bát phân (vd: 0644)

    // In ký tự loại file (d: thư mục, l: link, -: file...)
    if (S_ISDIR(st.st_mode)) printf("d");
    else if (S_ISLNK(st.st_mode)) printf("l");
    else if (S_ISCHR(st.st_mode)) printf("c");
    else if (S_ISBLK(st.st_mode)) printf("b");
    else if (S_ISFIFO(st.st_mode)) printf("p");
    else if (S_ISSOCK(st.st_mode)) printf("s");
    else printf("-");

    // In chuỗi rwx (Read-Write-Execute)
    // Kiểm tra từng bit và in ký tự tương ứng
    printf("%c%c%c%c%c%c%c%c%c)  Uid: ( %5u/%8s)   Gid: ( %5u/%8s)\n",
           (st.st_mode & S_IRUSR) ? 'r' : '-', // User Read
           (st.st_mode & S_IWUSR) ? 'w' : '-', // User Write
           (st.st_mode & S_IXUSR) ? 'x' : '-', // User Execute
           (st.st_mode & S_IRGRP) ? 'r' : '-', // Group Read
           (st.st_mode & S_IWGRP) ? 'w' : '-', // Group Write
           (st.st_mode & S_IXGRP) ? 'x' : '-', // Group Execute
           (st.st_mode & S_IROTH) ? 'r' : '-', // Others Read
           (st.st_mode & S_IWOTH) ? 'w' : '-', // Others Write
           (st.st_mode & S_IXOTH) ? 'x' : '-', // Others Execute
           st.st_uid, pw ? pw->pw_name : "unknown", // UID và tên User
           st.st_gid, gr ? gr->gr_name : "unknown"  // GID và tên Group
    );

    // Dòng 5, 6, 7, 8: Các mốc thời gian
    printf("Access: %s\n", atimebuf);
    printf("Modify: %s\n", mtimebuf);
    printf("Change: %s\n", ctimebuf);
    printf(" Birth: %s\n", btimebuf); // In ra Birth Time đã xử lý ở trên
}