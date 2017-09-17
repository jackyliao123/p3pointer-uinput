#include <cmath>
#include <tuple>
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

using namespace std;
struct vec2 {
    double x, y;

    vec2 operator + (const vec2 &v) {
        return vec2{x+v.x, y+v.y};
    }
    vec2 operator - (const vec2 &v) {
        return vec2{x-v.x, y-v.y};
    }

    double operator * (const vec2 &v) {
        return x*v.x+y*v.y;
    }

    double operator ^ (const vec2 &v) {
        return x * v.y - y * v.x;
    }

	vec2 operator - () {
		return vec2{-x, -y};
	}

    double lenSq() {
        return x*x+y*y;
    }

    double len() {
        return sqrt(lenSq());
    }

    vec2 scale(double fact) {
        return {x * fact, y * fact};
    }

    vec2 normalize() {
    	return scale(1.0 / len());
    }
};
struct vec3 {
    double x, y, z;

    vec3 operator + (const vec3 &v) {
        return vec3{x+v.x, y+v.y, z+v.z};
    }
    vec3 operator - (const vec3 &v) {
        return vec3{x-v.x, y-v.y, z-v.z};
    }

    double operator * (const vec3 &v) {
        return x*v.x+y*v.y+z*v.z;
    }

    vec3 operator ^ (const vec3 &v) {
        return vec3{y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }

    double lenSq() {
        return x*x+y*y+z*z;
    }

    double len() {
        return sqrt(lenSq());
    }

    vec3 scale(double fact) {
        return {x * fact, y * fact, z * fact};
    }

    vec3 normalize() {
    	return scale(1.0 / len());
    }

};

struct plane {
    double p[4];
    plane(double d1,double d2, double d3, double d4) {
        tie(p[0],p[1],p[2],p[3]) = make_tuple(d1,d2,d3,d4);
    }
    double operator [] (int i) {
        return p[i];
    }
};

vec3 projPlane(plane eq, vec3 p, vec3 orig) {
	vec3 v = p - orig;
	vec3 n = {eq[0], eq[1], eq[2]};
	double dist = v*n;
	vec3 res = p-n.scale(dist);
	return p-n.scale(dist);
}

vec2 proj2d(plane eq, vec3 p, vec3 orig, vec3 fixedPoint) {
	vec3 projectedP = projPlane(eq, p, orig);
	vec3 fp1 = (projPlane(eq, fixedPoint, orig) - orig).normalize();
	vec3 fp2 = fp1 ^ (vec3){eq[0], eq[1], eq[2]};
	return {fp1*projectedP, fp2*projectedP};
}

vec3 intersectLinePlane(plane eq, vec3 a, vec3 b) {
	double t = -(eq[0] * b.x + eq[1] * b.y + eq[2] * b.z + eq[3]) / (eq[0] * (a.x - b.x) + eq[1] * (a.y - b.y) + eq[2] * (a.z - b.z));
	if(t < 0 || t > 1) {
		return {NAN, NAN, NAN};
	}
	return a.scale(t) + b.scale(1 - t);
}

vec2 intersectLineSegment(vec2 la, vec2 lb, vec2 sa, vec2 sb) {
	double a = la.y - lb.y;
	double b = lb.x - la.x;
	double c = -(a * la.x + b * la.y);
	double t = -(a * sb.x + b * sb.y + c) / (a * (sa.x - sb.x) + b * (sa.y - sb.y));
	if(t < 0 || t > 1) {
		return {NAN, NAN};
	}
	return sa.scale(t)+sb.scale(1 - t);
}

plane getPlane(vec3 norm, vec3 pnt) {
    return plane(norm.x,norm.y,norm.z,-(norm*pnt));
}

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
	udev.absmin[ABS_X] = 0;
	udev.absmin[ABS_Y] = 0;
	udev.absmin[ABS_PRESSURE] = 0;

