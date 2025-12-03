/* this is an example of how to do a LRU cache in C using uthash */
#include <string.h>
#include <stdio.h>
#include "src/uthash.h"

#define MAX_CACHE_SIZE 50U /* a real value would be much larger */

struct CacheEntry
{
    char *key;
    char *value;
    UT_hash_handle hh;
};
struct CacheEntry *cache = NULL;

static void add_to_cache(const char *key, const char *value)
{
    struct CacheEntry *entry, *tmp_entry;
    entry = (struct CacheEntry *)malloc(sizeof(struct CacheEntry));
    if (entry == NULL)
    {
        exit(-1);
    }
    entry->key = strdup(key);
    entry->value = strdup(value);
    HASH_ADD_KEYPTR(hh, cache, entry->key, strlen(entry->key), entry);

    // prune the cache to MAX_CACHE_SIZE
    if (HASH_COUNT(cache) >= MAX_CACHE_SIZE)
    {
        HASH_ITER(hh, cache, entry, tmp_entry)
        {
            // prune the first entry (loop is based on insertion order so this deletes the oldest item)
            printf("LRU deleting %s %s\n", entry->key, entry->value);
            HASH_DELETE(hh, cache, entry);
            free(entry->key);
            free(entry->value);
            free(entry);
            break;
        }
    }
}

int main()
{
    char linebuf[100];
    char nbuf[11];
    FILE *file;
    unsigned int i = 0;

    file = fopen("LRU.dat", "r");
    if (file == NULL)
    {
        perror("can't open: ");
        exit(-1);
    }

    while (fgets(linebuf, sizeof(linebuf), file) != NULL)
    {
        snprintf(nbuf, sizeof(nbuf), "%u", i++);
        add_to_cache(linebuf, nbuf);
    }

    fclose(file);
    return 0;
}
