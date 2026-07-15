/*
 * vm_layout.c
 *
 * 实验目的：
 *   观察大块 malloc/free 对 Linux 进程虚拟地址空间的影响。
 *
 * 观察手段：
 *   1. /proc/<pid>/status：看 VmSize、VmRSS、VmData 等总体统计。
 *   2. /proc/<pid>/maps：看每一段 VMA 的起止地址、权限和来源。
 *
 * 核心思路：
 *   每次分配或释放内存前后都保存一份 status 和 maps 快照，
 *   这样程序结束后仍能复盘每一步地址空间如何合并、拆分和复用空洞。
 */

#define _GNU_SOURCE

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 本实验固定申请 6 个 128MB 块。 */
#define BLOCKS 6

/*
 * 用 unsigned long 表示 1MB，避免 1024 * 1024 先按 int 运算。
 * 后续 128 * MB、1024 * MB 都会提升到足够大的无符号整数类型。
 */
#define MB (1024UL * 1024UL)

/* 给保存的快照编号，方便按时间顺序查看 snapshots 目录。 */
static int snapshot_id = 0;

/*
 * 把 /proc/<pid>/status 或 /proc/<pid>/maps 复制到 snapshots 目录。
 *
 * 参数 proc_name：
 *   "status" 或 "maps"。
 *
 * 参数 label：
 *   当前实验阶段名称，例如 after_alloc_6。
 *
 * 为什么要复制：
 *   /proc/<pid>/maps 只能在进程存活时查看；程序退出后 /proc/<pid> 会消失。
 *   因此必须在程序运行过程中把每个阶段的 maps 保存成普通文件。
 */
static void copy_proc_file(const char *proc_name, const char *label) {
    char src[128];
    char dst[256];
    snprintf(src, sizeof(src), "/proc/%d/%s", getpid(), proc_name);
    snprintf(dst, sizeof(dst), "snapshots/%02d_%s.%s", snapshot_id, label, proc_name);

    FILE *in = fopen(src, "r");
    if (!in) {
        perror(src);
        exit(1);
    }
    FILE *out = fopen(dst, "w");
    if (!out) {
        perror(dst);
        exit(1);
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
    }
    fclose(in);
    fclose(out);
}

/*
 * 从 /proc/<pid>/status 中筛选和本实验有关的字段。
 *
 * VmSize：进程拥有的总虚拟地址空间。
 * VmRSS ：当前实际驻留在物理内存中的页。
 * VmData：数据区、堆和匿名映射等可读写数据空间。
 * VmStk ：栈空间。
 * VmExe ：程序自身可执行代码映射。
 * VmLib ：动态库代码映射。
 */
static void print_status_summary(void) {
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
            strncmp(line, "VmData:", 7) == 0 ||
            strncmp(line, "VmStk:", 6) == 0 ||
            strncmp(line, "VmExe:", 6) == 0 ||
            strncmp(line, "VmLib:", 6) == 0) {
            fputs(line, stdout);
        }
    }
    fclose(fp);
}

/*
 * 对一次实验状态拍快照：
 *   1. 在屏幕打印关键 status 字段；
 *   2. 保存完整 status；
 *   3. 保存完整 maps。
 *
 * 这样既方便现场观察，也方便写报告时回看原始证据。
 */
static void snapshot(const char *label) {
    ++snapshot_id;
    printf("\n===== snapshot %02d: %s =====\n", snapshot_id, label);
    print_status_summary();
    copy_proc_file("status", label);
    copy_proc_file("maps", label);
}

int main(void) {
    /*
     * p 数组只保存 6 个指针，每个指针只是一个地址。
     * 真正的大块内存来自后面的 malloc(128 * MB)。
     */
    void *p[BLOCKS] = {0};
    const size_t block_size = 128 * MB;
    const size_t big_size = 1024 * MB;
    const size_t small_size = 64 * MB;

    /* 如果 snapshots 已经存在，mkdir 会失败但不影响后续写入已有目录。 */
    mkdir("snapshots", 0755);
    printf("pid=%d\n", getpid());
    snapshot("start");

    /*
     * 第一阶段：连续申请 6 个 128MB 块。
     *
     * 重点观察：
     *   大块 malloc 通常由 glibc 使用匿名 mmap 完成，
     *   因此在 maps 中会看到匿名 rw-p VMA，而不只是 [heap] 变大。
     */
    for (int i = 0; i < BLOCKS; ++i) {
        char before[64];
        char after[64];
        snprintf(before, sizeof(before), "before_alloc_%d", i + 1);
        snprintf(after, sizeof(after), "after_alloc_%d", i + 1);
        snapshot(before);

        p[i] = malloc(block_size);
        if (!p[i]) {
            perror("malloc 128MB");
            return 1;
        }

        /*
         * 只写入第一页。
         *
         * 目的：
         *   让这块内存至少发生一次真实访问，建立最小页级证据。
         *
         * 为什么不写满 128MB：
         *   本实验主要观察虚拟地址布局，不需要把每个 128MB 都兑现为物理页。
         */
        memset(p[i], 0, 4096);
        printf("block[%d] = %p, size = 128MB\n", i + 1, p[i]);
        snapshot(after);
    }

    /*
     * 第二阶段：释放第 2、3、5 块。
     *
     * 实验指导书使用 1-based 编号，所以 free_list 中写 2、3、5；
     * 实际访问 C 数组时需要减 1，转成 0-based 下标。
     */
    int free_list[] = {2, 3, 5};
    for (size_t i = 0; i < sizeof(free_list) / sizeof(free_list[0]); ++i) {
        int idx = free_list[i] - 1;
        char before[64];
        char after[64];
        snprintf(before, sizeof(before), "before_free_%d", idx + 1);
        snprintf(after, sizeof(after), "after_free_%d", idx + 1);
        snapshot(before);
        free(p[idx]);
        p[idx] = NULL;
        printf("freed block[%d]\n", idx + 1);
        snapshot(after);
    }

    /*
     * 第三阶段：再申请 1024MB。
     *
     * 1024MB 大块通常无法塞进前面释放形成的 128MB/256MB 小洞，
     * 所以内核会在 mmap 区域中另找一段足够大的连续虚拟地址。
     */
    snapshot("before_alloc_1024");
    void *big = malloc(big_size);
    if (!big) {
        perror("malloc 1024MB");
        return 1;
    }
    memset(big, 0, 4096);
    printf("big_block = %p, size = 1024MB\n", big);
    snapshot("after_alloc_1024");

    /*
     * 第四阶段：预测并申请 64MB。
     *
     * 64MB 小于释放出来的 128MB 或 256MB 空洞，
     * 因此它有机会复用已有空洞；通过 maps 可以验证实际落点。
     */
    printf("\nPrediction: a new 64MB allocation should reuse one of the holes left by freed 128MB blocks.\n");
    snapshot("before_alloc_64");
    void *small = malloc(small_size);
    if (!small) {
        perror("malloc 64MB");
        return 1;
    }
    memset(small, 0, 4096);
    printf("small_block = %p, size = 64MB\n", small);
    snapshot("after_alloc_64");

    /*
     * 收尾清理。
     *
     * p[1]、p[2]、p[4] 前面已经置为 NULL，free(NULL) 是安全的。
     * malloc_trim(0) 是 glibc 扩展，尝试把可归还的堆空间还给内核，
     * 便于最后再观察一次清理后的状态。
     */
    free(small);
    free(big);
    for (int i = 0; i < BLOCKS; ++i) {
        free(p[i]);
    }
    malloc_trim(0);
    snapshot("after_cleanup");

    return 0;
}
