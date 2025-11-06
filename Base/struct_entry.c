#include <stdio.h>
#include <string.h>

#define struct_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
/**Linus edition*
 * container_of - 通过结构体的一个成员获取容器结构体的指针
 * @ptr: 指向成员的指针。
 * @type: 成员所嵌入的容器结构体类型。
 * @member: 结构体中的成员名。
 */
/* #define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
}) */

#define container_of(ptr, type, member) ({             \
    const void *__mptr = (ptr);                        \
    (type *)((char *)__mptr - offsetof(type, member)); \
})


#define UNUSED(x) (void)(x)

#define NAME_LEN 64
#define TEL_LEN 16

typedef enum Gender
{
    MALE,
    FEMALE
} Gender;

typedef struct Student
{
    int age;
    char *name;
    char *tel;
    Gender gender;
} Student;

typedef struct Teacher
{
    int age;
    char name[NAME_LEN];
    char tel[TEL_LEN];
    Gender gender;
} Teacher;

int main()
{
    Student stu;
    stu.age = 10;
    stu.name = "Zhang San";
    stu.tel = "12345678";
    stu.gender = MALE;

    Student *p = &stu;
    printf("stu address:%p\n", p);
    char **q = &stu.tel;
    printf("stu.tel address:%p\n", q);

    p = NULL;
    // p = struct_entry(q, Student, tel);
    p = container_of(q, Student, tel);
    printf("pointer p:%p\n", p);

    printf("\n");

    Teacher tea;
    tea.age = 42;
    strcpy(tea.name, "Li Si");
    strcpy(tea.tel, "000000000");
    tea.gender = FEMALE;

    Teacher *m = &tea;
    printf("tea address:%p\n", m);

    char (*n)[TEL_LEN] = &tea.tel;
    // UNUSED(n);
    m = NULL;
    m = container_of(n, Teacher, tel);
    printf("pointer m:%p\n", m);

    return 0;
}
