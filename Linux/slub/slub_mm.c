#include "slub_mm.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define PAGE_SIZE 4096UL
#define POOL_SIZE (8 * PAGE_SIZE) // 32KB内存池
#define MAX_OBJ_SIZE 1024UL
#define ALIGNMENT 8UL

// 向上对齐宏
#define ALIGN_UP(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// SLUB对象缓存
typedef struct kmem_cache
{
    size_t obj_size; // 对象大小
    void *freelist;  // 空闲对象链表
} kmem_cache_t;

// 支持的对象大小
static const size_t obj_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024};
#define CACHE_COUNT 8UL

static void *g_pool;
static size_t g_pool_used;
static kmem_cache_t g_caches[CACHE_COUNT];
static kmem_cache_t *g_slab_cache[POOL_SIZE / PAGE_SIZE];

// 查找对应大小的缓存
static kmem_cache_t *get_cache(size_t size)
{
    if (size == 0 || size > MAX_OBJ_SIZE)
        return NULL;
    size_t align_size = ALIGN_UP(size);
    for (int i = 0; i < CACHE_COUNT; i++)
    {
        if (obj_sizes[i] >= align_size)
        {
            return &g_caches[i];
        }
    }
    return NULL;
}

// 分配新的Slab，并初始化空闲链表
static int alloc_new_slab(kmem_cache_t *cache)
{
    if (g_pool_used + PAGE_SIZE > POOL_SIZE)
        return -1;

    size_t slab_idx = g_pool_used / PAGE_SIZE;
    uint8_t *slab = (uint8_t *)g_pool + g_pool_used;
    g_pool_used += PAGE_SIZE;
    size_t obj_size = cache->obj_size;
    int obj_cnt = (PAGE_SIZE) / obj_size;

    // 串起所有空闲对象
    for (int i = 0; i < obj_cnt - 1; i++)
    {
        uint8_t *curr = slab + i * obj_size;
        uint8_t *next = slab + (i + 1) * obj_size;
        *(void **)curr = next;
    }
    // 链表尾置空
    *(void **)(slab + (obj_cnt - 1) * obj_size) = NULL;

    // 挂载到缓存
    cache->freelist = slab;
    g_slab_cache[slab_idx] = cache;
    return 0;
}

int slub_init(void)
{
    g_pool = mmap(NULL, POOL_SIZE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_pool == MAP_FAILED)
    {
        perror("mmap failed");
        return -1;
    }

    g_pool_used = 0;
    // 初始化所有缓存
    for (int i = 0; i < CACHE_COUNT; i++)
    {
        g_caches[i].obj_size = obj_sizes[i];
        g_caches[i].freelist = NULL;
    }
    memset(g_slab_cache, 0, sizeof(g_slab_cache));

    printf("[SLUB] 初始化成功 | 内存池：%luKB | 页大小：%lu\n",
           POOL_SIZE / 1024, PAGE_SIZE);
    return 0;
}

void *slub_malloc(size_t size)
{
    kmem_cache_t *cache = get_cache(size);
    if (!cache)
    {
        printf("[malloc错误] 不支持的大小：%zu\n", size);
        return NULL;
    }

    // 分配新slab
    if (!cache->freelist)
    {
        if (alloc_new_slab(cache) != 0)
        {
            printf("[malloc错误] 内存池耗尽\n");
            return NULL;
        }
    }

    // 从链表头取对象
    void *obj = cache->freelist;
    cache->freelist = *(void **)obj;
    return obj;
}

void slub_free(void *ptr)
{
    if (!ptr)
        return;

    uintptr_t pool_start = (uintptr_t)g_pool;
    uintptr_t pool_end = pool_start + POOL_SIZE;
    uintptr_t addr = (uintptr_t)ptr;

    if (addr < pool_start || addr >= pool_end)
    {
        printf("[free错误] 非法指针\n");
        return;
    }

    // 计算所属slab
    size_t offset = addr - pool_start;
    size_t slab_idx = offset / PAGE_SIZE;
    kmem_cache_t *cache = g_slab_cache[slab_idx];
    if (!cache)
        return;

    // 插回空闲链表头
    *(void **)ptr = cache->freelist;
    cache->freelist = ptr;
}

void slub_destroy(void)
{
    if (g_pool)
    {
        munmap(g_pool, POOL_SIZE);
        g_pool = NULL;
        printf("[SLUB] 内存池已释放\n");
    }
}
