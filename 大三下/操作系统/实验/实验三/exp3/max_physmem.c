/*
 * max_physmem.c
 *
 * 实验目的：
 *   测试单个进程最多能“真正占住”多少物理内存。
 *
 * 和 max_vmem.c 的区别：
 *   max_vmem.c 只测试虚拟地址空间能否建立映射，成功 mmap 不代表占用物理内存；
 *   本程序会对每个页面执行写操作，强制 Linux 给该页面分配真实物理页。
 *
 * 推荐运行：
 *   gcc -Wall -Wextra -O2 -g -o max_physmem max_physmem.c
 *   ./max_physmem 64 512 0 0 | tee logs/max_physmem.log
 *
 * 参数说明：
 *   argv[1] chunk_mb     每次申请并写触页的大小，默认 64MB。越小结果越精细，但输出更多。
 *   argv[2] reserve_mb   给系统保留的 MemAvailable 安全线，默认 512MB。
 *   argv[3] max_mb       人为设置最多测试多少 MB；0 表示不设置上限，默认 0。
 *   argv[4] hold_seconds 达到停止条件后保持进程存活多少秒，方便另一个终端查看 /proc/pid/status，默认 0。
 *
 * 安全说明：
 *   物理内存测试必须写触页，确实会给虚拟机制造内存压力。
 *   程序默认在 MemAvailable 过低或发现本进程开始产生 VmSwap 时停止，
 *   避免把虚拟机推到 OOM Killer 或严重卡顿。
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* 使用 unsigned long long，避免 MB 换算为字节时发生 int 溢出。 */
#define MB (1024ULL * 1024ULL)

typedef struct {
    long vm_size_kb;
    long vm_rss_kb;
    long rss_anon_kb;
    long vm_swap_kb;
} ProcStatus;

typedef struct {
    long mem_available_kb;
    long swap_free_kb;
} MemInfo;

/*
 * 将 kB 转成 MB，便于输出。
 * /proc 里的内存字段几乎都以 kB 为单位。
 */
static double kb_to_mb(long kb) {
    return (double)kb / 1024.0;
}

/*
 * 读取当前进程 /proc/self/status 中最关键的几个字段。
 *
 * VmSize：
 *   进程拥有的虚拟地址空间总大小。
 *
 * VmRSS：
 *   当前还驻留在物理内存中的页面总大小。
 *
 * RssAnon：
 *   匿名私有页中仍然驻留在物理内存里的部分。
 *   本实验使用 MAP_ANONYMOUS，不对应文件，所以它最接近“本进程真实占用的物理内存”。
 *
 * VmSwap：
 *   本进程匿名页被换出到 swap 的大小。
 *   如果 VmSwap 开始上升，说明继续压测已经不再是纯物理内存测试了。
 */
static int read_proc_status(ProcStatus *status) {
    memset(status, 0, sizeof(*status));

    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) {
        perror("/proc/self/status");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "VmSize: %ld kB", &status->vm_size_kb);
        sscanf(line, "VmRSS: %ld kB", &status->vm_rss_kb);
        sscanf(line, "RssAnon: %ld kB", &status->rss_anon_kb);
        sscanf(line, "VmSwap: %ld kB", &status->vm_swap_kb);
    }

    fclose(fp);
    return 0;
}

/*
 * 读取 /proc/meminfo 中的系统整体内存状态。
 *
 * MemAvailable：
 *   内核估算的“还能较安全拿来用”的内存量。
 *   它比 free 字段更适合做停止条件，因为 Linux 会把一部分空闲内存用作缓存。
 *
 * SwapFree：
 *   当前还空闲的 swap 大小。
 */
static int read_meminfo(MemInfo *info) {
    memset(info, 0, sizeof(*info));

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("/proc/meminfo");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "MemAvailable: %ld kB", &info->mem_available_kb);
        sscanf(line, "SwapFree: %ld kB", &info->swap_free_kb);
    }

    fclose(fp);
    return 0;
}

