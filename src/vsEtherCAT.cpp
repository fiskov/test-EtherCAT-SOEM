/** \file
 * \brief Example code for Simple Open EtherCAT master
 */

#include <stdio.h>
#include <string.h>
 //#include <Mmsystem.h>

#include "osal.h"
#include "ethercat.h"
#include "common_types.h"
#include "con_helper.h"

#include <windows.h>
#include <conio.h>
#include <ctime>

#define EC_TIMEOUTMON 500

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
volatile int wkc;
volatile int rtcnt;
boolean inOP;
uint8 currentgroup = 0;

menu_t menu;
HANDLE hStdin;
DWORD fdwSaveOldMode;
static char alarm_msg[70] = "";

void print_menu(void);

int get_max_reg(uint8_t sle_no, uint8_t port, int *offset, uint8_t * SLE_type) {
    *offset = 0;
    int res = 0;
    *SLE_type = ec_slave[sle_no].name[4] - '0';

    if (port == 0) {
        res = (*SLE_type == 2) ? 64 : 128;
    }
    else {
        *offset = (*SLE_type == 1 || *SLE_type == 3) ? 128 : 256;
        res = (*SLE_type == 2 || *SLE_type == 3) ? 64 : 128;
    }
    return res;
}

void fill_SLE_port(uint8_t sle_no, uint8_t port, bool enable) {
    int offset = (port ? 128 : 0);
    uint8_t SLE_type = ec_slave[sle_no].name[4] - '0';

    if (port == 0)
    {
        if (SLE_type == 1 || SLE_type == 3)
            memset(ec_slave[sle_no].outputs, (enable ? menu.value : 0), 128);
        if (SLE_type == 2)
            for (int j = 0; j < 128; j++) *(uint32_t*)(ec_slave[sle_no].outputs + j * 4) =
                (enable ? menu.value2 : 0);
    } else {
        if (SLE_type == 1)
            memset(&ec_slave[sle_no].outputs[128], enable ? menu.value : 0, 128);
        if (SLE_type == 2)
            for (int j = 0; j < 128; j++) *(uint32_t*)(ec_slave[sle_no].outputs + j * 4 + 64*4) =
                (enable ? menu.value2 : 0);
        if (SLE_type == 3)
            for (int j = 0; j < 64; j++) *(uint32_t*)(ec_slave[sle_no].outputs + j * 4 + 128) =
                (enable ? menu.value2 : 0);
    }
}
void fill_SLE_all(bool enable) {
    for (int i = 1; i <= ec_slavecount; i++) {
        fill_SLE_port(i, 0, enable);
        fill_SLE_port(i, 1, enable);
    }
}


/* most basic RT thread for process data, just does IO transfer */
void CALLBACK RTthread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    IOmap[0]++;

    ec_send_processdata();
    wkc = ec_receive_processdata(EC_TIMEOUTRET);
    rtcnt++;
    /* do RT control stuff here */

    if (--menu.mode_tmr < 0) menu.mode_tmr = MENU_MODE_TMR;

    switch (menu.mode)
    {
    case MENU_OUT_ALL:
        fill_SLE_port(menu.pos_slave, menu.pos_port, true);
        break;
    case MENU_OUT_NONE:
        fill_SLE_port(menu.pos_slave, menu.pos_port, false);
        break;
    case MENU_OUT_BLINK_ALL:
        fill_SLE_port(menu.pos_slave, menu.pos_port, (menu.mode_tmr > MENU_MODE_TMR / 2));
        break;
    case MENU_OUT_ALL_SLE:
        fill_SLE_all(true);
        break;
    case MENU_OUT_NONE_SLE:
        fill_SLE_all(false);
        break;
    case MENU_OUT_BLINK_SLE:
        fill_SLE_all((menu.mode_tmr > MENU_MODE_TMR / 2));
        break;
    case MENU_OUT_BLINK_SINGLE:
        if (menu.mode_tmr == MENU_MODE_TMR)
            menu.pos_out++;
    default:
        fill_SLE_port(menu.pos_slave, menu.pos_port, false);
        int offset; uint8_t SLE_type;
        int max_reg = get_max_reg(menu.pos_slave, menu.pos_port, &offset, &SLE_type);
        int width = (max_reg == 64 ? 4 : 1);

        if (menu.pos_out >= max_reg) menu.pos_out = 0;

        if (max_reg != 64)
            *(ec_slave[menu.pos_slave].outputs + menu.pos_out + offset) = menu.value;
        else
            *(uint32_t*)(ec_slave[menu.pos_slave].outputs + menu.pos_out * 4 + offset) = menu.value2;

        break;
    }
            

}

