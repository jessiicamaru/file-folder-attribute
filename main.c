#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ls_l.h"       // Import hàm do_ls_l
#include "stat_cmd.h"   // Import hàm do_stat

/* * argc (Argument Count): Số lượng tham số (bao gồm cả tên chương trình)
 * argv (Argument Vector): Mảng các chuỗi chứa tham số
 * Ví dụ gõ: ./app ls -l -h /home
 * -> argc = 5
 * -> argv = ["./app", "ls", "-l", "-h", "/home"]
 */
int main(int argc, char *argv[]) {
    
    /* === 1. Kiểm tra số lượng tham số tối thiểu === */
    // Cần ít nhất 3 tham số: [Tên_CT] [Lệnh] [Đường_dẫn]
    // Ví dụ tối thiểu: ./app stat myfile
    if (argc < 3) {
        fprintf(stderr,
            "Usage:\n"
            "  %s stat <path>\n"
            "  %s ls -l [-i] [-h] [-a] <path>\n",
            argv[0], argv[0]); // argv[0] là tên chương trình
        exit(EXIT_FAILURE);
    }

    /* === 2. Xử lý lệnh "stat" === */
    // Logic: Nếu tham số thứ 2 là "stat" VÀ tổng tham số đúng bằng 3
    if (strcmp(argv[1], "stat") == 0 && argc == 3) {
        do_stat(argv[2]); // Gọi hàm stat với đường dẫn là argv[2]
        return 0;         // Kết thúc chương trình thành công
    }

    /* === 3. Xử lý lệnh "ls -l" === */
    // Logic: Bắt buộc tham số thứ 2 là "ls" VÀ tham số thứ 3 là "-l"
    if (strcmp(argv[1], "ls") == 0 && strcmp(argv[2], "-l") == 0) {
        // Khởi tạo các cờ (flag) mặc định là tắt (0)
        int show_inode = 0, human = 0, show_all = 0;

        // Vòng lặp phân tích các tùy chọn (Option Parsing)
        // Chạy từ tham số thứ 4 (index 3) đến áp chót (argc - 2)
        // Tại sao lại là argc - 1? Vì tham số CUỐI CÙNG luôn được coi là đường dẫn (path)
        int i;
        for (i = 3; i < argc - 1; i++) {
            if (strcmp(argv[i], "-i") == 0) show_inode = 1;      // Bật hiện inode
            else if (strcmp(argv[i], "-h") == 0) human = 1;      // Bật hiện size dễ đọc
            else if (strcmp(argv[i], "-a") == 0) show_all = 1;   // Bật hiện file ẩn
            else {
                // Nếu gặp tham số lạ (không phải path, không phải cờ đã quy định)
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        }

        // Tham số cuối cùng của mảng luôn là đường dẫn cần liệt kê
        const char *path = argv[argc - 1];
        
        // Gọi hàm xử lý chính bên ls_l.c
        do_ls_l(path, show_inode, human, show_all);
        return 0;
    }

    /* === 4. Trường hợp không khớp lệnh nào === */
    fprintf(stderr, "Invalid command\n");
    exit(EXIT_FAILURE);
}


/*
Inode (viết tắt của Index Node) là một khái niệm cốt lõi và 
cực kỳ quan trọng trong hệ điều hành Linux/Unix.
- Dữ liệu file (Content): Là nội dung viết trong cuốn sách.
- Tên file (Filename): Là dòng chữ ghi trên gáy sách hoặc trong danh mục tìm kiếm.
- Inode: Chính là "Thẻ thư viện" hoặc "Hồ sơ quản lý" của cuốn sách đó.
*/