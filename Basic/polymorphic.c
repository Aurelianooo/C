#include <stdio.h>

/* Offset of member MEMBER in a struct of type TYPE. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

/* container_of 宏 - 通过成员指针找到所属结构体的指针 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* 便利宏：从对象获取基类指针，需指定类型和成员名 */
#define ANIMAL(type, obj) \
    ((struct animal *)((char *)&(obj) + offsetof(type, base)))

/* ----- 基类接口定义 ----- */
struct animal
{
    void (*make_sound)(struct animal *self); /* 虚函数指针 */
};

/* 通用接口调用函数 */
void animal_make_sound(struct animal *a)
{
    a->make_sound(a);
}

/* ----- 派生类：猫 ----- */
typedef struct cat
{
    struct animal base; /* 嵌入基类 */
    const char *name;
}Cat;

void cat_sound(struct animal *self)
{
    struct cat *this = container_of(self, struct cat, base);
    printf("%s: meow\n", this->name);
}

struct cat create_cat(const char *name)
{
    struct cat c;
    c.base.make_sound = cat_sound;
    c.name = name;
    return c;
}

/* ----- 派生类：狗 ----- */
typedef struct dog
{
    int id; /* animal 不是第一个成员，container_of 允许
                基类嵌在结构体任意位置（甚至可以嵌入多个），灵活 */
    struct animal base;
    const char *name;
}Dog;

void dog_sound(struct animal *self)
{
    struct dog *this = container_of(self, struct dog, base);
    printf("%s (id=%d): woof\n", this->name, this->id);
}

struct dog create_dog(int id, const char *name)
{
    struct dog d;
    d.id = id;
    d.base.make_sound = dog_sound;
    d.name = name;
    return d;
}

int main(void)
{
    Cat c = create_cat("pussy");
    Dog d = create_dog(42, "doggy");

    /* 统一通过基类指针调用，实际执行的是各自的虚函数 */
    animal_make_sound(ANIMAL(Cat, c));
    animal_make_sound(ANIMAL(Dog, d));

    return 0;
}
