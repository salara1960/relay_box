#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <math.h>


//-----------------------------------------------------------------------
#define buf_size 512
#define SPEED B115200
#define max_cmd 3
#define max_rel 4

#define get_timer_sec(tm) (time(NULL) + tm)
#define check_delay_sec(tm) (time(NULL) >= tm ? 1 : 0)

//-----------------------------------------------------------------------
const char *cmd[] = {
    "SET_ON",
    "SET_OFF",
    "GET_STAT"
};
//---------------------------------------------------------------------
char *ThisTime()
{
time_t ct = time(NULL);
char *arba = ctime(&ct);

    arba[strlen(arba)-1] = 0;//remove '\n'

    return (arba);
}
//-----------------------------------------------------------------------
int main (int argc, char *argv[])
{
unsigned char to_dev[buf_size];
char ack[128] = {0};
char vrem[128] = {0};
char from_dev[buf_size];
char dev_name[128] = {0};
char chap[1024];
int fd, Vixod = 0, lenr = 0, lenr_tmp = 0, ukb = 0, i = 0, ik, rdy = 0, res, cmd_id = -1, rel, tm;
struct timeval mytv;
fd_set Fds;
char *uks = NULL, *uke = NULL;
unsigned char rl[max_rel] = {0}, stat = 0, bt;
unsigned int tmr[max_rel] = {0};
unsigned char tsm[max_rel] = {0};
struct termios oldtio, newtio;
uint32_t seq_num_cmd = 0;
time_t ct = time(NULL);
char *arba = ctime(&ct);

    arba[strlen(arba)-1] = 0;

    if (argc<2) {
        printf ("%s | ERROR: you must enter 1 param. For example: ./relay /dev/ttyUSB0\n\n", arba);
        return 1;
    }

    strcpy(dev_name, argv[1]);

    printf("%s | Start RS232 relay_box for device %s\n", arba, dev_name);

    fd = open(dev_name, O_RDWR , 0664);
    if (fd < 0) {
	printf("Can't open %s file (%d)\n", dev_name, fd);
	return 1;
    }

    tcgetattr(fd, &oldtio);
    bzero(&newtio, sizeof(newtio));
    memcpy(&newtio, &oldtio, sizeof(oldtio));
    newtio.c_cflag = SPEED | CS8 | CLOCAL | CREAD;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    while (!Vixod) {

	FD_ZERO(&Fds); FD_SET(fd, &Fds);
	mytv.tv_sec = 0; mytv.tv_usec = 25000;
	if (select(fd + 1, &Fds, NULL, NULL, &mytv) > 0) {
	    if (FD_ISSET(fd, &Fds)) {// event from my device
		//----------- read data from device ------------------
		lenr_tmp = read(fd, &from_dev[ukb], 1);
		if (lenr_tmp > 0) {
		    lenr += lenr_tmp;
		    ukb += lenr_tmp;
		    if (lenr >= buf_size-1) rdy = 1;
		    else
		    uks = strstr(from_dev,"\r\n");
		    if (uks) {
			*uks = '\0';
			lenr = strlen(from_dev);
			rdy = 1;
		    }
		}
		if (rdy) {
		    rdy = 0; cmd_id = -1; rel = 0; tm = 0;
		    printf("data from device (%d): %.*s | ", lenr, lenr, from_dev);
		    for (i=0; i<lenr; i++) printf("%02X ",(unsigned char)from_dev[i]);
		    printf("\n");
		    for (i = 0; i < max_cmd; i++) {
			if (strstr(from_dev, cmd[i])) {
			    cmd_id = i;
			    break;
			}
		    }
		    if (cmd_id != -1) {
			uks = &from_dev[strlen(cmd[cmd_id])];
			if (*uks == ' ') uks++;
			uke = strchr(uks, ' ');
			if (!uke) uke = strchr(uks, '\0');
			if (uke) {
			    memset(vrem, 0, sizeof(vrem));
			    memcpy(vrem, uks, uke - uks);
			    rel = atoi(vrem);//relay
			    //if (rel > max_rel) rel = 0;
			    if (*uke == ' ') {
				uks = uke + 1;
				uke = strchr(uks, '\0');
				if (uke) tm = atoi(uks);
			    }
			}
		    }
		    memset(ack, 0, sizeof(ack));
		    res = sprintf(ack,"%s : ERROR\r\n", from_dev);
		    if (rel <= max_rel) {
			switch (cmd_id) {//command type
			    case 0://SET_ON
			    case 1://SET_OFF
				if (rel > 0) {
				    res = sprintf(ack,"%s : OK\r\n", from_dev);
				    if (!cmd_id) {//SET_ON
					rl[rel-1] = 1;
					tsm[rel-1] = tm;
					if (tm > 0) tmr[rel-1] = get_timer_sec(tm);
					       else tmr[rel-1] = 0;
				    } else {//SET_OFF
					rl[rel-1] = 0;
					tmr[rel-1] = 0;
				    }
				}
			    break;
			    case 2://GET_STAT
				if (rel > 0) {
				    res = sprintf(ack,"%s : %d\r\n", from_dev, rl[rel-1]);
				} else {
				    stat = 0; i = max_rel - 1;
				    for (i = 0; i < max_rel; i++) {
					bt = rl[i] & 1;
					bt <<= i;
					stat |= bt;
				    }
				    res = sprintf(ack,"%s : %02X\r\n", from_dev, stat);
				}
			    break;
			}
		    }
		    memcpy(to_dev, ack, res);
		    seq_num_cmd++;
		    printf("[%u] cmd=%d rel=%d tm=%d\n", seq_num_cmd, cmd_id, rel, tm);
		    if (write(fd, to_dev, res) == res)
			sprintf(chap,"to_device send %d bytes:%s", res, ack);
		    else
			sprintf(chap,"Error sending to device %d bytes:%s", res, strerror(errno));
		    printf("%s\n", chap);
		    sprintf(chap,"relay status :");
		    for (ik = max_rel - 1; ik >= 0; ik--) sprintf(chap+strlen(chap), " %u", rl[ik]);
		    printf("%s\n\n", chap);

		    memset(from_dev, 0, buf_size);
		    ukb = 0; lenr = lenr_tmp = 0;
		}
	    }
	}
	for (i = 0; i < max_rel; i++) {//check timer[i] is done
	    if (tmr[i] > 0) {//if timer was setup for relay[i]
		if (check_delay_sec(tmr[i])) {//if timeer is done -> relay OFF
		    rl[i] = 0;
		    tmr[i] = 0;
		    sprintf(chap, "relay %d set to 0 by timer (%d sec.) => relay new status :", i+1, tsm[i]);
		    for (ik = max_rel - 1; ik >= 0; ik--) sprintf(chap+strlen(chap), " %u", rl[ik]);
		    printf("%s\n\n", chap);
		}
	    }
	}
	//---------------------------------------------------
    }

    tcsetattr(fd, TCSANOW, &oldtio);
    if (fd) close(fd);

    return 0;
}
