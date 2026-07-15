/*
 * locality_compare.c
 *
 * 实验目的：
 *   在访问总量相同的前提下，只改变访问顺序，
 *   对比“全空间循环访问”和“16 页分组访问”的性能差异。
 *
 * 核心思想：
 *   global  模式：每轮扫完整个 4096MB，再进入下一轮。
 *   group16 模式：每次只围绕 16 个连续页重复访问，再进入下一组。
 *
 * 如果物理内存压力较大，global 的短期工作集太大，更容易反复换页；
 * group16 的短期工作集只有 16 * 4KB = 64KB，局部性更好。
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

/* 使用 unsigned long，避免 MB 乘法在大输入时按 int 运算。 */
#define MB (1024UL * 1024UL)

/* 单调时钟计时，适合测量程序内部耗时。 */
static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/*
 * 输出缺页和最大 RSS。
 *
 * minor faults：次缺页，通常不需要磁盘 I/O。
 * major faults：主缺页，可能需要从 swap/磁盘换入，代价更高。
 * max RSS     ：进程运行期间达到过的最大物理驻留内存。
 */
static void print_usage_delta(struct rusage *before)
{
    struct rusage after;
    getrusage(RUSAGE_SELF, &after);
    printf("minor faults: %ld\n", after.ru_minflt - before->ru_minflt);
    printf("major faults: %ld\n", after.ru_majflt - before->ru_majflt);
    printf("max RSS: %ld KB\n", after.ru_maxrss);
}

/*
 * global 模式：
 *   repeat 放在最外层，每一轮都从第 0 页扫到最后一页。
 *
 * 直观理解：
 *   第 1 页被再次访问前，要等整个 4096MB 空间都扫完。
 *   如果内存不够，前面访问过的页很可能已经被换出。
 */
static void access_global(unsigned char *buf, size_t pages, size_t page_size, int repeat)
{
    for (int r = 0; r < repeat; ++r)
    {
        for (size_t page = 0; page < pages; ++page)
        {
            buf[page * page_size]++;
        }
    }
}

/*
 * group16 模式：
 *   每次选取 16 个连续页作为一个小组，在这个小组内完成 repeat 次访问，
 *   然后再移动到下一组。
 *
 * 直观理解：
 *   短时间内反复访问的页面只有 16 页，即约 64KB。
 *   这些页更可能仍然驻留在物理内存中，所以主缺页次数会明显减少。
 */
static void access_group16(unsigned char *buf, size_t pages, size_t page_size, int repeat)
{
    const size_t group = 16;
    for (size_t base = 0; base < pages; base += group)
    {
        size_t end = base + group;

        /* 最后一组可能不足 16 页，需要防止越界。 */
        if (end > pages)
        {
            end = pages;
        }
        for (int r = 0; r < repeat; ++r)
        {
            for (size_t page = base; page < end; ++page)
            {
                buf[page * page_size]++;
            }
        }
    }
}

int main(int argc, char **argv)
{
    /*
     * 参数说明：
     *   <MB>     ：申请的匿名空间大小；
     *   <repeat> ：每个页面总共重复写几次；
     *   global   ：全空间逐页扫描；
     *   group16  ：16 页分组访问。
     */
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <MB> <repeat> <global|group16>\n", argv[0]);
        return 1;
    }

    size_t mb = strtoull(argv[1], NULL, 10);
    int repeat = atoi(argv[2]);
    const char *mode = argv[3];
    size_t len = mb * MB;
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);

    /*
     * pages 是总页数。后续访问使用 buf[page * page_size]，
     * 保证每次写入的是对应页的第一个字节。
     */
    size_t pages = len / page_size;

    printf("pid=%d, allocation=%zu MB, pages=%zu, repeat=%d, mode=%s\n",
           getpid(), mb, pages, repeat, mode);

    /*
     * 建立一段大匿名映射作为待访问空间。
     * 只有后续写入页面时，内核才会逐步分配实际物理页。
     */
    unsigned char *buf = mmap(NULL, len, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    struct rusage before;
    getrusage(RUSAGE_SELF, &before);
    double t0 = now_seconds();

    /*
     * 两个分支的访问总量相同：
     *   pages * repeat 次页面写入。
     *
     * 差别只在访问顺序，因此更能说明局部性对缺页和耗时的影响。
     */
    if (strcmp(mode, "global") == 0)
    {
        access_global(buf, pages, page_size, repeat);
    }
    else if (strcmp(mode, "group16") == 0)
    {
        access_group16(buf, pages, page_size, repeat);
    }
    else
    {
        fprintf(stderr, "unknown mode: %s\n", mode);
        munmap(buf, len);
        return 1;
    }
    double t1 = now_seconds();

    printf("elapsed seconds: %.3f\n", t1 - t0);
    print_usage_delta(&before);

    /* 释放映射，避免程序结束前继续占用虚拟地址空间。 */
    munmap(buf, len);
    return 0;
}
