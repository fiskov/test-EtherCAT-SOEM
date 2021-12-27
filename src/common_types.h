#ifndef __COMMMON_TYPES_H
#define __COMMMON_TYPES_H

#include <windows.h>

#define VERSION 02

#define MENU_MODE_TMR 1000

typedef enum {
	MENU_SLE,
	MENU_PORT,
	MENU_VALUE,
	MENU_EXIT
} menu_pos_t;
typedef enum {
	MENU_OUT_NORMAL,
	MENU_OUT_ALL,
	MENU_OUT_NONE,
	MENU_OUT_BLINK_SINGLE,
	MENU_OUT_BLINK_ALL,
	MENU_OUT_ALL_SLE,
	MENU_OUT_NONE_SLE
} menu_out_mode_t;

typedef struct {
	menu_pos_t menu_pos, menu_pos_old;
	WORD key;
	char key_char;
	int pos_slave, pos_port;
	int pos_out, value2, mode_tmr;
	uint8_t value;
	menu_out_mode_t mode;
} menu_t;

#endif /*__COMMMON_TYPES_H */