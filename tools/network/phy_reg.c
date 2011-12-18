#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <linux/mii.h>
#include <linux/sockios.h>

#include <net/if.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#define READING 0
#define WRITING 1

void use()
{
    fprintf(stderr, "\nphy [OPTIONS]\n\n");
    fprintf(stderr, "  -a address (register within phy to access)\n");
    fprintf(stderr, "  -d device (something like eth0, eth1, etc.)\n");
    fprintf(stderr, "  -r (read the register)\n");
    fprintf(stderr, "  -w (write the register)\n");
    fprintf(stderr, "  -v value (the value to write)\n");
    fprintf(stderr, "  -h (this help message)\n");
}

int
main(int argc, char *argv[])
{
    int             fd;
    struct ifreq    request;
//    struct mii_ioctl_data data;
	uint16_t *data;
    int             opt;
    int             dir = READING;
    char            *pDev = "eth0";
    uint32_t        addr = 0;
    uint32_t        value = 0;

    while((opt = getopt(argc, argv, "a:d:hrv:w")) != -1) {
        switch(opt) {
            case 'a':
                addr = strtoul(optarg, NULL, 16);
                break;

            case 'd':
                pDev = optarg;
                break;

            case 'h':
                use();
                exit(0);

            case 'r':
                dir = READING;
                break;

            case 'v':
                value = strtoul(optarg, NULL, 16);
                break;

            case 'w':
                dir = WRITING;
                break;

            default:
                fprintf(stderr, "bad option\n");
                use();
                exit(1);
        }
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stderr, "no sock\n");
        exit(1);
    }

    memset(&request, 0, sizeof(request));
    memset(&data, 0, sizeof(data));
    strncpy(request.ifr_name, pDev, sizeof(request.ifr_name));
	data = (uint16_t *)(&request.ifr_data);
//    data[0] = 1;
    data[1] = addr; 
//	request.ifr_data = (__caddr_t)&data;

    if(dir == READING) {
        if(ioctl(fd, SIOCGMIIREG, &request) < 0) {
            fprintf(stderr, "SIOCGMIIREG failed\n");
            exit(1);
        }

//        printf("%d: %04x\n", addr, data.val_out);
	printf("%d: %04x\n", addr, data[3]);
    } else {
//        data.val_in = value;
	data[2] = value;
        if(ioctl(fd, SIOCSMIIREG, &request) < 0) {
            fprintf(stderr, "SIOCSMIIREG failed\n");
            exit(1);
        }
    }

    exit(0);
}

