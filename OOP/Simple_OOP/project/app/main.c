#include <stdio.h>
#include <string.h>
#include "../../build/config.h"
#include "../animal/animal.h"

ANIMAL_CLASS_IMPLEMENTS *animal_cat = NULL;
ANIMAL_CLASS_IMPLEMENTS *animal_dog = NULL;

static int animal_sound(void *t)
{
    ANIMAL_CLASS_IMPLEMENTS *this = (ANIMAL_CLASS_IMPLEMENTS *)t;
    this->speak(this);
    return 0;
}

int main()
{
    printf(">\n");
    char name[10] = {0};
    Animal_Attr Cat_Attr = {
        .name = CONFIG_ANIMAL_NAME_1,
    };
    Animal_Attr Dog_Attr = {
        .name = CONFIG_ANIMAL_NAME_2,
    };

    // 抽象
    animal_cat = (ANIMAL_CLASS_IMPLEMENTS *)ANIMAL_CLASS_CTOR();
    animal_cat->init(animal_cat, Cat_Attr);
    animal_dog = (ANIMAL_CLASS_IMPLEMENTS *)ANIMAL_CLASS_CTOR();
    animal_dog->init(animal_dog, Dog_Attr);

    // 封装
    memset(name, 0x00, sizeof(name));
    animal_cat->get_name(animal_cat, name);
    printf("animal cat name is:%s\n", name);
    animal_dog->get_name(animal_dog, name);
    printf("animal dog name is:%s\n", name);

    // 继承
    ANIMAL_CLASS_IMPLEMENTS *dog = NULL;
    ANIMAL_CLASS_IMPLEMENTS *cat = NULL;
    cat = (ANIMAL_CLASS_IMPLEMENTS *)CAT_CLASS_CTOR(animal_cat);
    dog = (ANIMAL_CLASS_IMPLEMENTS *)DOG_CLASS_CTOR(animal_dog);
    cat->speak(cat);
    dog->speak(dog);

    // 多态
    animal_sound(cat);
    animal_sound(dog);

    printf("<\n");
    return 0;
}
