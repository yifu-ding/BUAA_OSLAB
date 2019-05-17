/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurred during read the IDE disk, panic. 
// 	
// Hint: use syscalls to access device registers and buffers

void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
    // 0x200: the size of a sector: 512 bytes.
    int offset_begin = secno * 0x200;
    int offset_end = offset_begin + nsecs * 0x200;
    int offset = 0;

    int r = 0;
    int offset_disk = 0;
    const int read_disk = 0;
    // Select the IDE ID
    syscall_write_dev(&diskno, 0x13000010, 4);
    while (offset_begin + offset < offset_end) {
        // Your code here
        // offset
        offset_disk = offset_begin + offset;
        syscall_write_dev(&offset_disk, 0x13000000, 4);

        // Start a read or write operation. 
        // (Writing 0 means a Read oper- ation, a 1 means a Write operation.)
        // 告诉他是要读还是要写
        syscall_write_dev(&read_disk, 0x13000020, 4);

        // Get status of the last operation. 
        // (Status 0 means failure, non-zero means success.)
        // 获取一个返回值，0表示失败，1表示成功
        syscall_read_dev(&r, 0x13000030, 1);

        if (r != 0) {
            syscall_read_dev(dst + offset, 0x13004000, 0x200);
            offset += 0x200;
        } else {
            user_panic("ide_read error!");
        }
    }

}

// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurred during read the IDE disk, panic.
//	
// Hint: use syscalls to access device registers and buffers

void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
        // Your code here
        int offset_begin = secno * 0x200;
        int offset_end = offset_begin + nsecs * 0x200;;
        int offset = 0;
        writef("diskno: %d\n", diskno);
        while (offset_begin + offset < offset_end) {
            // copy data from source array to disk buffer.
            syscall_write_dev(&diskno, 0x13000010, 4);

            int va = offset_begin + offset;
            syscall_write_dev(&va, 0x13000000, 4);

            syscall_write_dev(src + offset, 0x13004000, 0x200);
            /*Start a read or write operation. 
            (Writing 0 means a Read oper- ation, a 1 means a Write operation.)*/
            va = 1;
            syscall_write_dev(&va, 0x13000020, 1);
            syscall_read_dev(&va, 0x13000030, 1);
            if (va != 0) {
                offset += 0x200;
            } else {
                user_panic("ide_write error!");
            }

            // if error occur, then panic.
    }
}

