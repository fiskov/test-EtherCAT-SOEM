#ifndef __COMMMON_TYPES_H
#define __COMMMON_TYPES_H

typedef enum {
	MENU_SLE,
	MENU_PORT,
	MENU_VALUE,
	MENU_EXIT
} menu_pos_t;

typedef struct {
	menu_pos_t menu_pos, menu_pos_old;
	WORD key;
	char key_char;
	int pos_slave, pos_port;
	int pos_out, value;
	bool all_enable, all_disable;
} menu_t;

#endif /*__COMMMON_TYPES_H */