/*
 * max_vmem.c
 *
 * 实验目的：
 *   测试单个进程一次能够建立的最大连续匿名虚拟映射。
 *
 * 注意：
 *   这里测的是“虚拟地址空间能否保留”，不是物理内存有多大。
 *   因此程序使用 PROT_NONE 和 MAP_NORESERVE，避免真正读写页面。
 */

#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

/* 使用 unsigned long long，保证 TiB 级别的字节数不会按 int 溢出。 */
#define GiB (1024ULL * 1024ULL * 1024ULL)

/*
 * 尝试建立一段 bytes 字节的匿名虚拟映射。
 *
 * 返回值：
 *   1：mmap 成功；
 *   0：mmap 失败。
 *
 * 关键参数解释：
 *   PROT_NONE：
 *     这段地址暂时不可读、不可写、不可执行。也就是说只占地址范围，
 *     不进行实际访问。
 *
 *   MAP_PRIVATE | MAP_ANONYMOUS：
 *     建立私有匿名映射，不对应任何文件，类似实验中大块虚拟内存。
 *
 *   MAP_NORESERVE：
 *     不要求内核提前为这段虚拟空间预留 swap/物理承诺。
 *     否则测试结果容易被物理内存或 swap 大小提前限制。
 *
 * 为什么成功后立刻 munmap：
 *   每次只是在“试探”一个大小是否能映射，不能让前一次映射占住地址空间，
 *   否则后续试探会被自己干扰。
 */
static int try_map(unsigned long long bytes) {
    void *p = mmap(NULL, (size_t)bytes, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        return 0;
    }
    munmap(p, (size_t)bytes);
    return 1;
}

/*
 * 打印 RLIMIT_AS。
 *
 * RLIMIT_AS 是进程虚拟地址空间的资源限制。
 * 如果它不是 unlimited，最大可映射空间会先受到这个限制影响。
 */
static void print_rlimit(void) {
    struct rlimit lim;
    if (getrlimit(RLIMIT_AS, &lim) == 0) {
        if (lim.rlim_cur == RLIM_INFINITY) {
            printf("RLIMIT_AS: unlimited\n");
        } else {
            printf("RLIMIT_AS: %.2f GiB\n", (double)lim.rlim_cur / GiB);
        }
    }
}

int main(void) {
    print_rlimit();

    /*
     * low  表示当前已经确认能成功映射的最大值；
     * high 表示下一次要尝试的候选值，初始从 1GiB 开始。
     *
     * cap 是保护上限，避免在异常系统上无限左移。
     */
    unsigned long long low = 0;
    unsigned long long high = 1ULL * GiB;
    const unsigned long long cap = 512ULL * 1024ULL * GiB;

    /*
     * 第一阶段：指数扩张。
     *
     * 每次成功就把 high 翻倍，快速找到“成功范围”和“失败上界”。
     * 这比 1GiB、2GiB、3GiB 线性扫描快得多。
     */
    while (high < cap && try_map(high)) {
        low = high;
        high <<= 1;
        printf("single mmap %.2f GiB: success\n", (double)low / GiB);
    }
    printf("first failed upper bound: %.2f GiB\n", (double)high / GiB);

    /*
     * 第二阶段：二分搜索。
     *
     * mmap 的长度最终应按页大小对齐，所以 mid 要向下取整到页边界。
     * 搜索停止条件是 high 和 low 的差距小于等于一个页面。
     */
    const unsigned long long page = (unsigned long long)sysconf(_SC_PAGESIZE);
    while (high - low > page) {
        unsigned long long mid = low + (high - low) / 2;
        mid -= mid % page;

        /* 防止页对齐后 mid 没有前进，造成死循环。 */
        if (mid == low) {
            break;
        }
        if (try_map(mid)) {
            low = mid;
        } else {
            high = mid;
        }
    }

    printf("max contiguous anonymous virtual mapping: %.2f GiB (%.2f TiB)\n",
           (double)low / GiB, (double)low / (1024.0 * GiB));
    printf("errno after final search step may be stale; result is based on mmap success/failure.\n");
    return 0;
}
