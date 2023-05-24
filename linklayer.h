#ifndef LINKLAYER
#define LINKLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fun_aux.h"
#include <stdbool.h>

typedef struct linkLayer
{
    char serialPort[50];
    int role; // defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
} linkLayer;

// ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1
#define BAUDRATE B9600

// SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 250

// CONNECTION deafault values
#define BAUDRATE_DEFAULT B9600
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

// MISC
#define FALSE 0
#define TRUE 1

int llopen(linkLayer connectionParameters);
int llwrite(char *buf, int bufSize);
int llread(char *packet);
int llclose(linkLayer connectionParameters, int showStatistics);

int llopen(linkLayer connectionParameters)
{
    alarm_type = 0;
    struct termios oldtio, newtio;
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        return -1;
    }
    sleep(1);
    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    sleep(1);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    printf("New termios structure set\n");

    n_tries = connectionParameters.numTries;
    timeout = connectionParameters.timeOut;

    if (connectionParameters.role == 0)
    {
        send_SET();
    }
    if (connectionParameters.role == 1)
    {
        send_UA();
    }
    sleep(1);
    return 1;
}
int llclose(linkLayer connectionParameters, int showStatistics)
{
    alarm_type = 2;
    if (connectionParameters.role == 0)
    {
        send_DISC_em(showStatistics);
    }
    if (connectionParameters.role == 1)
    {
        send_DISC_re(showStatistics);
    }
    close(fd);
    return 1;
}
int llwrite(char *buf, int bufSize)
{
    int res;

    unsigned char newBuff[MAX_PAYLOAD_SIZE * 2];
    unsigned char buf1[255];
    flag_write = !flag_write;
    if (flag_write)
    {
        BCC2(buf, newBuff, bufSize);
        printf("\n BCC2: %x \n", newBuff[bufSize]);
        int size = byte_stuffing(newBuff, bufSize + 1);
        send_info(newBuff, size, I0);
        while (1)
        {
            res = read(fd, buf1, 1);
            if (res > 0)
            {
                maquina_estados_RR_REJ(buf1);
            }
            if (c_state_r == STOPM_RR1)
            {
                alarm(0);
                aux2 = 0;
                printf("RR1 RECEBIDO\n");
                c_state_r = START_R;
                break;
            }
            if (c_state_r == STOPM_REJ0)
            {
                alarm(0);
                aux2 = 0;
                printf("REJ0 RECEBIDO\n");
                flag_write = !flag_write;
                c_state_r = START_R;
                llwrite(buf, bufSize);
                break;
            }
        }
    }
    else
    {
        BCC2(buf, newBuff, bufSize);
        printf("\n BCC2: %x \n", newBuff[bufSize]);
        int size = byte_stuffing(newBuff, bufSize + 1);
        send_info(newBuff, size, I1);
        while (1)
        {
            res = read(fd, buf1, 1);
            if (res > 0)
            {
                maquina_estados_RR_REJ(buf1);
            }
            if (c_state_r == STOPM_RR0)
            {
                alarm(0);
                aux4 = 0;
                printf("RR0 RECEBIDO\n");
                c_state_r = START;
                break;
            }
            if (c_state_r == STOPM_REJ1)
            {
                alarm(0);
                aux4 = 0;
                printf("REJ1 RECEBIDO\n");
                c_state_r = START_R;
                flag_write = !flag_write;
                llwrite(buf, bufSize);
                break;
            }
        }
    }
    return inf_count;
}
int llread(char *packet)
{
    int res;
    unsigned char newBuff[MAX_PAYLOAD_SIZE * 2] = {0};
    unsigned char data1[MAX_PAYLOAD_SIZE * 2] = {0};
    int count_data1 = 0;
    int tam = 0;
    unsigned char BCC2 = 0;
    if (a == 1)
    {
        packet[0] = 0;
        return 1;
    }
    if (i_llread == 0)
    {
        while (1)
        {
            res = read(fd, newBuff, 1);
            if (res > 0)
            {

                maquina_estados_dados(newBuff, data, &count_data);
            }
            if (c_state_d == STOPM_D_0)
            {
                printf("I0 RECEBIDO\n");
                c_state_d = START_D;
                count_data = byte_destuffing(data, count_data);
                BCC2 = 0;
                for (int i = 0; i < count_data - 1; i++)
                {
                    BCC2 ^= data[i];
                }
                if (BCC2 == data[count_data - 1])
                {
                    write(fd, RR1, 5);
                    printf("RR1 enviado\n");
                    while (1)
                    {
                        res = read(fd, newBuff, 1);
                        if (res > 0)
                        {
                            maquina_estados_dados(newBuff, data1, &count_data1);
                        }
                        if (c_state_d == STOPM_D_1)
                        {
                            printf("I1 RECEBIDO\n");
                            c_state_d = START_D;
                            count_data1 = byte_destuffing(data1, count_data1);
                            BCC2 = 0;
                            for (int i = 0; i < count_data1 - 1; i++)
                            {
                                BCC2 = BCC2 ^ data1[i];
                            }
                            if (BCC2 == data1[count_data1 - 1])
                            {
                                for (int i = 0; i < count_data - 1; i++)
                                {
                                    packet[i] = data[i];
                                }
                                memset(data, 0, MAX_PAYLOAD_SIZE * 2 * sizeof(unsigned char));
                                tam = count_data;
                                memcpy(data, data1, count_data1);
                                count_data = count_data1;
                                write(fd, RR0, 5);
                                printf("RR0 enviado\n");
                                i_llread = 1;
                                break;
                            }
                            else
                            {
                                printf("\n BCC2 errado \n");
                                write(fd, REJ1, 5);
                                i_llread = 2;
                                return 0;
                            }
                        }
                        else if (c_state_d == STOPM_D_0)
                        {
                            printf("\nI0 recebido novamente\n");
                            c_state_d = START_D;
                            count_data1 = byte_destuffing(data1, count_data1);
                            BCC2 = 0;
                            for (int i = 0; i < count_data1 - 1; i++)
                            {
                                BCC2 = BCC2 ^ data1[i];
                            }
                            if (BCC2 == data1[count_data1 - 1])
                            {
                                write(fd, RR1, 5);
                                printf("\nRR1 enviado novamente\n");
                                i_llread = 2;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    printf("\n BCC2 errado \n");
                    write(fd, REJ0, 5);
                    memset(data, 0, MAX_PAYLOAD_SIZE * 2 * sizeof(unsigned char));
                    count_data = 0;
                    i_llread = 0;
                    return 0;
                }
                break;
            }
        }
        return tam - 1;
    }
    else if (i_llread == 1)
    {
        while (1)
        {
            res = read(fd, newBuff, 1);
            if (res > 0)
            {
                maquina_estados_dados(newBuff, data1, &count_data1);
            }
            if (c_state_d == STOPM_D_0)
            {
                printf("I0 RECEBIDO\n");
                c_state_d = START_D;
                count_data1 = byte_destuffing(data1, count_data1);
                BCC2 = 0;
                for (int i = 0; i < count_data1 - 1; i++)
                {
                    BCC2 = BCC2 ^ data1[i];
                }
                if (BCC2 == data1[count_data1 - 1])
                {
                    for (int i = 0; i < count_data - 1; i++)
                    {
                        packet[i] = data[i];
                    }
                    if (count_data1 == 2)
                    {
                        a = 1;
                    }
                    printf("\n BCC2 : %x \n", data1[count_data1 - 1]);
                    memset(data, 0, MAX_PAYLOAD_SIZE * 2 * sizeof(unsigned char));
                    tam = count_data;
                    memcpy(data, data1, count_data1);
                    count_data = count_data1;
                    write(fd, RR1, 5);
                    printf("RR1 enviado\n");
                    break;
                }
                else
                {
                    printf("\n BCC2 errado \n");
                    write(fd, REJ0, 5);
                    i_llread = 1;
                    return 0;
                }
            }
            if (c_state_d == STOPM_D_1)
            {
                printf("\nI1 recebido novamente\n");
                c_state_d = START_D;
                count_data1 = byte_destuffing(data1, count_data1);
                BCC2 = 0;
                for (int i = 0; i < count_data1 - 1; i++)
                {
                    BCC2 = BCC2 ^ data1[i];
                }
                printf("\n\n BCC2 : %x \n ", BCC2);
                if (BCC2 == data1[count_data1 - 1])
                {
                    write(fd, RR0, 5);
                    printf("RR0 enviado novamente\n");
                    i_llread = 1;
                    return 0;
                }
            }
        }
        i_llread = 2;
        return tam - 1;
    }
    else if (i_llread == 2)
    {
        while (1)
        {
            res = read(fd, newBuff, 1);
            if (res > 0)
            {
                maquina_estados_dados(newBuff, data1, &count_data1);
            }
            if (c_state_d == STOPM_D_1)
            {
                printf("I1 RECEBIDO\n");
                c_state_d = START_D;
                count_data1 = byte_destuffing(data1, count_data1);
                BCC2 = 0;
                for (int i = 0; i < count_data1 - 1; i++)
                {
                    BCC2 ^= data1[i];
                }
                if (BCC2 == data1[count_data1 - 1])
                {
                    for (int i = 0; i < count_data - 1; i++)
                    {
                        packet[i] = data[i];
                    }
                    if (count_data1 == 2)
                    {
                        a = 1;
                    }
                    printf("\n BCC2 : %x\n", data1[count_data1 - 1]);
                    memset(data, 0, MAX_PAYLOAD_SIZE * 2 * sizeof(unsigned char));
                    tam = count_data;
                    memcpy(data, data1, count_data1);
                    count_data = count_data1;
                    write(fd, RR0, 5);
                    printf("RR0 enviado\n");
                    break;
                }
                else
                {
                    printf("\n BCC2 errado \n");
                    write(fd, REJ1, 5);
                    i_llread = 2;
                    return 0;
                }
            }
            if (c_state_d == STOPM_D_0)
            {
                printf("\nI0 recebido novamente\n");
                c_state_d = START_D;
                count_data1 = byte_destuffing(data1, count_data1);
                BCC2 = 0;
                for (int i = 0; i < count_data1 - 1; i++)
                {
                    BCC2 = BCC2 ^ data1[i];
                }
                if (BCC2 == data1[count_data1 - 1])
                {
                    write(fd, RR1, 5);
                    printf("RR1 enviado novamente\n");
                    i_llread = 2;
                    return 0;
                }
            }
        }
        i_llread = 1;
        return tam - 1;
    }
}
#endif
