#ifndef FUN_AUX
#define FUN_AUX

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "linklayer.h"

#define MAX_PAYLOAD_SIZE 250

typedef enum
{
    START,
    F,
    A,
    C,
    BCC,
    STOPM
} Maquina;
typedef enum
{
    START_D,
    F_D,
    A_D,
    C_D_0,
    BCC_D_0,
    STOPM_D_0,
    C_D_1,
    BCC_D_1,
    STOPM_D_1
} Maquina_d;

typedef enum
{
    START_R,
    F_R,
    A_R,
    C_RR1,
    C_RR0,
    C_REJ1,
    C_REJ0,
    BCC_RR1,
    BCC_RR0,
    BCC_REJ1,
    BCC_REJ0,
    STOPM_RR1,
    STOPM_RR0,
    STOPM_REJ1,
    STOPM_REJ0
} Maquina_r;

Maquina c_state = START;
Maquina_d c_state_d = START_D;
Maquina_r c_state_r = START_R;

unsigned char SET[5] = {0x5c, 0x01, 0x03, 0x02, 0x5c};
unsigned char UA[5] = {0x5c, 0x01, 0x07, 0x06, 0x5c};
unsigned char DISC[5] = {0x5c, 0x01, 0x0b, 0X01 ^ 0X0b, 0x5c};
unsigned char I0[5] = {0x5c, 0x01, 0x00, 0X01 ^ 0X00, 0x5c};
unsigned char I1[5] = {0x5c, 0x01, 0x02, 0X01 ^ 0X02, 0x5c};
unsigned char RR0[5] = {0x5c, 0x01, 0x01, 0X01 ^ 0X01, 0x5c};
unsigned char RR1[5] = {0x5c, 0x01, 0x21, 0X01 ^ 0X21, 0x5c};
unsigned char REJ1[5] = {0x5c, 0x01, 0x25, 0X01 ^ 0X25, 0x5c};
unsigned char REJ0[5] = {0x5c, 0x01, 0x05, 0X05 ^ 0X01, 0x5c};

int alarm_type = 0, fd = -1;
int aux1 = 0, aux2 = 0, aux3 = 0, aux4 = 0;
int n_tries, timeout;
unsigned char data[MAX_PAYLOAD_SIZE * 2] = {0};
int count_data = 0;
bool flag_write = 0;
int i_llread = 0;
int a = 0;
unsigned char info[MAX_PAYLOAD_SIZE * 2];
int inf_count = 0;

void escrever();
void maquina_estados(unsigned char *buf1, unsigned char *aux);
void send_UA();
void send_SET();
void send_DISC_em(int showStatistics);
void send_DISC_re(int showStatistics);
void maquina_estados_RR_REJ(unsigned char *buf1);
void maquina_estados_dados(unsigned char *buf1, unsigned char *data, int *count_data);
int byte_destuffing(unsigned char *buf, int bufSize);
int byte_stuffing(unsigned char *buf, int bufSize);
void send_info(char *buf, int bufSize, unsigned char *frame);
void BCC2(const unsigned char *buf, unsigned char *newBuff, int bufSize);

