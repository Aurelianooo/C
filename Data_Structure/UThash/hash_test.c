#include <stdio.h>
#include "src/uthash.h"

#ifdef BASIC_TEST
typedef struct user_t
{
    int id; /* key */
    int cookie;
    UT_hash_handle hh;
} user_t;
#endif

#ifdef STRING_TEST
typedef struct user_t
{
    char name[10]; /* key */
    int id;
    UT_hash_handle hh;
} user_t;
#endif

#ifdef CUSTOM_TEST
typedef struct
{
    char a;
    int b;
} custom_key_t;

typedef struct
{
    custom_key_t key;
    /* ... other data ... */
    UT_hash_handle hh;
} custom_t;
#endif

int main()
{
#ifdef BASIC_TEST
    int i;
    user_t *user, *tmp, *users = NULL;
    /* create elements */
    for (i = 0; i < 10; i++)
    {
        user = (user_t *)malloc(sizeof(user_t));
        if (user == NULL)
        {
            exit(-1);
        }
        user->id = i;
        user->cookie = i * i;
        HASH_ADD_INT(users, id, user);
    }
    for (user = users; user != NULL; user = (user_t *)(user->hh.next))
    {
        printf("user %d, cookie %d\n", user->id, user->cookie);
    }
    printf("------------------\n");
    /* delete each even ID */
    for (i = 0; i < 10; i += 2)
    {
        HASH_FIND_INT(users, &i, tmp);
        if (tmp != NULL)
        {
            HASH_DEL(users, tmp);
            free(tmp);
        }
        else
        {
            printf("user id %d not found\n", i);
        }
    }
    for (user = users; user != NULL; user = (user_t *)(user->hh.next))
    {
        printf("user %d, cookie %d\n", user->id, user->cookie);
    }
#endif

#ifdef STRING_TEST
    const char **n, *names[] = {"joe", "bob", "betty", NULL};
    user_t *s, *tmp, *users = NULL;
    int i = 0;

    for (n = names; *n != NULL; n++)
    {
        s = (user_t *)malloc(sizeof(user_t));
        if (s == NULL)
            exit(-1);
        strcpy(s->name, *n);
        s->id = i++;
        HASH_ADD_STR(users, name, s);
    }
    HASH_FIND_STR(users, "betty", s);
    if (s != NULL)
    {
        printf("betty's id is %d\n", s->id);
    }
    /* free the hash table contents */
    HASH_ITER(hh, users, s, tmp)
    {
        HASH_DEL(users, s);
        free(s);
    }
#endif

#ifdef CUSTOM_TEST
    custom_t *p, *r, *tmp, *customs = NULL;
    // custom_t l;
    custom_key_t q;

    r = (custom_t *)malloc(sizeof(custom_t));
    if (!r)
        exit(-1);
    memset(r, 0, sizeof(custom_t));
    r->key.a = 'a';
    r->key.b = 1;
    HASH_ADD(hh, customs, key, sizeof(custom_key_t), r);

    // memset(&l, 0, sizeof(custom_t));
    // l.key.a = 'a';
    // l.key.b = 1;

    // memset is important because of struct auto align!!!
    memset(&q, 0, sizeof(custom_key_t));
    q.a = 'a';
    q.b = 1;

    p = NULL;
    // HASH_FIND(hh, customs, &l.key, sizeof(custom_key_t), p);
    HASH_FIND(hh, customs, &q, sizeof(custom_key_t), p);
    if (p)
        printf("found %c %d\n", p->key.a, p->key.b);

    HASH_ITER(hh, customs, p, tmp)
    {
        HASH_DEL(customs, p);
        free(p);
    }
#endif

    return 0;
}
