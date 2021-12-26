#include "con_helper.h"

void con_clear() {
    system("cls");
}
void con_setxy(int x, int y)
{
    COORD c;
    c.X = x; 
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);    
}
void con_setcolor(int color)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

WORD get_key(char *key_char)
{
    INPUT_RECORD irInBuf[128] = { 0 };
    DWORD cNumRead = 0;
    WORD key = 0;
    if (PeekConsoleInput(
        GetStdHandle(STD_INPUT_HANDLE),      // input buffer handle
        irInBuf,     // buffer to read into
        128,         // size of read buffer
        &cNumRead))
        for (WORD i = 0; i < cNumRead; i++)
        {
            switch (irInBuf[i].EventType)
            {
            case KEY_EVENT:
                if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                    key = irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
                    *key_char = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
                }
                break;
            default: break;
            }
        }
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    return key;
}