void escrever()
{
    (void)signal(SIGALRM, escrever);
    if (alarm_type == 0)
    {
        write(fd, SET, 5);
        if (aux1 < n_tries)
        {
            alarm(timeout);
        }
        if (aux1 == n_tries)
        {
            printf("\nComunicação não estabelecida\n");
            exit(-1);
        }
        printf("A enviar SET %d\n", aux1 + 1);
        aux1++;
    }
    if (alarm_type == 1)
    {
        write(fd, info, inf_count + 5);
        if (aux2 < n_tries)
        {
            alarm(timeout);
        }
        if (aux2 == n_tries)
        {
            printf("\nComunicação não estabelecida\n");
            exit(-1);
        }
        printf("A enviar I0 %d\n", aux2 + 1);
        aux2++;
    }
    if (alarm_type == 2)
    {
        write(fd, DISC, 5);
        if (aux3 < n_tries)
        {
            alarm(timeout);
        }
        if (aux3 == n_tries)
        {
            printf("\nComunicação não estabelecida\n");
            exit(-1);
        }
        printf("A enviar DISC %d\n", aux3 + 1);
        aux3++;
    }
    if (alarm_type == 3)
    {
        write(fd, info, inf_count + 5);
        if (aux4 < n_tries)
        {
            alarm(timeout);
        }
        if (aux4 == n_tries)
        {
            printf("Comunicação não estabelecida\n");
            exit(-1);
        }
        printf("A enviar I1 %d\n", aux4 + 1);
        aux4++;
    }
}
void maquina_estados_RR_REJ(unsigned char *buf1)
{
    switch (c_state_r)
    {
    case START_R:
        if (buf1[0] == 0x5c)
        {
            c_state_r = F_R;
        }
        else
            c_state_r = START_R;
        break;
    case F_R:
        if (buf1[0] == 0x01)
        {
            c_state_r = A_R;
        }
        else
            c_state_r = START_R;

        break;
    case A_R:
        if (buf1[0] == RR1[2])
        {
            c_state_r = C_RR1;
        }
        else if (buf1[0] == RR0[2])
            c_state_r = C_RR0;
        else if (buf1[0] == REJ0[2])
            c_state_r = C_REJ0;
        else if (buf1[0] == REJ1[2])
            c_state_r = C_REJ1;
        else if (buf1[0] == 0x5c)
            c_state_r = START;
        break;
    case C_RR1:
        if (buf1[0] == RR1[3])
        {
            c_state_r = BCC_RR1;
        }
        else
            c_state_r = START_R;
        break;
    case C_RR0:
        if (buf1[0] == RR0[3])
            c_state_r = BCC_RR0;
        else
            c_state_r = START_R;
        break;
    case C_REJ1:
        if (buf1[0] == REJ1[3])
            c_state_r = BCC_REJ1;
        else
            c_state_r = START_R;
        break;
    case C_REJ0:
        if (buf1[0] == REJ0[3])
            c_state_r = BCC_REJ0;
        else
            c_state_r = START_R;
        break;
    case BCC_RR1:
        if (buf1[0] == RR1[4])
        {
            c_state_r = STOPM_RR1;
        }
        else
            c_state_r = START_R;
        break;
    case BCC_RR0:
        if (buf1[0] == RR0[4])
        {
            c_state_r = STOPM_RR0;
        }
        else
            c_state_r = START_R;
        break;
    case BCC_REJ1:
        if (buf1[0] == REJ1[4])
        {
            c_state_r = STOPM_REJ1;
        }
        else
            c_state_r = START_R;
        break;
    case BCC_REJ0:
        if (buf1[0] == REJ0[4])
        {
            c_state_r = STOPM_REJ0;
        }
        else
            c_state_r = START_R;
        break;
    case STOPM_RR1:
        c_state_r = START_R;
        break;
    case STOPM_REJ1:
        c_state_r = START_R;
        break;
    case STOPM_RR0:
        c_state_r = START_R;
        break;
    case STOPM_REJ0:
        c_state_r = START_R;
        break;
    }
}
void maquina_estados(unsigned char *buf1, unsigned char *aux)
{
    switch (c_state)
    {
    case START:
        if (buf1[0] == aux[0])
            c_state = F;
        else
            c_state = START;
        break;
    case F:
        if (buf1[0] == aux[1])
            c_state = A;
        else if (buf1[0] == aux[0])
            c_state = F;
        else
            c_state = START;
        break;
    case A:
        if (buf1[0] == aux[2])
            c_state = C;
        else if (buf1[0] == aux[1])
            c_state = F;
        else
            c_state = START;
        break;
    case C:
        if (buf1[0] == aux[3])
            c_state = BCC;
        else if (buf1[0] == aux[1])
            c_state = F;
        else
            c_state = START;
        break;
    case BCC:
        if (buf1[0] == aux[4])
        {
            c_state = STOPM;
        }
        else
            c_state = START;
        break;
    case STOPM:
        c_state = START;
        break;
    }
}
void BCC2(const unsigned char *buf, unsigned char *newBuff, int bufSize)
{
    unsigned char BCC2 = 0;
    for (int i = 0; i < bufSize; i++)
    {
        newBuff[i] = buf[i];
        BCC2 = BCC2 ^ buf[i];
    }
    newBuff[bufSize] = BCC2;
}
int byte_stuffing(unsigned char *buf, int bufSize)
{
    int tam = 0;
    unsigned char new_Buff[bufSize * 2];
    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == 0x5c)
        {
            new_Buff[tam] = 0X5d;
            tam++;
            new_Buff[tam] = 0X7c;
            tam++;
        }
        else if (buf[i] == 0x5d)
        {
            new_Buff[tam] = 0X5d;
            tam++;
            new_Buff[tam] = 0X7d;
            tam++;
        }
        else
        {
            new_Buff[tam] = buf[i];
            tam++;
        }
    }
    memcpy(buf, new_Buff, tam);
    return tam;
}
int byte_destuffing(unsigned char *buf, int bufSize)
{
    int tam = 0;
    unsigned char new_Buff[bufSize * 2];
    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == 0x5d)
        {
            if (buf[i + 1] == 0x7c)
            {
                new_Buff[tam] = 0X5c;
                tam++;
                i++;
            }
            else if (buf[i + 1] == 0x7d)
            {
                new_Buff[tam] = 0X5d;
                tam++;
                i++;
            }
        }
        else
        {
            new_Buff[tam] = buf[i];
            tam++;
        }
    }
    memcpy(buf, new_Buff, tam);
    return tam;
}
void send_info(char *buf, int bufSize, unsigned char *frame)
{
    inf_count = 0;
    for (int i = 0; i < 4; i++)
    {
        info[i] = frame[i];
    }

    for (int i = 0; i < bufSize; i++)
    {
        info[4 + inf_count] = buf[i];
        inf_count++;
    }

    info[inf_count + 4] = 0x5c;

    if (frame[2] == 0x02)
    {
        alarm_type = 3;
        aux4 = 0;
    }
    if (frame[2] == 0x00)
    {
        alarm_type = 1;
        aux2 = 0;
    }
    escrever();
}
void send_UA()
{
    int res;
    unsigned char buf[255];
    while (1)
    {
        res = read(fd, buf, 1);
        if (res > 0)
        {
            maquina_estados(buf, SET);
        }
        if (c_state == STOPM)
        {
            res = write(fd, UA, 5);
            c_state = START;
            break;
        }
    }
}
void send_SET()
{
    int res;
    unsigned char buf1[255];
    alarm_type = 0;
    escrever();
    while (1)
    {
        res = read(fd, buf1, 1);
        if (res > 0)
        {
            maquina_estados(buf1, UA);
        }
        if (c_state == STOPM)
        {
            alarm(0);
            aux1 = 0;
            printf("UA recebido");
            c_state = START;
            break;
        }
    }
}
void send_DISC_re(int showStatistics)
{
    int res;
    unsigned char buf[255];
    while (1)
    {

        res = read(fd, buf, 1);
        if (res > 0)
        {
            maquina_estados(buf, DISC);
        }
        if (c_state == STOPM)
        {
            c_state = START;
            write(fd, DISC, 5);
            while (1)
            {
                res = read(fd, buf, 1);
                if (res > 0)
                {
                    maquina_estados(buf, UA);
                }
                if (c_state == STOPM)
                {
                    if (showStatistics)
                    {
                        printf("\n UA RECEBIDO \n");
                    }
                    c_state = START;
                    break;
                }
            }
            break;
        }
    }
}
void send_DISC_em(int showStatistics)
{
    int res;
    unsigned char buf1[255];
    alarm_type = 2;
    escrever();
    while (1)
    {
        res = read(fd, buf1, 1);
        if (res > 0)
        {
            maquina_estados(buf1, DISC);
        }
        if (c_state == STOPM)
        {
            alarm(0);
            aux3 = 0;
            if (showStatistics)
            {
                printf("\n DISC RECEBIDO \n");
            }
            c_state = START;
            write(fd, UA, 5);
            break;
        }
    }
}
void maquina_estados_dados(unsigned char *buf1, unsigned char *data, int *count_data)
{
    switch (c_state_d)
    {
    case START_D:
        if (buf1[0] == 0x5c)
        {
            c_state_d = F_D;
        }
        else
            c_state_d = START_D;
        break;
    case F_D:
        if (buf1[0] == 0x01)
        {
            c_state_d = A_D;
        }
        else if (buf1[0] == 0x5c)
            c_state_d = F_D;
        else
            c_state_d = START_D;
        break;
    case A_D:
        if (buf1[0] == 0x00)
        {
            c_state_d = C_D_0;
        }
        else if (buf1[0] == 0x02)
        {
            c_state_d = C_D_1;
        }
        else
            c_state_d = START_D;
        break;
    case C_D_0:
        if (buf1[0] == 0x01)
        {
            c_state_d = BCC_D_0;
        }
        else if (buf1[0] == 0x5c)
        {
            c_state_d = START_D;
        }
        break;
    case BCC_D_0:
        if (buf1[0] == 0x5c)
        {
            c_state_d = STOPM_D_0;
        }
        else
        {
            c_state_d = BCC_D_0;
            data[*count_data] = buf1[0];
            (*count_data)++;
        }
        break;
    case STOPM_D_0:
        c_state_d = START_D;
        break;
    case C_D_1:
        if (buf1[0] == 0x03)
        {
            c_state_d = BCC_D_1;
        }
        else if (buf1[0] == 0x5c)
            c_state_d = START_D;
        break;
    case BCC_D_1:
        if (buf1[0] == 0x5c)
        {
            c_state_d = STOPM_D_1;
        }
        else
        {
            c_state_d = BCC_D_1;
            data[*count_data] = buf1[0];
            (*count_data)++;
        }
        break;
    case STOPM_D_1:
        c_state_d = START_D;
        break;
    }
}
#endif
