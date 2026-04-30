#ifndef SLUB_MM_H
#define SLUB_MM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int slub_init(void);
    void *slub_malloc(size_t size);
    void slub_free(void *ptr);
    void slub_destroy(void);
#ifdef __cplusplus
}
#endif

#endif