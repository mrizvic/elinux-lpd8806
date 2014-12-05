/* This code needs cleanup. It has been ported in november 2014 from https://github.com/FastLED/FastLED/blob/master/examples/Fire2012/Fire2012.ino */

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

static const int sparking = 120;
static const int cooling = 80;

typedef struct {
	int r;
	int g;
	int b;
} CRGB;

// qadd8: add one byte to another, saturating at 0xFF
int qadd8( int i, int j)
{
    int t = i + j;
    if( t > 255) t = 255;
    return t;
}

// qsub8: subtract one byte from another, saturating at 0x00
int qsub8( int i, int j)
{
    int t = i - j;
    if( t < 0) t = 0;
    return t;
}

CRGB HeatColor( int temperature)
{
    CRGB heatcolor;

    // Scale 'heat' down from 0-255 to 0-191,
    // which can then be easily divided into three
    // equal 'thirds' of 64 units each.
    int t192 = scale8_video( temperature, 192);

    // calculate a value that ramps up from
    // zero to 255 in each 'third' of the scale.
    int heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252

    // now figure out which third of the spectrum we're in:
    if( t192 & 0x80) {
        // we're in the hottest third
        heatcolor.r = 255; // full red
        heatcolor.g = 255; // full green
        heatcolor.b = heatramp; // ramp up blue

    } else if( t192 & 0x40 ) {
        // we're in the middle third
        heatcolor.r = 255; // full red
        heatcolor.g = heatramp; // ramp up green
        heatcolor.b = 0; // no blue

    } else {
        // we're in the coolest third
        heatcolor.r = heatramp; // ramp up red
        heatcolor.g = 0; // no green
        heatcolor.b = 0; // no blue
    }

    return heatcolor;
}

int scale8_video( int i, int scale)
{
    uint8_t j = (((int)i * (int)scale) >> 8) + ((i&&scale)?1:0);
    return j;
}

int main(int argc, char *argv[]) {
  lpd8806_buffer buf;
  int fd;
  int ret;
  int i, j;
  int a,b;
  int m,n;
  int k;
  int color=0;
  int heat[leds];
  CRGB myleds[leds];
  int x,y;
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

  x=((cooling*10)/leds) + 2;
  for( i = 0; i < leds; i++) {
    heat[i]=0;
  }

  while(42) {

    for( i = 0; i < leds; i++) {
      a=heat[i];
      b=random() % x;
      heat[i] = qsub8(a,b);
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( k= leds- 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( (random() % 255) < sparking) {
      int y = random() % 7;

      a=heat[y];
      b=(random() % 95) + 160;

      heat[y] = qadd8(a, b);

    }

    //Step 4.  Map from heat cells to LED colors
    for(j = 0; j < leds; j++) {
        myleds[j] = HeatColor( heat[j]);
	write_gamma_color(&buf.pixels[j],myleds[j].r,myleds[j].b,myleds[j].g);
    }

	send_buffer(fd,&buf);
	usleep(10000);

  }

  lpd8806_free(&buf);
  close(fd);
  return 0;
}
