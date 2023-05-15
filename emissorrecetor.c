#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

typedef enum
{
    START,
    F,
    A,
    C,
    BCC,
    STOPM
} Maquina;

Maquina c_state = START;

volatile int STOP = FALSE;
unsigned char SET[5] = {0x5c, 0x01, 0x03, 0x02, 0x5c};
unsigned char UA[5] = {0x5c, 0x01, 0x07, 0x06, 0x5c};

int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int fd;
int aux1 = 0;
void recetor(int fd);
void emissor(int fd);
void maquina_estados(unsigned char *buf1, int fd, unsigned char *aux);
void escrever();

int main(int argc, char **argv)
{
    (void)signal(SIGALRM, escrever);
    // int fd;
    struct termios oldtio, newtio;
    if ((argc < 3) ||
        ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
         (strcmp("/dev/ttyS11", argv[1]) != 0)) ||
        ((strcmp("/recetor", argv[2]) != 0) && (strcmp("/emissor", argv[2]) != 0)))
    {
        printf("Usage:\tnserial SerialPort (Emissor ou recetor)\n\tex: nserial /dev/ttyS1 /emissor\n");
        exit(1);
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }

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

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    // printf("%s", argv[2]);
    printf("New termios structure set\n");

    if (strcmp(argv[2], "/recetor") == 0)
    {
        // printf("ehehehe");
        recetor(fd);
    }
    else if (strcmp(argv[2], "/emissor") == 0)
    {
        emissor(fd);
    }
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
void escrever()
{
    write(fd, SET, 5);
    if (aux1 < 3)
    {
        alarm(3);
    }
    if (aux1 == 3)
    {
        printf("Comunicação não estabelecida\n");
        exit(-1);
    }
    printf("A enviar SET %d\n", aux1 + 1);
    aux1++;
    return;
}

void recetor(int fd)
{
    int res;
    unsigned char buf[255];
    while (STOP == FALSE)
    {
        if (flag1 == 0)
        {
            res = read(fd, buf, 1);
        }
        maquina_estados(buf, fd, SET);
        if (flag3 == 1)
        {
            printf("Tudooooo");
            res = write(fd, UA, 5);
            flag3 = 0;
        }
    }
}
void emissor(int fd)
{
    int res;
    unsigned char buf1[255];
    // write(fd,SET,5);
    escrever();
    while (STOP == FALSE)
    {
        res = read(fd, buf1, 1);
        maquina_estados(buf1, fd, UA);
        if (c_state == STOPM)
        {
            printf("comunicação estabelecida");
            alarm(0);
            c_state = START;
            break;
        }
    }
}
void maquina_estados(unsigned char *buf1, int fd, unsigned char *aux)
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
            flag1 = 1;
        }
        else
            c_state = START;
        break;
    case STOPM:
        printf("Tudo bem :)");
        flag1 = 0;
        flag3 = 1;
        c_state = START;
        break;
    }
    if (flag3 != 1)
    {
        printf("%x\n", buf1[0]);
    }
}