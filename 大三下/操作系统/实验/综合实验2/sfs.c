#define _DEFAULT_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DISK_SIZE (20u * 1024u * 1024u)
#define BLOCK_SIZE 1024u
#define TOTAL_BLOCKS (DISK_SIZE / BLOCK_SIZE)
#define INODE_BITMAP_BLOCKS 1u
#define BLOCK_BITMAP_BLOCKS 3u
#define MAX_FILES 128u
#define NAME_LEN 64u
#define DIRECT_BLOCKS 32u
#define SFS_MAGIC 0x53465332u

// 超级块保存整个虚拟磁盘的布局信息，以及当前还能使用的空闲资源统计。
typedef struct
{
    // 文件系统标识，用于确认磁盘内容是否是本实验定义的 SFS。
    uint32_t magic;
    // 整个模拟磁盘的总字节数。
    uint32_t disk_size;
    // 每个磁盘块的大小，当前固定为 1KB。
    uint32_t block_size;
    // 模拟磁盘一共有多少个块。
    uint32_t total_blocks;
    // 最多允许创建多少个文件，也就是 inode 表容量。
    uint32_t max_files;
    // inode 位图在磁盘中的起始块号。
    uint32_t inode_bitmap_start;
    // 数据块位图在磁盘中的起始块号。
    uint32_t block_bitmap_start;
    // inode 表在磁盘中的起始块号。
    uint32_t inode_table_start;
    // inode 表一共占用多少个块。
    uint32_t inode_table_blocks;
    // 数据区的起始块号，之后的块才真正存放文件内容。
    uint32_t data_block_start;
    // 当前还剩多少个空闲 inode。
    uint32_t free_inodes;
    // 当前还剩多少个可用数据块。
    uint32_t free_blocks;
} SuperBlock;

// 这里的 inode 只描述普通文本文件：文件名、大小、已占用块数和直接块地址。
// 为了突出实验重点，没有实现间接块、目录层级等更复杂的机制。
typedef struct
{
    // 标记这个 inode 是否已经被分配并用于某个文件。
    uint8_t used;
    // 文件名，实验里不支持目录，因此名字直接唯一标识一个文件。
    char name[NAME_LEN];
    // 文件当前大小，单位是字节。
    uint32_t size;
    // 文件实际占用了多少个数据块。
    uint32_t block_count;
    // 直接块地址表，按顺序保存文件内容所在的数据块号。
    uint32_t blocks[DIRECT_BLOCKS];
} SfsInode;

static uint8_t *g_disk = NULL;
static pthread_mutex_t g_fs_lock = PTHREAD_MUTEX_INITIALIZER;

// 后面的辅助函数都默认 g_disk 已经指向一块完成格式化的内存磁盘。
static SuperBlock *super_block(void)
{
    return (SuperBlock *)g_disk;
}

static uint8_t *block_ptr(uint32_t block_no)
{
    return g_disk + (size_t)block_no * BLOCK_SIZE;
}

static uint8_t *inode_bitmap(void)
{
    return block_ptr(super_block()->inode_bitmap_start);
}

static uint8_t *block_bitmap(void)
{
    return block_ptr(super_block()->block_bitmap_start);
}

static SfsInode *inode_table(void)
{
    return (SfsInode *)block_ptr(super_block()->inode_table_start);
}

static int bitmap_test(uint8_t *bitmap, uint32_t index)
{
    return (bitmap[index / 8u] >> (index % 8u)) & 1u;
}

static void bitmap_set(uint8_t *bitmap, uint32_t index)
{
    bitmap[index / 8u] |= (uint8_t)(1u << (index % 8u));
}

static void bitmap_clear(uint8_t *bitmap, uint32_t index)
{
    bitmap[index / 8u] &= (uint8_t)~(1u << (index % 8u));
}

static uint32_t ceil_div_u32(uint32_t a, uint32_t b)
{
    return (a + b - 1u) / b;
}

static int valid_name(const char *name)
{
    return name != NULL && name[0] != '\0' && strchr(name, '/') == NULL &&
           strlen(name) < NAME_LEN;
}

