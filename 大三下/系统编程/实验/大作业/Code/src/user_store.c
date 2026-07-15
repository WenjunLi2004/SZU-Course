/*
 * user_store.c
 *
 * 共享内存用户表实现。
 * 初始化：shm_open(O_CREAT) -> fchmod(0600) -> ftruncate -> mmap ->
 * 在共享区内用 PTHREAD_PROCESS_SHARED 初始化互斥锁 -> 写 magic/version。
 * 所有对 user_count/users[]/send_count/offline_messages 的访问都在锁内完成；
 * 调用方把要用于 FIFO 写的数据复制到本地快照后，在锁外做 FIFO I/O。
 */

#include "user_store.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    snprintf(dst, dst_size, "%s", src == NULL ? "" : src);
}

static void su_lock(UserStoreHandle *h)   { pthread_mutex_lock(&h->store->mutex); }
static void su_unlock(UserStoreHandle *h) { pthread_mutex_unlock(&h->store->mutex); }

static int find_locked(const ChatUserStore *s, const char *username)
{
    int i;
    if (!username || !username[0]) return -1;
    for (i = 0; i < s->user_count; i++)
        if (strcmp(s->users[i].username, username) == 0) return i;
    return -1;
}

/* 共享区内的简单 LCG，调用前须持锁。 */
static unsigned lcg_next(ChatUserStore *s)
{
    s->rand_state = s->rand_state * 1103515245u + 12345u;
    return (s->rand_state >> 16) & 0x7fffu;
}

/* 持锁前提下为 viewer（下标 vi，可为 -1）构造在线名单串，并回填在线总数。 */
static void build_list_locked(const ChatUserStore *s, int vi,
                              char *buf, size_t bufsz, int *online_count)
{
    int i, count = 0;
    size_t off = 0;
    if (bufsz) buf[0] = '\0';
    for (i = 0; i < s->user_count; i++)
    {
        int important, n;
        if (!s->users[i].online) continue;
        count++;
        /* “重要朋友”是相对 viewer 而言的：viewer 给此人累计发信 >5 次才标 *，且不给自己标。 */
        important = (vi >= 0 && i != vi && s->send_count[vi][i] > CHAT_IMPORTANT_FRIEND_MIN);
        n = snprintf(buf + off, bufsz - off, "%s%s%s",
                     off ? "," : "", s->users[i].username, important ? "*" : "");
        if (n > 0 && (size_t)n < bufsz - off) off += (size_t)n;
        else { off = bufsz ? bufsz - 1 : 0; }  /* 截断保护：名单过长则停止追加 */
    }
    if (online_count) *online_count = count;
}

int user_store_init(UserStoreHandle *h, const char *short_name)
{
    char                name[64];
    int                 fd = -1;
    ChatUserStore      *s = MAP_FAILED;
    pthread_mutexattr_t attr;
    int                 pshared_ok = 0;
    int                 rc;

    memset(h, 0, sizeof(*h));

    rc = snprintf(name, sizeof(name), "/chatroom_%s_users",
                  (short_name && short_name[0]) ? short_name : "default");
    if (rc < 0 || (size_t)rc >= sizeof(name)) { errno = ENAMETOOLONG; return -1; }

    fd = shm_open(name, O_CREAT | O_RDWR, 0600);
    if (fd == -1) return -1;

    /* 有的平台不支持对 shm fd 直接 fchmod（返回 EINVAL）：退而用 fstat 核对权限，
     * 只要没有“组/其他可访问”位即放行。 */
    if (fchmod(fd, 0600) == -1)
    {
        struct stat st;
        if (fstat(fd, &st) == -1) goto fail;
        if ((st.st_mode & 077) != 0) { errno = EPERM; goto fail; }
    }

    if (ftruncate(fd, (off_t)sizeof(ChatUserStore)) == -1) goto fail;

    s = mmap(NULL, sizeof(ChatUserStore), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s == MAP_FAILED) goto fail;
    close(fd);
    fd = -1;

    /* shm 对象可能残留自上一次运行：每次启动都清零，从干净的用户表开始。 */
    memset(s, 0, sizeof(*s));

    if (pthread_mutexattr_init(&attr) != 0) goto fail_unmap;
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) == 0)
        pshared_ok = 1;
    rc = pthread_mutex_init(&s->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (rc != 0) { errno = rc; goto fail_unmap; }

    s->magic = CHAT_USER_STORE_MAGIC;
    s->version = CHAT_USER_STORE_VERSION;
    s->user_count = 0;
    s->bot_seq = 0;
    s->rand_state = (unsigned)(time(NULL) ^ (long)getpid() ^ 0x9e3779b9u);

    copy_string(h->name, sizeof(h->name), name);
    h->store = s;
    h->size = sizeof(ChatUserStore);
    h->process_shared = pshared_ok;
    return 0;

    /* 失败回滚：按已获取资源的相反顺序释放；先存住 errno，别被后面的清理调用覆盖了失败原因。 */
fail_unmap:
    munmap(s, sizeof(ChatUserStore));
fail:
    {
        int saved = errno;
        if (fd != -1) close(fd);
        shm_unlink(name);
        errno = saved;
    }
    return -1;
}

