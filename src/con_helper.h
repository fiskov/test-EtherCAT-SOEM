#ifndef __CON_HELPER_H
#define __CON_HELPER_H

#include <windows.h>
#include <conio.h>

void con_clear(void);
void con_setxy(int x, int y);
void con_setcolor(int color);
WORD get_key(char *key_char);


#endif 