static int find_inode_unlocked(const char *name)
{
    SfsInode *table = inode_table();
    for (uint32_t i = 0; i < MAX_FILES; ++i)
    {
        if (table[i].used && strcmp(table[i].name, name) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

static int alloc_inode_unlocked(void)
{
    uint8_t *bitmap = inode_bitmap();
    for (uint32_t i = 0; i < MAX_FILES; ++i)
    {
        if (!bitmap_test(bitmap, i))
        {
            bitmap_set(bitmap, i);
            super_block()->free_inodes--;
            return (int)i;
        }
    }
    return -1;
}

static void free_inode_unlocked(uint32_t inode_no)
{
    uint8_t *bitmap = inode_bitmap();
    if (inode_no < MAX_FILES && bitmap_test(bitmap, inode_no))
    {
        bitmap_clear(bitmap, inode_no);
        super_block()->free_inodes++;
    }
}

static int alloc_block_unlocked(void)
{
    SuperBlock *sb = super_block();
    uint8_t *bitmap = block_bitmap();
    // 数据区从 data_block_start 之后才开始扫描，前面的超级块和元数据块一律保留。
    for (uint32_t block = sb->data_block_start; block < sb->total_blocks; ++block)
    {
        if (!bitmap_test(bitmap, block))
        {
            bitmap_set(bitmap, block);
            memset(block_ptr(block), 0, BLOCK_SIZE);
            sb->free_blocks--;
            return (int)block;
        }
    }
    return -1;
}

static void free_block_unlocked(uint32_t block_no)
{
    SuperBlock *sb = super_block();
    uint8_t *bitmap = block_bitmap();
    if (block_no >= sb->data_block_start && block_no < sb->total_blocks &&
        bitmap_test(bitmap, block_no))
    {
        bitmap_clear(bitmap, block_no);
        memset(block_ptr(block_no), 0, BLOCK_SIZE);
        sb->free_blocks++;
    }
}

static void release_inode_blocks_unlocked(SfsInode *inode)
{
    // 删除或覆盖写入时，必须先把 inode 之前持有的所有数据块归还给位图。
    for (uint32_t i = 0; i < inode->block_count; ++i)
    {
        free_block_unlocked(inode->blocks[i]);
        inode->blocks[i] = 0;
    }
    inode->block_count = 0;
    inode->size = 0;
}

void sfs_format(uint8_t *disk)
{
    g_disk = disk;
    memset(g_disk, 0, DISK_SIZE);

    SuperBlock *sb = super_block();
    uint32_t inode_table_bytes = MAX_FILES * (uint32_t)sizeof(SfsInode);
    uint32_t inode_table_blocks = ceil_div_u32(inode_table_bytes, BLOCK_SIZE);

    // 磁盘布局顺序固定为：超级块 | inode 位图 | 数据块位图 | inode 表 | 数据区。
    // 这样所有元数据都位于磁盘前部，便于通过固定偏移直接访问。
    sb->magic = SFS_MAGIC;
    sb->disk_size = DISK_SIZE;
    sb->block_size = BLOCK_SIZE;
    sb->total_blocks = TOTAL_BLOCKS;
    sb->max_files = MAX_FILES;
    sb->inode_bitmap_start = 1u;
    sb->block_bitmap_start = sb->inode_bitmap_start + INODE_BITMAP_BLOCKS;
    sb->inode_table_start = sb->block_bitmap_start + BLOCK_BITMAP_BLOCKS;
    sb->inode_table_blocks = inode_table_blocks;
    sb->data_block_start = sb->inode_table_start + sb->inode_table_blocks;
    sb->free_inodes = MAX_FILES;
    sb->free_blocks = sb->total_blocks - sb->data_block_start;

    uint8_t *bb = block_bitmap();
    for (uint32_t block = 0; block < sb->data_block_start; ++block)
    {
        bitmap_set(bb, block);
    }

    printf("[format] disk=%u bytes (%u MB), block=%u bytes, total_blocks=%u\n",
           sb->disk_size, sb->disk_size / 1024u / 1024u, sb->block_size,
           sb->total_blocks);
    printf("[format] metadata: super=0, inode_bitmap=%u, block_bitmap=%u..%u, inode_table=%u..%u\n",
           sb->inode_bitmap_start, sb->block_bitmap_start,
           sb->block_bitmap_start + BLOCK_BITMAP_BLOCKS - 1u,
           sb->inode_table_start,
           sb->inode_table_start + sb->inode_table_blocks - 1u);
    printf("[format] data_block_start=%u, free_inodes=%u, free_data_blocks=%u\n",
           sb->data_block_start, sb->free_inodes, sb->free_blocks);
}

int sfs_open(const char *name)
{
    int rc = 0;
    pthread_mutex_lock(&g_fs_lock);
    if (!valid_name(name))
    {
        printf("[open] invalid file name: %s\n", name ? name : "(null)");
        rc = -1;
        goto out;
    }

    // 同名文件已经存在时，不重复创建，直接提示调用者即可。
    int existing = find_inode_unlocked(name);
    if (existing >= 0)
    {
        printf("[open] %s already exists at inode %d\n", name, existing);
        goto out;
    }

    // 先在 inode 位图里申请一个空闲编号，再把文件信息写入 inode 表。
    int ino = alloc_inode_unlocked();
    if (ino < 0)
    {
        printf("[open] no free inode for %s\n", name);
        rc = -1;
        goto out;
    }

    SfsInode *inode = &inode_table()[ino];
    memset(inode, 0, sizeof(*inode));
    inode->used = 1u;
    strncpy(inode->name, name, NAME_LEN - 1u);
    printf("[open] created %s at inode %d\n", name, ino);

out:
    pthread_mutex_unlock(&g_fs_lock);
    return rc;
}

int sfs_write(const char *name, const char *text)
{
    int rc = 0;
    pthread_mutex_lock(&g_fs_lock);
    int ino = find_inode_unlocked(name);
    if (ino < 0)
    {
        printf("[write] %s not found\n", name);
        rc = -1;
        goto out;
    }
    if (text == NULL)
    {
        printf("[write] null text for %s\n", name);
        rc = -1;
        goto out;
    }

    SfsInode *inode = &inode_table()[ino];
    uint32_t len = (uint32_t)strlen(text);
    // 一个文件的内容按 1KB 分块存储，最后一块只写入实际长度对应的字节数。
    uint32_t need_blocks = len == 0 ? 0 : ceil_div_u32(len, BLOCK_SIZE);
    if (need_blocks > DIRECT_BLOCKS)
    {
        printf("[write] %s too large: %u bytes, max=%u bytes\n", name, len,
               DIRECT_BLOCKS * BLOCK_SIZE);
        rc = -1;
        goto out;
    }
    if (super_block()->free_blocks + inode->block_count < need_blocks)
    {
        printf("[write] no enough blocks for %s: need=%u free_plus_old=%u\n", name,
               need_blocks, super_block()->free_blocks + inode->block_count);
        rc = -1;
        goto out;
    }

    // 覆盖写采用“先释放旧块，再重新分配”的策略，逻辑简单，便于验证位图状态。
    release_inode_blocks_unlocked(inode);
    inode->size = len;
    inode->block_count = need_blocks;
    for (uint32_t i = 0; i < need_blocks; ++i)
    {
        int block = alloc_block_unlocked();
        if (block < 0)
        {
            printf("[write] internal allocation failure for %s\n", name);
            rc = -1;
            goto out;
        }
        inode->blocks[i] = (uint32_t)block;
        uint32_t offset = i * BLOCK_SIZE;
        uint32_t chunk = len - offset > BLOCK_SIZE ? BLOCK_SIZE : len - offset;
        memcpy(block_ptr((uint32_t)block), text + offset, chunk);
    }
    printf("[write] %s size=%u bytes blocks=%u\n", name, inode->size,
           inode->block_count);

out:
    pthread_mutex_unlock(&g_fs_lock);
    return rc;
}
// out 缓冲区指针
// out_size 缓冲区大小
int sfs_read(const char *name, char *out, size_t out_size)
{
    int rc = 0;
    pthread_mutex_lock(&g_fs_lock);
    int ino = find_inode_unlocked(name);
    if (ino < 0)
    {
        printf("[read] %s not found\n", name);
        rc = -1;
        goto out;
    }

    SfsInode *inode = &inode_table()[ino];
    // 读取的大小 out_size 必须充足 >= inode->size + 1
    // 读取结果要作为 C 字符串返回，因此缓冲区必须额外留出 1 个字节放 '\0'
    if (out_size <= inode->size)
    {
        printf("[read] buffer too small for %s: need=%u got=%zu\n", name,
               inode->size + 1u, out_size);
        rc = -1;
        goto out;
    }

    uint32_t copied = 0;
    for (uint32_t i = 0; i < inode->block_count; ++i)
    {
        uint32_t chunk = inode->size - copied > BLOCK_SIZE ? BLOCK_SIZE
                                                           : inode->size - copied;
        memcpy(out + copied, block_ptr(inode->blocks[i]), chunk);
        copied += chunk;
    }
    out[inode->size] = '\0';
    printf("[read] %s size=%u bytes blocks=%u preview=\"%.144s%s\"\n", name,
           inode->size, inode->block_count, out, inode->size > 144 ? "..." : "");

out:
    pthread_mutex_unlock(&g_fs_lock);
    return rc;
}

int sfs_rm(const char *name)
{
    int rc = 0;
    pthread_mutex_lock(&g_fs_lock);
    int ino = find_inode_unlocked(name);
    if (ino < 0)
    {
        printf("[rm] %s not found\n", name);
        rc = -1;
        goto out;
    }

    SfsInode *inode = &inode_table()[ino];
    release_inode_blocks_unlocked(inode);
    memset(inode, 0, sizeof(*inode));
    free_inode_unlocked((uint32_t)ino);
    printf("[rm] removed %s from inode %d\n", name, ino);

out:
    pthread_mutex_unlock(&g_fs_lock);
    return rc;
}

void sfs_list(void)
{
    pthread_mutex_lock(&g_fs_lock);
    SuperBlock *sb = super_block();
    SfsInode *table = inode_table();
    printf("[ls] files in simple file system:\n");
    int count = 0;
    for (uint32_t i = 0; i < MAX_FILES; ++i)
    {
        if (table[i].used)
        {
            printf("  inode=%03u name=%-16s size=%5u bytes blocks=%2u\n", i,
                   table[i].name, table[i].size, table[i].block_count);
            count++;
        }
    }
    printf("[ls] total_files=%d free_inodes=%u free_data_blocks=%u\n", count,
           sb->free_inodes, sb->free_blocks);
    pthread_mutex_unlock(&g_fs_lock);
}

static void make_payload(char *buf, size_t size, const char *thread_name,
                         const char *file_name, char fill, int repeat)
{
    int n = snprintf(buf, size,
                     "%s writes %s. This text is stored in the simulated disk. ",
                     thread_name, file_name);
    if (n < 0)
    {
        buf[0] = '\0';
        return;
    }
    size_t used = (size_t)n < size ? (size_t)n : size - 1u;
    for (int i = 0; i < repeat && used + 2u < size; ++i)
    {
        buf[used++] = fill;
        if ((i + 1) % 64 == 0 && used + 2u < size)
        {
            buf[used++] = '|';
        }
    }
    buf[used] = '\0';
}

typedef struct
{
    // 线程显示名称，用于日志输出，方便区分不同并发执行者。
    const char *thread_name;
    // 线程要操作的第一个文件名。
    const char *file_a;
    // 线程要操作的第二个文件名。
    const char *file_b;
    // 填充第一个文件内容时使用的字符。
    char fill_a;
    // 填充第二个文件内容时使用的字符。
    char fill_b;
} ThreadJob;

static void *worker(void *arg)
{
    ThreadJob *job = (ThreadJob *)arg;
    char payload[1800];
    char read_buf[4096];

    // 两个线程并发调用同一套接口，用来验证互斥锁确实保护了共享磁盘状态。
    printf("[%s] start\n", job->thread_name);

    make_payload(payload, sizeof(payload), job->thread_name, job->file_a,
                 job->fill_a, 1300);
    sfs_open(job->file_a);
    usleep(100000);
    sfs_write(job->file_a, payload);
    usleep(100000);
    sfs_read(job->file_a, read_buf, sizeof(read_buf));
    usleep(100000);

    make_payload(payload, sizeof(payload), job->thread_name, job->file_b,
                 job->fill_b, 900);
    sfs_open(job->file_b);
    usleep(100000);
    sfs_write(job->file_b, payload);
    usleep(100000);
    sfs_read(job->file_b, read_buf, sizeof(read_buf));
    printf("[%s] finish\n", job->thread_name);
    return NULL;
}

static void run_basic_interface_demo(void)
{
    char read_buf[4096];
    // 这段演示依次覆盖创建、写入、再次写入和删除，方便观察文件生命周期变化。
    printf("\n== Basic file interface demo ==\n");
    sfs_open("manual.txt");
    sfs_write("manual.txt", "first version: open + write + read");
    sfs_read("manual.txt", read_buf, sizeof(read_buf));
    sfs_write("manual.txt", "second version: overwrite keeps file name but replaces blocks");
    sfs_read("manual.txt", read_buf, sizeof(read_buf));
    sfs_rm("manual.txt");
    sfs_read("manual.txt", read_buf, sizeof(read_buf));
}

static void run_thread_demo(void)
{
    pthread_t t1;
    pthread_t t2;
    ThreadJob job1 = {"thread-1", "t1_alpha.txt", "t1_beta.txt", 'A', 'B'};
    ThreadJob job2 = {"thread-2", "t2_alpha.txt", "t2_beta.txt", 'X', 'Y'};

    // 两个工作线程并行运行，重点压测 inode 查找、块分配和回收的并发安全性。
    printf("\n== Two pthread writers/readers demo ==\n");
    if (pthread_create(&t1, NULL, worker, &job1) != 0)
    {
        perror("pthread_create thread-1");
        exit(1);
    }
    if (pthread_create(&t2, NULL, worker, &job2) != 0)
    {
        perror("pthread_create thread-2");
        exit(1);
    }
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

int main(void)
{
    uint8_t *disk = (uint8_t *)malloc(DISK_SIZE);
    if (disk == NULL)
    {
        fprintf(stderr, "malloc %u bytes failed: %s\n", DISK_SIZE, strerror(errno));
        return 1;
    }

    printf("Simple File System experiment 2\n");
    printf("Target: 20M malloc disk, 1KB blocks, no directory, text files only\n\n");
    sfs_format(disk);
    run_basic_interface_demo();
    run_thread_demo();
    printf("\n== Final file table ==\n");
    sfs_list();

    free(disk);
    return 0;
}
