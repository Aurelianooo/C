#include<stdio.h>
#define int8_t unsigned char

void set_symbol(int8_t *reg_value, int8_t symbol_value, int8_t symbol_len, int8_t symbol_shift);

int main(void){
	int8_t reg = 0b00000000;
	printf("before %d\n", reg);
	set_symbol(&reg, 7, 3, 3);
	printf("after %d\n", reg);
	return 0;
}

void set_symbol(int8_t *reg_value, int8_t symbol_value, int8_t symbol_len, int8_t symbol_shift){
	int8_t i;
	for(i=0;i<symbol_len;i++){
		*reg_value &= ~(1<<symbol_shift+i);
	}
	*reg_value |= symbol_value<<symbol_shift;
}

