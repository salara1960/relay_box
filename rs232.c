
#define LINUX


#ifndef LINUX
    #ifndef _WIN32
        #define _WIN32
    #endif // __WIN32
#endif // LINUX

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <sys/un.h>
    #include <sys/resource.h>
    #include <sys/msg.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <termios.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
#endif

//-----------------------------------------------------------------------

#define buf_size 512 //1024
#define sdef 256
#define SPEED B115200
#define max_cmd 4
#define max_rel 8

//-----------------------------------------------------------------------

#define get_timer_sec(tm) (time(NULL) + tm)
#define check_delay_sec(tm) (time(NULL) >= tm ? 1 : 0)

//-----------------------------------------------------------------------
#ifdef _WIN32
    const char *param = "115200,n,8,1";
    HANDLE fd = NULL;
#else
    #define SPEED B115200
    int fd = -1;
#endif

const char *cmd[] = {
    "SET_ON",
    "SET_OFF",
    "GET_STAT",
    "SET_ALL"
};
//----------------------------------------------------------------------
char *ThisTime()
{
time_t ct = time(NULL);
char *arba = ctime(&ct);

    arba[strlen(arba) - 1] = 0;//remove '\n'

    return (arba);
}
//----------------------------------------------------------------------
int writes(char *data, int len)
{
int ret = -1;

#ifdef _WIN32
    DWORD wr = 0;
    if (WriteFile(fd, data, (DWORD)len, &wr, NULL)) ret = (int)wr;
#else
    ret = write(fd, data, len);
#endif

    return ret;
}
//-----------------------------------------------------------------------
int reads(char *data, int len)
{
#ifdef _WIN32
    DWORD rd = 0;
    if (!ReadFile(fd, data, (DWORD)len, &rd, NULL)) return -1;
    return (int)rd;
#else
    return (read(fd, data, len));
#endif
}
//-----------------------------------------------------------------------
int parse_set_all(char *in, unsigned char *v_buf, unsigned char *t_buf)
{
//X1,Y1 X2,Y2 X3,Y3 X4,Y4 X5,Y5 X6,Y6 X7,Y7 X8,Y8
//'1,0 0,0 0,0 0,0 0,0 0,0 0,0 0,0'
unsigned char v, t;
int k = 0, dl, it, ret = -1;
char *us = NULL, *uke = NULL, *uks = NULL;
char tmp[16] = {0};
char cin[sdef] = {0};


    if (strlen(in) < max_rel*3) return ret;

    memcpy(cin, in, strlen(in));

    uks = cin;
    if (*uks == ' ') uks++;
    while (1) {
        memset(tmp, 0, sizeof(tmp));
        uke = strchr(uks, ' ');
        if (!uke) uke = strchr(uks, '\0');
        if (uke) {
            dl = (uke - uks) & 0x0f;
            if (dl > 0) {
                memcpy(tmp, uks, dl);
                us = strchr(tmp, ',');
                if (us) {
                    it = atoi(us + 1);
                    *us = '\0';
                    if (strchr(tmp, 'X')) {
                        *(v_buf + k) = (unsigned char)255;
                    } else {
                        t = it;
                        v = atoi(tmp);
                        *(v_buf + k) = v;
                        *(t_buf + k) = t;
                    }
                }
            } else break;
        }
        k++;
        if (k < max_rel) uks = uke + 1;
        else {
            ret = 0;
            break;
        }
    }

    return ret;
}
//-----------------------------------------------------------------------
int main (int argc, char *argv[])
{
char to_dev[buf_size];
char ack[buf_size] = {0};
char vrem[sdef] = {0};
char from_dev[buf_size];
char dev_name[sdef] = {0};
char chap[buf_size << 1];
int Vixod = 0, lenr = 0, lenr_tmp = 0, ukb = 0, i = 0, ik, rdy = 0, res, cmd_id = -1, rel, tm;
char *uks = NULL, *uke = NULL;
unsigned char rl[max_rel] = {0}, vbuf[max_rel] = {0}, stat = 0, bt;
unsigned char tsm[max_rel] = {0}, tbuf[max_rel] = {0};
unsigned int tmr[max_rel] = {0};
uint32_t seq_num_cmd = 0;
char *abra = ThisTime();

#ifndef _WIN32
    struct timeval mytv;
    fd_set Fds;
    struct termios oldtio, newtio;
#else
    DCB dcb = {0};
#endif // __WIN32


    if (argc < 2) {
#ifdef _WIN32
        printf ("%s | ERROR: you must enter 1 param. For example: ./relay_box COM1\n\n", abra);
#else
        printf ("%s | ERROR: you must enter 1 param. For example: ./relay_box /dev/ttyUSB0\n\n", abra);
#endif // __WIN32
        return 1;
    }

    strcpy(dev_name, argv[1]);

    printf("%s | Start RS232 relay_box for device %s\n", abra, dev_name);

#ifdef _WIN32
    fd = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (fd < 0) {
        printf("Can't open %s file\n", dev_name);
        return 1;
    }
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!BuildCommDCB(param, &dcb)) {
        printf("%s Can\'t BuildCommDCB() - %s.\n", abra, strerror(errno));
        if (fd > 0) CloseHandle(fd);
        return 1;
    }
    if (!SetCommState(fd, &dcb)) {
        printf("%s Can\'t SetCommState() - %s.\n", abra, strerror(errno));
        if (fd > 0) CloseHandle(fd);
        return 1;
    }
    if (!SetupComm(fd, 2, 2)) {
        printf("%s Can\'t SetupComm() - %s.\n", abra, strerror(errno));
        if (fd > 0) CloseHandle(fd);
        return 1;
    }