void simpletest(char* ifname)
{
    int i, wkc_count, chk;
    UINT mmResult;

    inOP = FALSE;
    clock_t begin = clock();
    printf("Starting Ethercat test. v%02d\n", VERSION);

    /* initialise SOEM, bind socket to ifname */
    if (ec_init(ifname))
    {
        printf("ec_init on %s succeeded.\n", ifname);
        /* find and auto-config slaves */

        if (ec_config_init(FALSE) > 0)
        {
            printf("%d slaves found and configured.\n", ec_slavecount);
            Sleep(100);
            printf("Config map. Please wait...\n");
            ec_config_map(&IOmap);

            ec_configdc();

            printf("\nSlaves mapped, state to SAFE_OP.\n");
            /* wait for all slaves to reach SAFE_OP state */
            ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

            //oloop = ec_slave[0].Obytes;
            //if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
            //if (oloop > 8) oloop = 8;
            //iloop = ec_slave[0].Ibytes;
            //if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
            //if (iloop > 8) iloop = 8;

            //printf("segments : %d : %d %d %d %d\n", ec_group[0].nsegments, ec_group[0].IOsegment[0], ec_group[0].IOsegment[1], ec_group[0].IOsegment[2], ec_group[0].IOsegment[3]);

            printf("Request operational state for all slaves\n");
            expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
            printf("Calculated workcounter %d\n", expectedWKC);
            ec_slave[0].state = EC_STATE_OPERATIONAL;
            /* send one valid process data to make outputs in slaves happy*/
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);

            /* start RT thread as periodic MM timer */
            mmResult = timeSetEvent(1, 0, RTthread, 0, TIME_PERIODIC);

            /* request OP state for all slaves */
            ec_writestate(0);
            chk = 200;
            /* wait for all slaves to reach OP state */
            do
            {
                ec_statecheck(0, EC_STATE_OPERATIONAL, 100000);
            } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
            if (ec_slave[0].state == EC_STATE_OPERATIONAL)
            {
                printf("Operational state reached for all slaves.\n");
                for (int i = 1; i <= ec_slavecount; i++) 
                    printf("%s | ", ec_slave[i].name);
                printf("\n");
                Sleep(100);
                con_clear();

                wkc_count = 0;
                inOP = TRUE;
                clock_t end = clock();
                double elapsed_secs = double(1.0*end - begin) / CLOCKS_PER_SEC;
                sprintf_s(alarm_msg, "connection time = %6.2f s", elapsed_secs);

                /* cyclic loop, reads data from RT thread */
                //for (i = 1; i <= 500; i++)
                while (menu.menu_pos != MENU_EXIT)
                {
                    print_menu();
                    if (wkc >= expectedWKC)
                    {          
                        /*
                        printf("Processdata cycle %4d, WKC %d , O:", rtcnt, wkc);

                        for (j = 0; j < oloop; j++)
                        {
                            printf(" %2.2x", *(ec_slave[0].outputs + j));
                        }

                        printf(" I:");
                        for (j = 0; j < iloop; j++)
                        {
                            printf(" %2.2x", *(ec_slave[0].inputs + j));
                        }
                        printf(" T:%lld\r", ec_DCtime);
                        */
                    }
                    osal_usleep(50000);

                }
                inOP = FALSE;
            }
            else
            {
                printf("Not all slaves reached operational state.\n");
                ec_readstate();
                for (i = 1; i <= ec_slavecount; i++)
                {
                    if (ec_slave[i].state != EC_STATE_OPERATIONAL)
                    {
                        printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                            i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
                system("PAUSE");
            }

            /* stop RT thread */
            timeKillEvent(mmResult);

            printf("\nRequest init state for all slaves\n");
            ec_slave[0].state = EC_STATE_INIT;
            /* request INIT state for all slaves */
            ec_writestate(0);
        }
        else
        {
            printf("\nNo slaves found!\n");
            system("PAUSE");
        }
        printf("End simple test, close socket\n");
        /* stop SOEM, close socket */
        ec_close();
    }
    else
    {
        printf("No socket connection on %s\nExcecute as root\n", ifname);
    }
}

//DWORD WINAPI ecatcheck( LPVOID lpParam )
OSAL_THREAD_FUNC ecatcheck(void* lpParam)
{
    int slave;
    
    while (1)
    {
        if (inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
                if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
                {
                    ec_group[currentgroup].docheckstate = TRUE;
                    if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                    {
                        ec_configdc();
                        sprintf_s(alarm_msg, "ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                        ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                        ec_writestate(slave);
                    }
                    else if (ec_slave[slave].state == EC_STATE_SAFE_OP)
                    {
                        ec_configdc();
                        sprintf_s(alarm_msg, "WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                        ec_slave[slave].state = EC_STATE_OPERATIONAL;
                        ec_writestate(slave);
                    }
                    else if (ec_slave[slave].state > EC_STATE_NONE)
                    {
                        if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                        {
                            ec_slave[slave].islost = FALSE;
                            ec_configdc();
                            sprintf_s(alarm_msg, "MESSAGE : slave %d reconfigured\n", slave);
                        }
                    }
                    else if (!ec_slave[slave].islost)
                    {
                        /* re-check state */
                        ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                        if (ec_slave[slave].state == EC_STATE_NONE)
                        {
                            ec_slave[slave].islost = TRUE;
                            ec_configdc();
                            sprintf_s(alarm_msg, "ERROR : slave %d lost\n", slave);
                        }
                    }
                }
                if (ec_slave[slave].islost)
                {
                    if (ec_slave[slave].state == EC_STATE_NONE)
                    {
                        if (ec_recover_slave(slave, EC_TIMEOUTMON))
                        {
                            ec_slave[slave].islost = FALSE;
                            ec_configdc();
                            sprintf_s(alarm_msg, "MESSAGE : slave %d recovered\n", slave);
                        }
                    }
                    else
                    {
                        ec_slave[slave].islost = FALSE;
                        ec_configdc();
                        ec_recover_slave(slave, EC_TIMEOUTMON);
                        sprintf_s(alarm_msg, "MESSAGE : slave %d found\n", slave);
                    }
                }
            }
            if (!ec_group[currentgroup].docheckstate)
                sprintf_s(alarm_msg, "OK : all slaves resumed OPERATIONAL.\n");
        }

        osal_usleep(10000);
    }

    //return 0;
}

char ifbuf[1024];
char adapters_name[20][80] = { 0 };

#define X_SLAVES 0
#define X_PORTS (X_SLAVES + 11)
#define X_INPUT  (X_PORTS + 11)
#define X_OUTPUT (X_INPUT + 10*3 + 7)
#define X_ALARM (X_PORTS)
#define X_POS (X_OUTPUT + 10*3 + 14)

#define Y_SLAVES 3
#define Y_PORTS  (Y_SLAVES)
#define Y_INPUT  (Y_SLAVES)
#define Y_OUTPUT (Y_SLAVES)
#define Y_ALARM 22
#define Y_POS (1)

void print_menu(void)
{
    static bool isStart = true;
    if (isStart) {
        isStart = false;
        memset(&menu, 0, sizeof(menu_t));
        menu.pos_slave = 1;
        menu.value = 0xFF;
        menu.value2 = 0xF47F223D;
    }

    menu.key = get_key(&menu.key_char);

    con_setxy(0, 0);
    printf("Test EtherCAT. v%02d",VERSION);
    // adapter
    con_setxy(0, 1);
    printf("Adapter: %s", ifbuf);

    // slaves
    con_setxy(X_SLAVES, Y_SLAVES);
    printf("Slaves: ");
    for (int i = 1; i <= min(ec_slavecount, 25 - Y_SLAVES); i++)
    {
        con_setxy(X_SLAVES, Y_SLAVES + i);
        con_setcolor(i == menu.pos_slave ? 
            (menu.menu_pos == MENU_SLE && menu.mode_tmr++ & 1 ? 0x9F : 0x1F) : 0x7);
        printf("%7s  ", ec_slave[i].name);
    }

    // ports
    if (menu.menu_pos == MENU_PORT || menu.menu_pos == MENU_VALUE)
    {
        con_setxy(X_PORTS, Y_PORTS);
        con_setcolor(0x7);
        printf("Bus:");
        for (int i = 0; i < 2; i++) {
            con_setxy(X_PORTS, Y_PORTS + 1 + i);
            con_setcolor(menu.pos_port == i ? 
                (menu.menu_pos == MENU_PORT && menu.mode_tmr++ & 1 ? 0x9F : 0x1F) : 0x7);
            printf("port%c", 'A'+i);
        }
    }

    // input 
    uint8_t SLE_type = ec_slave[menu.pos_slave].name[4] - '0';
    int offset = (menu.pos_port ? 128 : 0);
    if (menu.menu_pos == MENU_VALUE)
    {
        con_setxy(X_INPUT, Y_INPUT);
        con_setcolor(0x7);
        printf(" Input: ");
        for (int i = 0; i < 13; i++)
        {
            con_setxy(X_INPUT - 4, Y_INPUT + 1 + i);
            printf("%3d|", i * 10);
        }
        for (int i = 0; i < 128; i++)
        {
            con_setxy(X_INPUT + (i % 10) * 3, Y_INPUT + 1 + i / 10);

            uint32_t value = *(ec_slave[menu.pos_slave].inputs + i + offset);
            int color = 0x7;
            if (i & 1) color |= 0x8;
            if (value == 0xFF) color |= 0x20; else
                if (value) color |= 0x10;
            con_setcolor(color);
            printf(" %02X", value);
        }
    }
    // output
    if (menu.menu_pos == MENU_VALUE)
    {
        bool isLong = (SLE_type == 2 || (SLE_type == 3 && menu.pos_port == 1));
        con_setxy(X_OUTPUT, Y_OUTPUT);
        con_setcolor(0x7);
        printf(" Output: ");
        for (int i = 0; i < 13; i++)
        {
            con_setxy(X_OUTPUT - 5, Y_OUTPUT + 1 + i);
            printf("|%3d|", i * (isLong ? 5 : 10));
        }

        // 1-byte SLE01, SLE03_portA
        if (SLE_type == 1 || (SLE_type == 3 && menu.pos_port == 0) )
            for (int i = 0; i < 128; i++)
            {
                con_setxy(X_OUTPUT + (i % 10) * 3, Y_OUTPUT + 1 + i / 10);

                uint8_t value = *(ec_slave[menu.pos_slave].outputs + i + offset);
                con_setcolor(value ? 0xE0 : (i & 1 ? 0x8 : 0) | 0x7);
                printf(" %02X", value);
            }
        if (SLE_type == 2 || (SLE_type == 3 && menu.pos_port == 1))
            for (int i = 0; i < 64; i++)
            {
                con_setxy(X_OUTPUT + (i % 5) * 9, Y_OUTPUT + 1 + i / 5);

                uint32_t value = *(uint32_t*)(ec_slave[menu.pos_slave].outputs +
                    i * sizeof(uint32_t) + offset);
                con_setcolor(value ? 0xE0 : (i & 1 ? 0x8 : 0) | 0x7);
                printf(" %08X", value);
            }
    }
    // alarm
    if (strlen(alarm_msg) > 0)
    {
        con_setxy(X_ALARM, Y_ALARM);
        con_setcolor(0xE);
        printf("%70s", alarm_msg);
    }


    // control
    con_setxy(X_POS, Y_POS);
    con_setcolor(0x7);
    printf("Pos=%3d", menu.pos_out);

    con_setxy(X_POS, Y_POS+2);
    con_setcolor(0x7);
    if (SLE_type == 2 || (SLE_type == 3 && menu.pos_port == 1))
        printf("Value=%08X", menu.value2);
    else
        printf("Value=%02X", menu.value);

    // end
    con_setxy(X_ALARM, Y_ALARM - 3);
    con_setcolor(0x7);

    switch (menu.menu_pos) {
    case MENU_SLE:
        printf("Press 'UP', 'DOWN', 'ENTER' for select SLE, 'F1' = enable all, 'F2' = dinable all, 'F3' = blink all");
        {
            int pos = menu.pos_slave;
            if (menu.key == VK_UP || menu.key == VK_LEFT) pos--;
            if (menu.key == VK_DOWN || menu.key == VK_RIGHT) pos++;
            if (pos < 1) pos = ec_slavecount;
            if (pos > ec_slavecount) pos = 1;
            menu.pos_slave = pos;
        }

        if (menu.key == VK_RETURN) menu.menu_pos = MENU_PORT;
        if (menu.key == VK_ESCAPE) menu.menu_pos = MENU_EXIT;
        if (menu.key == VK_F1) menu.mode = MENU_OUT_ALL_SLE;
        if (menu.key == VK_F2) menu.mode = MENU_OUT_NONE_SLE;
        if (menu.key == VK_F3) menu.mode = MENU_OUT_BLINK_SLE;

        break;
    case MENU_PORT:
        printf("Press 'UP', 'DOWN', 'ENTER' for select PORT, 'ESC' for back to SLE");
        if ((menu.key == VK_UP || menu.key == VK_LEFT) && menu.pos_port > 0) menu.pos_port--;
        if ((menu.key == VK_DOWN || menu.key == VK_RIGHT) && menu.pos_port < 1) menu.pos_port++;
        if (menu.key == VK_RETURN) menu.menu_pos = MENU_VALUE;
        if (menu.key == VK_ESCAPE) menu.menu_pos = MENU_SLE;
        break;
    case MENU_VALUE:
        printf("Press 'UP' = all enable, 'DOWN' = all disable, 'LEFT' = pos-1, 'RIGHT' = pos+1,"
            "'ESC' for back to PORT");
        con_setxy(X_ALARM, Y_ALARM - 1);
        printf("'+' = blink all, '-' = auto++, 'F1' = enable all SLE, 'F2' = dinable all SLE, 'F3' = blink all SLE");
        con_setxy(X_ALARM, Y_ALARM - 2);
        printf("Press '0123456789abcdef' for edit value");
        {
            int pos = menu.pos_out;
            int pos_max = 127;
            if (SLE_type == 2 || (SLE_type == 3 && menu.pos_port == 1)) pos_max = 63;
            if (menu.key == VK_LEFT) {
                menu.mode = MENU_OUT_NORMAL;
                pos--;
            }
            if (menu.key == VK_RIGHT) {
                menu.mode = MENU_OUT_NORMAL;
                pos++;
            }
            if (pos > pos_max) pos = 0;
            if (pos < 0) pos = pos_max;
            menu.pos_out = pos;
        }
        if (menu.key == VK_UP) menu.mode = MENU_OUT_ALL;
        if (menu.key == VK_DOWN) menu.mode = MENU_OUT_NONE;
        if (menu.key == VK_ESCAPE) menu.menu_pos = MENU_PORT;
        if (menu.key == VK_F1) menu.mode = MENU_OUT_ALL_SLE;
        if (menu.key == VK_F2) menu.mode = MENU_OUT_NONE_SLE;
        if (menu.key == VK_F3) menu.mode = MENU_OUT_BLINK_SLE;

        if ((menu.key_char >= '0' && menu.key_char <= '9') || (menu.key_char >= 'a' && menu.key_char <= 'f')) {
            char c = menu.key_char & 0xFF;
            if (c >= '0' && c <= '9') c = c-'0';
            if (c >= 'a' && c <= 'f') c = c-'a'+10;  
            if (SLE_type == 1 || (SLE_type == 3 && menu.pos_port == 0))
                menu.value = ((menu.value << 4) | c);
            else
                menu.value2 = ((menu.value2 << 4) | c);
        }
        if (menu.key_char == '+') menu.mode = MENU_OUT_BLINK_ALL;
        if (menu.key_char == '-') menu.mode = MENU_OUT_BLINK_SINGLE;

        break;
    }
    if (menu.key)
    {
        if (menu.menu_pos_old != menu.menu_pos) con_clear();
        menu.menu_pos_old = menu.menu_pos;

        menu.key = 0;
    }
    menu.key_char = 0;

    con_setxy(X_ALARM, Y_ALARM+1);    
    printf("T:%19lld  wkc=%3d", ec_DCtime, wkc);
}

void start_ethercat_thread(const char* name)
{
    /* create thread to handle slave error handling in OP */
    osal_thread_create(&thread1, 128000, &ecatcheck, (void*)&ctime_s);
    strcpy_s(ifbuf, name);
    /* start cyclic part */
    simpletest(ifbuf);
}
int main(int argc, char* argv[])
{
    ec_adaptert* adapter = NULL;
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwSaveOldMode);
    SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT);

    con_clear();
    printf("SOEM (Simple Open EtherCAT Master). Simple test v%02d\n", VERSION);

    if (argc > 1)
    {
        start_ethercat_thread(argv[1]);
    }
    else
    {
        printf("Use: %s \\device\\npf_{xxxxxx}\n", argv[0]);
        /* Print the list */
        printf("Select network adapters:\n");
        adapter = ec_find_adapters();

        int n = 0;
        while (adapter != NULL)
        {
            printf("%d : %40s = %s\n", n+1, adapter->desc, adapter->name);
            strcpy_s(adapters_name[n], adapter->name);
            n++;
            adapter = adapter->next;
        }
        printf("\nSelect adtapter [1..%d]: ", n);
        int pos = _getch() - '1';
        if (pos >= 0 && pos <= n)
        {            
            printf("\nSelected %d = %s\n", pos+1, adapters_name[pos]);
            start_ethercat_thread(adapters_name[pos]);
        }
        else {
            printf("\n\nIncorrect number!!!\n\n");
            system("PAUSE");
        }        
    }
    printf("End program\n");
    SetConsoleMode(hStdin, fdwSaveOldMode);
    return (0);
}
