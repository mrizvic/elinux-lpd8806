/*
 * lpd8806_speedtest.c
 *
 * Copyright 2012 Christopher De Vries
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt
 */
#include <unistd.h>
#include <stdio.h>
#include "lpd8806led.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

static const char *device = "/dev/spidev0.0";
static const int leds = 260;
static const int frames = 10000;

int main(int argc, char *argv[]) {
  lpd8806_buffer buf;
  int fd;
  int ret;
  int i, j;
  int color=0;
  lpd8806_color *p;
  set_gamma(2.5,2.5,2.5);

  if(ret==-1) {
    fprintf(stderr,"Error %d: %s\n",errno, strerror(errno));
    exit(1);
  }

  fd = open(device,O_WRONLY);
  if(fd<0) {
    fprintf(stderr, "Can't open device.\n");
    exit(1);
  }

  ret=spi_init(fd);
  if(ret==-1) {
    fprintf(stderr,"error=%d, %s\n", errno, strerror(errno));
    exit(1);
  }

  lpd8806_init(&buf,leds);

  unsigned int iseed = (unsigned int)time(NULL);
  srand (iseed);

  while(42) {
	i=random() % leds;

	j=random() % 3;

	switch (j) {
		case 0: { write_gamma_color(&buf.pixels[i], 0xff,0xff,0xff); break; }
		case 1: { write_gamma_color(&buf.pixels[i], 0x80,0xff,0x80); break; }
		case 2: { write_gamma_color(&buf.pixels[i], 0xff,0x80,0x80); break; }
		default: break;
	}
	//printf("%d ",j);
	send_buffer(fd,&buf);

	j = random() % 30000 + 3000;
	usleep(j);

	write_gamma_color(&buf.pixels[i], 0,0,0);
	send_buffer(fd,&buf);

	j = random() % 20000 + 2000;
	usleep(j);

  }

  lpd8806_free(&buf);
  close(fd);
  return 0;
}