void user_store_destroy(UserStoreHandle *h)
{
    if (h->store != NULL)
    {
        pthread_mutex_destroy(&h->store->mutex);
        munmap(h->store, h->size);
        h->store = NULL;
    }
    if (h->name[0] != '\0')
    {
        shm_unlink(h->name);
        h->name[0] = '\0';
    }
}

UserStoreStatus user_store_register(UserStoreHandle *h,
                                    const char *username,
                                    const char *password,
                                    const char *fifo,
                                    int is_bot)
{
    ChatUserStore  *s = h->store;
    UserStoreStatus ret;

    su_lock(h);
    if (find_locked(s, username) != -1)
        ret = USER_STORE_ERR_EXISTS;
    else if (s->user_count >= CHAT_MAX_USERS)
        ret = USER_STORE_ERR_FULL;
    else
    {
        ChatUserRecord *r = &s->users[s->user_count];
        memset(r, 0, sizeof(*r));
        copy_string(r->username, sizeof(r->username), username);
        copy_string(r->password, sizeof(r->password), password);
        copy_string(r->fifo, sizeof(r->fifo), fifo);
        r->online = 0;
        r->is_bot = is_bot ? 1 : 0;
        s->user_count++;
        ret = USER_STORE_OK;
    }
    su_unlock(h);
    return ret;
}

UserStoreStatus user_store_login(UserStoreHandle *h, const char *username,
                                 const char *password, const char *fifo, long now)
{
    ChatUserStore  *s = h->store;
    UserStoreStatus ret;
    int             idx;

    su_lock(h);
    idx = find_locked(s, username);
    if (idx < 0)
        ret = USER_STORE_ERR_NOT_FOUND;
    else if (strcmp(s->users[idx].password, password) != 0)
        ret = USER_STORE_ERR_BAD_PASSWORD;
    else
    {
        copy_string(s->users[idx].fifo, sizeof(s->users[idx].fifo), fifo);
        s->users[idx].online = 1;
        s->users[idx].login_time = now;
        ret = USER_STORE_OK;
    }
    su_unlock(h);
    return ret;
}

UserStoreStatus user_store_logout(UserStoreHandle *h, const char *username,
                                  const char *fifo, long now)
{
    ChatUserStore  *s = h->store;
    UserStoreStatus ret;
    int             idx;

    su_lock(h);
    idx = find_locked(s, username);
    if (idx < 0)
        ret = USER_STORE_ERR_NOT_FOUND;
    else if (strcmp(s->users[idx].fifo, fifo) != 0)
        ret = USER_STORE_ERR_FIFO_MISMATCH;
    else
    {
        s->users[idx].online = 0;
        s->users[idx].logout_time = now;
        ret = USER_STORE_OK;
    }
    su_unlock(h);
    return ret;
}

void user_store_build_online_list(UserStoreHandle *h, const char *viewer,
                                  char *buf, size_t bufsz, int *online_count)
{
    ChatUserStore *s = h->store;
    su_lock(h);
    build_list_locked(s, find_locked(s, viewer), buf, bufsz, online_count);
    su_unlock(h);
}