/* 返回单调递增时间，用来统计整个压测耗时。 */
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/*
 * 打印一行状态。
 *
 * allocated_mb：
 *   程序已经 mmap 并写触页的总大小。
 *
 * 注意：
 *   “已经写触页的总大小”不一定完全等于真实物理内存占用。
 *   真正要看的是 RssAnon。如果内存开始被换出，RssAnon 会下降，VmSwap 会上升。
 */
static void print_row(size_t step, unsigned long long allocated_mb,
                      const ProcStatus *status, const MemInfo *info) {
    printf("[%02zu] touched=%5llu MB | VmRSS=%8.1f MB | RssAnon=%8.1f MB | "
           "VmSwap=%8.1f MB | MemAvailable=%8.1f MB | SwapFree=%8.1f MB\n",
           step,
           allocated_mb,
           kb_to_mb(status->vm_rss_kb),
           kb_to_mb(status->rss_anon_kb),
           kb_to_mb(status->vm_swap_kb),
           kb_to_mb(info->mem_available_kb),
           kb_to_mb(info->swap_free_kb));
}

/*
 * 写触页函数：每隔一个页面写入一个字节。
 *
 * 为什么不是 memset 整块内存：
 *   页是 Linux 内存管理的基本单位。只要写到某个页中的任意一个字节，
 *   内核就必须给整个页分配真实物理页。
 *   因此每页写 1 字节足以达到“真正占用物理页”的目的，比写完整块更快。
 */
static void touch_every_page(unsigned char *addr, size_t length, size_t page_size,
                             unsigned char value) {
    for (size_t off = 0; off < length; off += page_size) {
        addr[off] = value;
    }
}

static unsigned long long parse_ull_arg(char **argv, int argc, int index,
                                        unsigned long long default_value) {
    if (argc <= index) {
        return default_value;
    }

    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(argv[index], &end, 10);
    if (errno != 0 || end == argv[index] || *end != '\0') {
        fprintf(stderr, "invalid numeric argument: %s\n", argv[index]);
        exit(1);
    }
    return value;
}

static void print_usage(const char *program) {
    fprintf(stderr,
            "usage: %s [chunk_mb] [reserve_mb] [max_mb] [hold_seconds]\n"
            "example: %s 64 512 0 0\n",
            program, program);
}

