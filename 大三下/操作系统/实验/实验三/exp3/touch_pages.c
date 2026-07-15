/*
 * touch_pages.c
 *
 * 实验目的：
 *   对比“只建立虚拟映射”“读触页”“写触页”三者对物理内存的影响。
 *
 * 典型运行：
 *   ./touch_pages 3072 read
 *   ./touch_pages 3072 write
 *   ./touch_pages 1792 write-hold 180
 *
 * 核心结论：
 *   mmap 成功只代表虚拟地址空间建立；
 *   读匿名页通常映射共享零页，RssAnon 几乎不涨；
 *   写匿名页必须分配私有物理页，RssAnon 会明显上升，内存紧张时会产生 Swap。
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* 用 unsigned long 表示 1MB，避免大规模 MB 计算时按 int 溢出。 */
#define MB (1024UL * 1024UL)

/*
 * 打印进程内存状态摘要。
 *
 * 本程序重点关注：
 *   VmSize ：虚拟地址空间总量，mmap 后会立即变大。
 *   VmRSS  ：当前驻留物理内存的总量。
 *   RssAnon：匿名页中仍驻留物理内存的部分，判断写触页是否分配了私有页。
 *   RssFile：文件映射页驻留量，用于和匿名页区分。
 *   VmSwap ：匿名页被换出到 swap 的大小。
 */
static void print_status_summary(const char *label) {
    printf("\n===== %s =====\n", label);
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/status", getpid());
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        exit(1);
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "RssAnon:", 8) == 0 ||
            strncmp(line, "RssFile:", 8) == 0 ||
            strncmp(line, "VmSwap:", 7) == 0) {
            fputs(line, stdout);
        }
    }
    fclose(fp);
}

/* 返回单调递增时间，用于统计触页循环耗时，不受系统时间校准影响。 */
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/*
 * 打印触页前后的缺页次数差值。
 *
 * ru_minflt：minor fault，页不在当前页表中，但不需要从磁盘读入。
 * ru_majflt：major fault，需要磁盘 I/O，常见于从 swap 换回页面。
 */
static void print_usage_delta(const char *label, struct rusage *before) {
    struct rusage after;
    getrusage(RUSAGE_SELF, &after);
    printf("%s page faults: minor +%ld, major +%ld\n", label,
           after.ru_minflt - before->ru_minflt,
           after.ru_majflt - before->ru_majflt);
}

int main(int argc, char **argv) {
    /*
     * 参数说明：
     *   <MB>              申请多少 MB 匿名空间；
     *   read              按页读取；
     *   write             按页写入；
     *   write-hold 秒数   写入后保持进程存活，方便另一个进程制造内存竞争。
     */
    if (argc < 3) {
        fprintf(stderr, "usage: %s <MB> <read|write|write-hold> [hold_seconds]\n", argv[0]);
        return 1;
    }

    size_t mb = strtoull(argv[1], NULL, 10);
    const char *mode = argv[2];
    int hold_seconds = argc >= 4 ? atoi(argv[3]) : 0;
    size_t len = mb * MB;

    /*
     * 获取系统页大小。本虚拟机为 4096 字节。
     * 后续循环每次 off += page，保证每次访问都落到一个新页面上。
     */
    size_t page = (size_t)sysconf(_SC_PAGESIZE);

    /*
     * volatile 防止编译器把读循环优化掉。
     * 如果只是写 buf[off];，编译器可能认为读取结果没用而删除访问。
     */
    volatile unsigned char sink = 0;

    printf("pid=%d, allocation=%zu MB, page_size=%zu, mode=%s\n", getpid(), mb, page, mode);
    print_status_summary("before mmap");

    /*
     * 建立私有匿名映射：
     *   MAP_ANONYMOUS 表示不对应文件；
     *   MAP_PRIVATE 表示每个进程拥有自己的私有写入结果。
     *
     * 这里 mmap 成功后，VmSize 会变大，但尚未逐页访问，
     * 所以 VmRSS/RssAnon 通常不会同步大幅增加。
     */
    unsigned char *buf = mmap(NULL, len, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    printf("mapped address range: %p - %p\n", buf, buf + len);
    print_status_summary("after mmap before touch");

    struct rusage before;
    getrusage(RUSAGE_SELF, &before);
    double t0 = now_seconds();

    /*
     * read 模式：
     *   每页读取一个字节，但不修改页面。
     *   对匿名页而言，Linux 可以把这些页映射到共享只读零页，
     *   因此会产生大量 minor fault，但 RssAnon 几乎不变。
     */
    if (strcmp(mode, "read") == 0) {
        for (size_t off = 0; off < len; off += page) {
            sink ^= buf[off];
        }

    /*
     * write / write-hold 模式：
     *   每页写入一个字节。
     *   写匿名页时，内核不能再让它共享零页，必须分配私有物理页，
     *   因此 RssAnon 会显著上升；如果内存不足，还可能出现 VmSwap。
     */
    } else if (strcmp(mode, "write") == 0 || strcmp(mode, "write-hold") == 0) {
        for (size_t off = 0; off < len; off += page) {
            buf[off] = (unsigned char)(buf[off] + 1);
        }
    } else {
        fprintf(stderr, "unknown mode: %s\n", mode);
        munmap(buf, len);
        return 1;
    }
    double t1 = now_seconds();
    printf("touch finished in %.3f seconds, sink=%u\n", t1 - t0, sink);
    print_usage_delta("touch", &before);
    print_status_summary("after touch");

    /*
     * write-hold 用于双进程竞争实验。
     *
     * A 进程写完后不立刻退出，而是 sleep 保持映射；
     * 另一个终端启动 B 进程写入大内存后，可以观察 A 的 VmRSS 下降、
     * VmSwap 上升，从而证明 A 的匿名页被换出。
     */
    if (strcmp(mode, "write-hold") == 0 && hold_seconds > 0) {
        printf("holding mapping for %d seconds; inspect /proc/%d/status from another process now.\n",
               hold_seconds, getpid());
        sleep((unsigned int)hold_seconds);
        print_status_summary("after hold");
    }

    /* 释放匿名映射后，VmSize 会回落到接近程序初始状态。 */
    munmap(buf, len);
    print_status_summary("after munmap");
    return 0;
}
