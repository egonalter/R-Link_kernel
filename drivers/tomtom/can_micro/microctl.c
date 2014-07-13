/*
 * Test program for can micro driver
 *
 * To keep the driver open during a debugging session do:
 *
 * $ exec 3</dev/micro		# Open the file on shell's fd #3
 * $ microctl <command1> <&3	# Do some microctl commands ...
 * ...
 * $ microctl <commandn> <&3
 * $ exec 3<- 			# Close fd #3
 * 
 */
#include <sys/ioctl.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>

#include "../../../include/linux/can_micro.h"

#define MONITOR_PERIOD 1 /* seconds */

struct {
	bool verbose;
	int dev_fd;
} ctx = {
	.dev_fd = STDIN_FILENO,
};

#define V(fmt, args...) { \
	if (ctx.verbose) \
		printf("V: " fmt, ## args); \
}

typedef int (*micro_handler_t)(char *cmd);

#define DELAY() usleep(100000) /* 100 ms */

int sync_handler     (char *cmd);
int updatefw_handler (char *cmd);
int power_handler    (char *cmd);
int monitor_handler  (char *cmd);

static struct {
	char *cmd;
	micro_handler_t handler;
}
cmds[] = {
	{
		.cmd = "powerup",
		.handler = power_handler,
	},
	{
		.cmd = "powerdown",
		.handler = power_handler,
	},
	{
		.cmd = "updatefw",
		.handler = power_handler,
	},
	{
		.cmd = "sync",
		.handler = sync_handler,
	},
	{
		.cmd = "monitor",
		.handler = monitor_handler,
	},
};
#define NUM_CMDS (sizeof(cmds)/sizeof(cmds[0]))

static int __power_mode(int m)
{
	int r = 0;

	V("Powering %s Can microcontroller\n", m ? "on" : "off");
	if (ioctl(ctx.dev_fd, IO_PWR_5V_ON_MCU, !!m) == -1) {
		warn("Couldn't switch on 5V Can micro power");
		r = -1;
	}
	if (ioctl(ctx.dev_fd, IO_MCU_nPWR_ON, !m) == -1) {
		warn("Couldn't switch on Can micro power");
		r = -1;
	}
	
	return r;
}
#define power_on() __power_mode(1)
#define power_off() __power_mode(0)

static int __reset_mode(int m)
{
	int r = 0;

	V("%sing reset of Can microcontroller\n", m ? "hold" : "releas");
	if (ioctl(ctx.dev_fd, IO_nCAN_RST, !m) == -1) {
		warn("Couldn't modify Can micro reset");
		r = -1;
	}
	
	return r;
}
#define reset_hold() __reset_mode(1)
#define reset_release() __reset_mode(0)

static int __set_bootmode(int m)
{
	int r = 0;

	V("Setting %s boot mode of Can microcontroller\n", m ? "update" : "normal");
	if (ioctl(ctx.dev_fd, IO_CAN_BT_MD, m) == -1) {
		warn("Couldn't modify Can micro reset");
		r = -1;
	}
	
	return r;
}
#define bootmode_normal() __set_bootmode(0)
#define bootmode_update() __set_bootmode(1)

static int get_reserve()
{
	int v;

	V("Reading reserve pin state\n");
	if (ioctl(ctx.dev_fd, IO_CAN_RES, &v) == -1) {
		warn("Couldn't get Can micro reserve pin state");
		v = -1;
	}
	
	return v;
}

static int get_nreset()
{
	int v;
	V("Reading nreset pin state\n");
	if (ioctl(ctx.dev_fd, IO_CAN_nRST, &v) == -1) {
		warn("Couldn't get Can micro nreset pin state");
		v = -1;
	}
	
	return v;
}

static int power_up()
{
	int r = 0;

	r = reset_hold();
	DELAY();
	r |= power_on();
	DELAY();
	r |= reset_release();

	return r;
}

static int power_down()
{
	int r = 0;

	r = reset_hold();
	DELAY();
	r |= power_off();

	return r;
}

static int micro_sync()
{
	int r = 0;

	V("Sending sync pulse to Can microcontroller\n");
	if (ioctl(ctx.dev_fd, IO_CAN_SYNC, 0) == -1) {
		warn("Couldn't sync Can micro");
		r = -1;
	}
	
	return r;
}

int power_handler(char *cmd)
{
	int r=0;

	if (strcmp(cmd, "powerdown") == 0)
		goto pdown;

	if (( (strcmp(cmd, "updatefw") == 0 && !bootmode_update()) ||
	      !bootmode_normal()) &&
            !power_up()) {
		return 0;
	}

	r = 1;
	DELAY();
pdown:
	r |= power_down();
	
	return r;
}

int sync_handler(char *cmd)
{
	return micro_sync();
}

int monitor_handler(char *cmd)
{
	int flags;

	void sigio_handler(int signum)
	{
		printf("Received SIGIO\n");
	}
	//signal(SIGIO, sigio_handler);

	flags = fcntl(ctx.dev_fd, F_GETFL);
	if (flags == -1)
		err(EXIT_FAILURE, "Couldn't get fd %d flags", ctx.dev_fd);
	if (fcntl(ctx.dev_fd, F_SETFL, flags | FASYNC) == -1)
		err(EXIT_FAILURE, "Couldn't set fd %d flags", ctx.dev_fd);

	while (1) {
		sleep(MONITOR_PERIOD);
		printf("nRESET = %d, RESERV = %d", get_nreset(), get_reserve());
	}

	if (fcntl(ctx.dev_fd, F_SETFL, flags) == -1)
		err(EXIT_FAILURE, "Couldn't set fd %d flags", ctx.dev_fd);
	//signal(SIGIO, SIG_DFL);

	return 0;
}

static void usage()
{
	int i;

	printf("Usage: microctl [-v] <action> < /dev/<microdev> | microctl -h\n"
	       " -v : verbose\n"
	       " action is one of:\n"
	);

	for(i = 0; i<NUM_CMDS; i++)
		printf(" - %s\n", cmds[i].cmd);
}

int main(int argc, char *argv[])
{
	int i, o;

	while ((o = getopt(argc, argv, "hnv")) != -1) {
		switch (o) {
			case 'v':
				ctx.verbose = true;
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			default :
				goto err;
		}
	}

	if (optind >= argc) {
		puts("Error: Must specify a command.");
		goto err;
	}

	if (optind < argc-1) {
		puts("Error: Trailing garbage.");
		goto err;
	}

	for(i=0; i<NUM_CMDS; i++) {
		if (strcmp(cmds[i].cmd, argv[optind]) == 0) {
			V("cmd = %s\n", cmds[i].cmd);
			if (cmds[i].handler(argv[optind])) {
				err(EXIT_FAILURE, "Command %s failed\n", cmds[i].cmd);
			}
			exit(EXIT_SUCCESS);
		}
	}

err:
	usage();
	exit(EXIT_FAILURE);
}

/* EOF */

