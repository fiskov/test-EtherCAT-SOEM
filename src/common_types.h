#ifndef __COMMMON_TYPES_H
#define __COMMMON_TYPES_H

#include <windows.h>

#define MENU_MODE_TMR 1000

typedef enum {
	MENU_SLE,
	MENU_PORT,
	MENU_VALUE,
	MENU_EXIT
} menu_pos_t;
typedef enum {
	MENU_OUT_NORMAL,
	MENU_OUT_ALL_ENABLE,
	MENU_OUT_ALL_DISABLE,
	MENU_OUT_BLINK_SINGLE,
	MENU_OUT_BLINK_ALL,
} menu_out_mode_t;
typedef enum {
	MENU_MODE_ALL_SLE,
	MENU_MODE_SINGLE_SLE,
	MENU_MODE_SINGLE_PORT
} menu_mode_t;
typedef struct {
	menu_pos_t menu_pos, menu_pos_old;
	WORD key;
	char key_char;
	int pos_slave, pos_port;
	int pos_out, value, value2, mode_tmr;
	menu_out_mode_t mode;
} menu_t;

#endif /*__COMMMON_TYPES_H */