int main(int argc, char **argv) {
    if (argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    /*
     * 让输出按行刷新。
     * 即使通过 tee 保存日志，也能实时看到每一步进度。
     */
    setvbuf(stdout, NULL, _IOLBF, 0);

    unsigned long long chunk_mb = parse_ull_arg(argv, argc, 1, 64);
    unsigned long long reserve_mb = parse_ull_arg(argv, argc, 2, 512);
    unsigned long long max_mb = parse_ull_arg(argv, argc, 3, 0);
    unsigned long long hold_seconds = parse_ull_arg(argv, argc, 4, 0);

    if (chunk_mb == 0) {
        fprintf(stderr, "chunk_mb must be greater than 0\n");
        return 1;
    }

    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    size_t chunk_bytes = (size_t)(chunk_mb * MB);

    /*
     * 保存每一次 mmap 返回的地址，最后统一 munmap。
     * 只有持续持有这些映射，才能测出“同时最多占用多少物理内存”。
     */
    size_t capacity = 64;
    size_t count = 0;
    unsigned char **blocks = calloc(capacity, sizeof(*blocks));
    if (!blocks) {
        perror("calloc blocks");
        return 1;
    }

    ProcStatus base_status;
    MemInfo base_info;
    if (read_proc_status(&base_status) != 0 || read_meminfo(&base_info) != 0) {
        free(blocks);
        return 1;
    }

    printf("pid=%d, page_size=%zu bytes\n", getpid(), page_size);
    printf("chunk=%llu MB, reserve=%llu MB, max=%s, hold=%llu seconds\n",
           chunk_mb,
           reserve_mb,
           max_mb == 0 ? "unlimited" : argv[3],
           hold_seconds);
    printf("stop rule: stop before MemAvailable is below reserve, or when this process gets VmSwap.\n\n");

    print_row(0, 0, &base_status, &base_info);

    unsigned long long allocated_mb = 0;
    long base_swap_kb = base_status.vm_swap_kb;
    long peak_rss_anon_kb = base_status.rss_anon_kb;
    unsigned long long peak_at_allocated_mb = 0;
    double t0 = now_seconds();

    while (1) {
        ProcStatus status;
        MemInfo info;
        if (read_proc_status(&status) != 0 || read_meminfo(&info) != 0) {
            break;
        }

        /*
         * 先看剩余 MemAvailable 是否足够再申请下一块。
         * 加上 chunk_mb，是为了保证下一块写完后仍能留出 reserve_mb。
         */
        long need_before_next_kb = (long)((reserve_mb + chunk_mb) * 1024ULL);
        if (info.mem_available_kb <= need_before_next_kb) {
            printf("\nstop: MemAvailable %.1f MB is not enough for another %llu MB chunk "
                   "while keeping %llu MB reserved.\n",
                   kb_to_mb(info.mem_available_kb), chunk_mb, reserve_mb);
            break;
        }

        if (status.vm_swap_kb > base_swap_kb) {
            printf("\nstop: this process has started using VmSwap "
                   "(baseline %.1f MB -> current %.1f MB).\n",
                   kb_to_mb(base_swap_kb), kb_to_mb(status.vm_swap_kb));
            break;
        }

        if (max_mb != 0 && allocated_mb + chunk_mb > max_mb) {
            printf("\nstop: reached user max limit %llu MB.\n", max_mb);
            break;
        }

        if (count == capacity) {
            size_t new_capacity = capacity * 2;
            unsigned char **new_blocks = realloc(blocks, new_capacity * sizeof(*blocks));
            if (!new_blocks) {
                perror("realloc blocks");
                break;
            }
            blocks = new_blocks;
            capacity = new_capacity;
        }

        unsigned char *p = mmap(NULL, chunk_bytes, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            perror("mmap");
            break;
        }

        /*
         * 这一步才是本程序的关键：
         * mmap 只是拿到虚拟地址；写触页后，页面才会进入 RssAnon。
         */
        touch_every_page(p, chunk_bytes, page_size, (unsigned char)(count + 1));

        blocks[count++] = p;
        allocated_mb += chunk_mb;

        if (read_proc_status(&status) != 0 || read_meminfo(&info) != 0) {
            break;
        }

        if (status.rss_anon_kb > peak_rss_anon_kb) {
            peak_rss_anon_kb = status.rss_anon_kb;
            peak_at_allocated_mb = allocated_mb;
        }

        print_row(count, allocated_mb, &status, &info);
    }

    double t1 = now_seconds();
    printf("\nresult:\n");
    printf("  total touched virtual range: %llu MB\n", allocated_mb);
    printf("  peak RssAnon observed: %.1f MB, when touched=%llu MB\n",
           kb_to_mb(peak_rss_anon_kb), peak_at_allocated_mb);
    printf("  elapsed: %.3f seconds\n", t1 - t0);

    if (hold_seconds > 0) {
        printf("\nholding memory for %llu seconds; inspect with:\n", hold_seconds);
        printf("  grep -E '^(VmSize|VmRSS|RssAnon|VmSwap):' /proc/%d/status\n", getpid());
        sleep((unsigned int)hold_seconds);
    }

    for (size_t i = 0; i < count; ++i) {
        munmap(blocks[i], chunk_bytes);
    }
    free(blocks);

    ProcStatus end_status;
    MemInfo end_info;
    if (read_proc_status(&end_status) == 0 && read_meminfo(&end_info) == 0) {
        printf("\nafter cleanup:\n");
        print_row(0, 0, &end_status, &end_info);
    }

    return 0;
}