int user_store_build_online_broadcast(UserStoreHandle *h, ChatPacket *packets,
                                      char fifos[][CHAT_FIFO_PATH_LEN], int max)
{
    ChatUserStore *s = h->store;
    int i, n = 0, total = 0;

    su_lock(h);
    for (i = 0; i < s->user_count; i++) if (s->users[i].online) total++;
    for (i = 0; i < s->user_count && n < max; i++)
    {
        ChatPacket *p;
        if (!s->users[i].online || s->users[i].fifo[0] == '\0') continue; /* 跳过机器人/无 FIFO */
        p = &packets[n];
        memset(p, 0, sizeof(*p));
        p->type = CHAT_PACKET_ONLINE_LIST;
        p->ok = 1;
        copy_string(p->from, sizeof(p->from), "server");
        build_list_locked(s, i, p->message, sizeof(p->message), NULL);
        p->online_count = total;
        copy_string(fifos[n], CHAT_FIFO_PATH_LEN, s->users[i].fifo);
        n++;
    }
    su_unlock(h);
    return n;
}

int user_store_snapshot_receiver_fifos(UserStoreHandle *h,
                                       char fifos[][CHAT_FIFO_PATH_LEN], int max,
                                       const char *exclude)
{
    ChatUserStore *s = h->store;
    int i, n = 0;

    su_lock(h);
    for (i = 0; i < s->user_count && n < max; i++)
    {
        if (!s->users[i].online || s->users[i].fifo[0] == '\0') continue;
        if (exclude && exclude[0] && strcmp(s->users[i].username, exclude) == 0) continue;
        copy_string(fifos[n], CHAT_FIFO_PATH_LEN, s->users[i].fifo);
        n++;
    }
    su_unlock(h);
    return n;
}

void user_store_prepare_send(UserStoreHandle *h, const char *from, const char *to,
                             ChatSendInfo *out)
{
    ChatUserStore *s = h->store;
    int fi, ti;

    memset(out, 0, sizeof(*out));
    su_lock(h);
    fi = find_locked(s, from);
    if (fi >= 0)
    {
        out->sender_online = s->users[fi].online;
        copy_string(out->from_fifo, sizeof(out->from_fifo), s->users[fi].fifo);
    }
    ti = find_locked(s, to);
    if (ti >= 0)
    {
        out->target_exists = 1;
        out->target_online = s->users[ti].online;
        out->target_is_bot = s->users[ti].is_bot;
        copy_string(out->to_fifo, sizeof(out->to_fifo), s->users[ti].fifo);
    }
    if (fi >= 0 && ti >= 0) out->send_count = s->send_count[fi][ti];
    su_unlock(h);
}

int user_store_increment_send(UserStoreHandle *h, const char *from, const char *to)
{
    ChatUserStore *s = h->store;
    int fi, ti, val = -1;
    su_lock(h);
    fi = find_locked(s, from);
    ti = find_locked(s, to);
    if (fi >= 0 && ti >= 0) val = ++s->send_count[fi][ti];
    su_unlock(h);
    return val;
}

int user_store_set_offline(UserStoreHandle *h, const char *username, const char *fifo)
{
    ChatUserStore *s = h->store;
    int idx, marked = 0;
    su_lock(h);
    idx = find_locked(s, username);
    /* 在线投递失败说明该用户的私有 FIFO 已不可写，认定其已掉线：username 与 fifo 都匹配、
     * 且当前仍 online 时置离线，之后发来的消息走离线暂存。 */
    if (idx >= 0 && s->users[idx].online && strcmp(s->users[idx].fifo, fifo) == 0)
    {
        s->users[idx].online = 0;
        marked = 1;
    }
    su_unlock(h);
    return marked;
}

UserStoreStatus user_store_store_offline(UserStoreHandle *h, const char *from,
                                         const char *to, const char *text, long timestamp)
{
    ChatUserStore  *s = h->store;
    UserStoreStatus ret = USER_STORE_ERR_FULL;
    int i;
    su_lock(h);
    /* 在定长离线队列里找第一个空槽存入；全满则返回 USER_STORE_ERR_FULL。 */
    for (i = 0; i < CHAT_MAX_OFFLINE_MESSAGES; i++)
    {
        if (s->offline_messages[i].used) continue;
        s->offline_messages[i].used = 1;
        copy_string(s->offline_messages[i].from, CHAT_NAME_LEN, from);
        copy_string(s->offline_messages[i].to, CHAT_NAME_LEN, to);
        copy_string(s->offline_messages[i].text, CHAT_TEXT_LEN, text);
        s->offline_messages[i].timestamp = timestamp;
        ret = USER_STORE_OK;
        break;
    }
    su_unlock(h);
    return ret;
}

