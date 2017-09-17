#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdint.h>

#define max(a, b) a > b ? a : b
#define min(a, b) a < b ? a : b

#define error(str) {\
	printf("%s\n", str);\
	exit(0);\
}\

void init_device(int fd) {
	struct uinput_user_dev udev;

	if(ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
		error("error: ioctl UI_SET_EVBIT EV_SYN");

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		error("error: ioctl UI_SET_EVBIT EV_KEY");
	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0)
		error("error: ioctl UI_SET_KEYBIT");
//	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN) < 0)
//		error("error: ioctl UI_SET_KEYBIT");
//	if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS) < 0)
//		error("error: ioctl UI_SET_KEYBIT");
//	if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2) < 0)
//		error("error: ioctl UI_SET_KEYBIT");
	
	if(ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
		error("error: ioctl UI_SET_EVBIT EV_ABS");
	if(ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0)
		error("error: ioctl UI_SET_ABSBIT ABS_X");
	if(ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
		error("error: ioctl UI_SET_ABSBIT ABS_Y");
	if(ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE) < 0)
		error("error: ioctl UI_SET_ABSBIT ABS_PRESSURE");

	memset(&udev, 0, sizeof(udev));
	snprintf(udev.name, UINPUT_MAX_NAME_SIZE, "Absolute Trackpad");
	udev.id.bustype = BUS_VIRTUAL;
	udev.id.vendor = 0x1;
	udev.id.product = 0x1;
	udev.id.version = 1;
	udev.absmin[ABS_X] = 1574 + 400;
	udev.absmin[ABS_Y] = 1355 + 400;
	udev.absmin[ABS_PRESSURE] = 0;

	udev.absmax[ABS_X] = 5386 - 400;
	udev.absmax[ABS_Y] = 4499 - 400;
	udev.absmax[ABS_PRESSURE] = 255;

//	udev.absmin[ABS_X] -= udev.absmax[ABS_X] - udev.absmin[ABS_X];

	if(write(fd, &udev, sizeof(udev)) < 0)
		error("error: write uinput_user_dev");

	if(ioctl(fd, UI_DEV_CREATE) < 0)
		error("error: ioctl UI_DEV_CREATE");

}

void send_event(int device, int type, int code, int value) {
	struct input_event ev;
	ev.type = type;
	ev.code = code;
	ev.value = value;
	if(write(device, &ev, sizeof(ev)) < 0)
		error("error: write to device failed");
}

int main() {

	int in_fd;
	int rd;
	int nrd;
	int ev_size = sizeof(struct input_event);
	char in_name[256] = "Unknown";

	uint8_t in_pressure = 0;
	
	struct input_event ev[64];

	int out_fd;

	if((in_fd = open("/dev/input/event11", O_RDONLY)) < 0)
		error("error: open /dev/input/event11");

	ioctl(in_fd, EVIOCGNAME (sizeof(in_name)), in_name);
	printf("Input: %s\n", in_name);
	
	if((out_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
		error("error: open /dev/uinput");

	init_device(out_fd);

	while(1) {
		if((rd = read(in_fd, ev, ev_size * 64)) < ev_size)
			error("error: not enough data read");
		nrd = rd / ev_size;
		for(int i = 0; i < nrd; ++i) {
			if(ev[i].type == 3) {
				switch(ev[i].code) {
					case 0: case 1: case 24:
						send_event(out_fd, ev[i].type, ev[i].code, ev[i].value);
						break;
					default:
						send_event(out_fd, ev[i].type, ev[i].code, ev[i].value);
						printf("%d %d\n", ev[i].code, ev[i].value);
						break;
				}
			} else if(ev[i].type == 1 && ev[i].code == 330) {
				send_event(out_fd, ev[i].type, ev[i].code, ev[i].value);
				printf("%d %d\n", ev[i].code, ev[i].value);
			}
		}
		send_event(out_fd, EV_SYN, SYN_REPORT, 1);
		//printf("\n");
	}
	
	return 0;
}