	udev.absmax[ABS_X] = 32767;
	udev.absmax[ABS_Y] = 32767;
	udev.absmax[ABS_PRESSURE] = 255;

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

vec2 mapToRegion(vec2 p[4], vec2 t) {
	vec2 n0 = p[3] - p[0];
	n0 = ((vec2){n0.y, -n0.x}).normalize();
	vec2 n3 = p[2] - p[3];
	n3 = ((vec2){n3.y, -n3.x}).normalize();
	vec2 n2 = p[2] - p[1];
	n2 = ((vec2){-n2.y, n2.x}).normalize();
	vec2 n1 = p[1] - p[0];
	n1 = ((vec2){-n1.y, n1.x}).normalize();
	double u = (t - p[0]) * n0 / ((t - p[0]) * n0 + (t - p[2]) * n2);
	double v = (t - p[0]) * n1 / ((t - p[0]) * n1 + (t - p[3]) * n3);
	/*double a = n0.x;
	double b = n0.y;
	double c = -p[0] * n0;
	double d = n0.x + n2.x;
	double e = n0.y + n2.y;
	double f = -p[0] * n0 - p[2] * n2;
	double g = n1.x;
	double h = n1.y;
	double i = -p[0] * n1;
	double j = n1.x + n3.x;
	double k = n1.y + n3.y;
	double l = -p[0] * n1 - p[2] * n3;

	double uda = u * (d - a);
	double ueb = u * (e - b);
	double ufc = u * (f - c);
	double vjg = v * (j - g);
	double vkh = v * (k - h);
	double vli = v * (l - i);

	printf("== %f %f %f %f %f %f %f %f\n", uda, ueb, ufc, vjg, vkh, vli);

	double x = (vkh * ufc - vli * ueb) / (vjg * ueb - vkh * uda);
	double y = (vli * uda - ufc * vjg) / (vjg * ueb - vkh * uda);

	printf("=== %.20lf %.20f\n", x, y);

	return vec2{x, y};*/

	return vec2{u, v};

}

int main() {

	int rd;
	int nrd;
	int ev_size = sizeof(struct input_event);

	struct input_event ev[64];

	int out_fd;

	if((out_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
		error("error: open /dev/uinput");

	init_device(out_fd);

	vec2 calibg[4];

//    double x,y,z;
//    scanf("%lf %lf %lf",&x,&y,&z);
//    vec3 v = vec3{x,y,z};
//    plane p = getPlane(v,v.normalize());
	int button;
	int calibrating = 0;
	int prevButton = 0;
	bool calib = true;
    for(double d1,d2,d3;;) {
		if(calib) {
			printf("Calibrating: %d\n", calibrating);
		}
        scanf("%d %lf %lf %lf",&button,&d1,&d2,&d3);
		vec2 pl = {-d2 / d1, -d3 / d1};
		if(calib) {
			if(button && !prevButton) {
				calibg[calibrating++] = pl;
			}
			if(calibrating == 4) {
				calib = false;
			}
			prevButton = button;
			continue;
		}
		if(button && !prevButton) {
			send_event(out_fd, EV_KEY, BTN_LEFT, 1);
			send_event(out_fd, EV_SYN, SYN_REPORT, 1);
		} else if(!button && prevButton) {
			send_event(out_fd, EV_KEY, BTN_LEFT, 0);
			send_event(out_fd, EV_SYN, SYN_REPORT, 1);
		}
//        vec2 res = proj2d(p,vec3{d1,d2,d3},v.normalize(),vec3{1,d2/d1,d3/d1});
//        printf("<vec2 res:%.6lf %.6lf>\n",res.x,res.y);
		for(int i = 0; i < 4; ++i) {
			printf("%d: %10.6f %10.6f\n", i, calibg[i].x, calibg[i].y);
		}
		printf("Before: %10.6f %10.6f\n", pl.x, pl.y);
		pl = mapToRegion(calibg, pl);
		printf("%10.6f %10.6f\n", pl.x, pl.y);
		int x = (int)(pl.x * 32767);
		int y = (int)(pl.y * 32767);
		send_event(out_fd, 1, 330, 1);
		send_event(out_fd, EV_ABS, 0, x);
		send_event(out_fd, EV_ABS, 1, y);
		send_event(out_fd, EV_ABS, 24, 255);
		send_event(out_fd, EV_SYN, SYN_REPORT, 1);
		prevButton = button;
    }
	
	return 0;
}