#else
    fd = open(dev_name, O_RDWR , 0664);
    if (fd < 0) {
        printf("Can't open %s file\n", dev_name);
        return 1;
    }
    tcgetattr(fd, &oldtio);
    memset(&newtio, 0, sizeof(newtio));
    memcpy(&newtio, &oldtio, sizeof(oldtio));
    cfmakeraw(&newtio);//set RAW mode
    newtio.c_cflag = SPEED | CS8 | CLOCAL | CREAD;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);
#endif



    while (!Vixod) {

#ifndef _WIN32
        FD_ZERO(&Fds); FD_SET(fd, &Fds);
        mytv.tv_sec = 0; mytv.tv_usec = 25000;
        if (select(fd + 1, &Fds, NULL, NULL, &mytv) > 0) {
            if (FD_ISSET(fd, &Fds)) {// event from my device
#else
                usleep(25000);
#endif // __WIN32
                //----------- read data from device ------------------

                lenr_tmp = reads(&from_dev[ukb], 1);

                if (lenr_tmp <= 0) {
                    //Vixod = 1;
                    printf("Can't read from device '%s'\n", dev_name);
                    break;
                } else if (lenr_tmp > 0) {
                    lenr += lenr_tmp;
                    ukb += lenr_tmp;
                    if (lenr >= buf_size-1) rdy = 1;
                    else {
                        uks = strchr(from_dev,'\r');
                        if (!uks) uks = strchr(from_dev,'\n');
                        if (uks) {
                            *uks = '\0';
                            lenr = strlen(from_dev);
                            rdy = 1;
                        }
                    }
                }
                if (rdy) {
                    rdy = rel = 0;
                    cmd_id = -1;
                    tm = 0;
                    printf("data from device (%d): %.*s | ", lenr, lenr, from_dev);
                    for (i = 0; i < lenr; i++) printf("%02X ", (unsigned char)from_dev[i]);
                    printf("\n");
                    for (i = 0; i < max_cmd; i++) {
                        if (strstr(from_dev, cmd[i])) {
                            cmd_id = i;
                            break;
                        }
                    }
                    memset(vrem, 0, sizeof(vrem));
                    if (cmd_id != -1) {
                        uks = &from_dev[strlen(cmd[cmd_id])];
                        if (*uks == ' ') uks++;
                        uke = strchr(uks, ' ');
                        if (!uke) uke = strchr(uks, '\0');
                        if (uke) {
                            if (cmd_id == (max_cmd - 1)) {//"SET_ALL X1,Y1 X2,Y2 X3,Y3 X4,Y4 X5,Y5 X6,Y6 X7,Y7 X8,Y8"
                                strcpy(vrem, uks);
                                rel = tm = -1;
                            } else {//other command
                                memcpy(vrem, uks, uke - uks);
                                rel = atoi(vrem);//relay
                                if (*uke == ' ') {
                                    uks = uke + 1;
                                    uke = strchr(uks, '\0');
                                    if (uke) tm = atoi(uks);
                                }
                            }
                        }
                    }
                    memset(ack, 0, sizeof(ack));
                    res = sprintf(ack, "%s : ERROR\r\n", from_dev);
                    if (rel <= max_rel) {
                        switch (cmd_id) {//command type
                            case 0://SET_ON
                            case 1://SET_OFF
                                if (rel > 0) {
                                    res = sprintf(ack, "%s : OK\r\n", from_dev);
                                    if (!cmd_id) {//SET_ON
                                        rl[rel - 1] = 1;
                                        tsm[rel - 1] = tm;
                                        if (tm > 0) tmr[rel - 1] = get_timer_sec(tm);
                                               else tmr[rel - 1] = 0;
                                    } else {//SET_OFF
                                        rl[rel - 1] = 0;
                                        tmr[rel - 1] = 0;
                                    }
                                }
                            break;
                            case 2://GET_STAT
                                if (rel > 0) {
                                    res = sprintf(ack, "%s : %d\r\n", from_dev, rl[rel-1]);
                                } else {
                                    stat = 0;
                                    i = max_rel - 1;
                                    for (i = 0; i < max_rel; i++) {
                                        bt = rl[i] & 1;
                                        bt <<= i;
                                        stat |= bt;
                                    }
                                    res = sprintf(ack, "%s : %02X\r\n", from_dev, stat);
                                }
                            break;
                            case 3://SET_ALL
                                memcpy(vbuf, rl, max_rel);
                                memcpy(tbuf, tsm, max_rel);
                                if (!parse_set_all(vrem, vbuf, tbuf)) {
                                    for (i = 0; i < max_rel; i++) {
                                        switch (vbuf[i]) {
                                            case 0:
                                                rl[i] = vbuf[i];
                                                tmr[i] = 0;
                                            break;
                                            case 1:
                                                rl[i] = vbuf[i];
                                                tsm[i] = tbuf[i];
                                                if (tsm[i] > 0) tmr[i] = get_timer_sec(tsm[i]);
                                                           else tmr[i] = 0;
                                            break;
                                        }//switch(vbuf[i])
                                    }//for (i=0;....
                                    res = sprintf(ack, "%s : OK\r\n", from_dev);
                                } else res = sprintf(ack, "%s : ERROR\r\n", from_dev);
                            break;
                        }//switch(cmd)
                    }
                    memcpy(to_dev, ack, res);
                    seq_num_cmd++;
                    if (cmd_id == max_rel - 1) {
                        memset(vrem, 0, sizeof(vrem));
                        for (i = 0; i < max_rel; i++) {
                            if (rl[i] == 255)
                                sprintf(vrem+strlen(vrem), " X,%u", tsm[i]);
                            else
                                sprintf(vrem+strlen(vrem), " %u,%u", rl[i], tsm[i]);
                        }
                        printf("[%u] cmd=%d (val,time) :%s\n", seq_num_cmd, cmd_id, vrem);
                    } else printf("[%u] cmd=%d rel=%d tm=%d\n", seq_num_cmd, cmd_id, rel, tm);

                    if (writes(to_dev, res) == res)
                        sprintf(chap,"data to device (%d): %s", res, ack);
                    else
                        sprintf(chap,"Error sending to device %d bytes:%s", res, strerror(errno));
                    printf("%s\n", chap);

                    sprintf(chap, "relay status :");
                    for (ik = max_rel - 1; ik >= 0; ik--) sprintf(chap+strlen(chap), " %u", rl[ik]);
                    printf("%s\n\n", chap);

                    memset(from_dev, 0, buf_size);
                    ukb = 0;
                    lenr = lenr_tmp = 0;
                }//if (rdy)
#ifndef _WIN32
            }//if (FD_ISSET(fd, &Fds))
        }//if (select
#endif // __WIN32
        for (i = 0; i < max_rel; i++) {//check timer[i] is done
            if (tmr[i] > 0) {//if timer was setup for relay[i]
                if (check_delay_sec(tmr[i])) {//if timeer is done -> relay OFF
                    rl[i] = 0;
                    tmr[i] = 0;
                    sprintf(chap, "relay %d set to 0 by timer (%d sec.) => relay new status :", i + 1, tsm[i]);
                    for (ik = max_rel - 1; ik >= 0; ik--) sprintf(chap+strlen(chap), " %u", rl[ik]);
                    printf("%s\n\n", chap);
                }
            }
        }
        //---------------------------------------------------
    }//while (!Vixod)

#ifdef _WIN32
    CloseHandle(fd);
#else
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
#endif

    return 0;
}