int user_store_pop_offline(UserStoreHandle *h, const char *user,
                           ChatOfflineMessage *out, int max)
{
    ChatUserStore *s = h->store;
    int i, n = 0;
    su_lock(h);
    /* 一次加锁内取出并清空发给 user 的所有离线消息（取出即清槽）。 */
    for (i = 0; i < CHAT_MAX_OFFLINE_MESSAGES && n < max; i++)
    {
        if (!s->offline_messages[i].used) continue;
        if (strcmp(s->offline_messages[i].to, user) != 0) continue;
        out[n++] = s->offline_messages[i];
        s->offline_messages[i].used = 0;
    }
    su_unlock(h);
    return n;
}

int user_store_lookup_fifo(UserStoreHandle *h, const char *username,
                           char *buf, size_t bufsz)
{
    ChatUserStore *s = h->store;
    int idx, found = 0;
    su_lock(h);
    idx = find_locked(s, username);
    if (idx >= 0) { copy_string(buf, bufsz, s->users[idx].fifo); found = 1; }
    su_unlock(h);
    return found;
}

int user_store_lookup_online_client_fifo(UserStoreHandle *h, const char *username,
                                         char *buf, size_t bufsz)
{
    ChatUserStore *s = h->store;
    int idx, found = 0;
    su_lock(h);
    idx = find_locked(s, username);
    /* 只有“在线 + 非机器人 + 有私有 FIFO”的真实客户端用户才算合法的机器人管理发起者。 */
    if (idx >= 0 && s->users[idx].online && !s->users[idx].is_bot &&
        s->users[idx].fifo[0] != '\0')
    {
        copy_string(buf, bufsz, s->users[idx].fifo);
        found = 1;
    }
    su_unlock(h);
    return found;
}

UserStoreStatus user_store_add_bot(UserStoreHandle *h,
                                   char *out_name,
                                   size_t out_sz,
                                   long now)
{
    ChatUserStore  *s = h->store;
    UserStoreStatus ret;

    su_lock(h);
    if (s->user_count >= CHAT_MAX_USERS)
        ret = USER_STORE_ERR_FULL;
    else
    {
        ChatUserRecord *r;
        char name[CHAT_NAME_LEN];
        char pw[CHAT_PASSWORD_LEN];
        int  tries = 0;
        /* 递增序号 + 随机后缀拼用户名，撞名就重试（至多 64 次），保证机器人用户名唯一。 */
        do {
            unsigned long seq = ++s->bot_seq;
            snprintf(name, sizeof(name), "lwjbot_%lu_%u",
                     seq, lcg_next(s) % 1000u);
        } while (find_locked(s, name) != -1 && ++tries < 64);
        snprintf(pw, sizeof(pw), "bp%u%u", lcg_next(s), lcg_next(s));

        r = &s->users[s->user_count];
        memset(r, 0, sizeof(*r));
        copy_string(r->username, sizeof(r->username), name);
        copy_string(r->password, sizeof(r->password), pw);
        r->fifo[0] = '\0';            /* 机器人无客户端 FIFO */
        r->online = 1;
        r->is_bot = 1;
        r->login_time = now;
        s->user_count++;
        copy_string(out_name, out_sz, name);
        ret = USER_STORE_OK;
    }
    su_unlock(h);
    return ret;
}

int user_store_pick_online_bots(UserStoreHandle *h,
                                int x,
                                char names[][CHAT_NAME_LEN],
                                int max,
                                long now)
{
    ChatUserStore *s = h->store;
    int idxs[CHAT_MAX_USERS];
    int i, m = 0, want, k, n = 0;

    su_lock(h);
    for (i = 0; i < s->user_count; i++)
        if (s->users[i].online && s->users[i].is_bot)
            idxs[m++] = i;

    want = x;
    if (want > m) want = m;
    if (want > max) want = max;

    /* 部分 Fisher–Yates 洗牌：只洗前 want 个，从 m 个在线机器人里等概率、无放回地抽取，O(want)。 */
    for (k = 0; k < want; k++)
    {
        int r = k + (int)(lcg_next(s) % (unsigned)(m - k));
        int bi = idxs[r];
        idxs[r] = idxs[k];
        idxs[k] = bi;
        s->users[bi].online = 0;
        s->users[bi].logout_time = now;
        copy_string(names[n], CHAT_NAME_LEN, s->users[bi].username);
        n++;
    }
    su_unlock(h);
    return n;
}
