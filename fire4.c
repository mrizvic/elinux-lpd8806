#include <unistd.h>
#include <stdio.h>
#include "lpd8806led.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


static const char *device = "/dev/spidev0.0";
static const int leds = 260;
static const int frames = 10000;

int sparking = 60;
int cooling = 160;
int x=0;

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

int signal_handler(int signo) {
  FILE *f;
  char line[16];
  int value;
  printf("signal received %d\n",signo);

  if (signo == SIGUSR1) {

    //read cooling parameter
    f=fopen("/tmp/lpd8806-fire-cooling","r");
    if (f == NULL) { printf("fopen error %d\n", errno); return 1; }
    fgets(line,16,f);
    value = atoi(line);
    if ( (value > 0) || (value < 300) ) { cooling = value; }
    x=((cooling*10)/leds) + 2;
    printf("cooling set to %d\n", cooling);
    fclose(f);
    
    //read sparking parameter
    f=fopen("/tmp/lpd8806-fire-sparking","r");
    if (f == NULL) { printf("fopen error %d\n", errno); return 1; }
    fgets(line,16,f);
    value = atoi(line);
    if ( (value > 0) || (value < 300) ) { sparking = value; }
    printf("sparking set to %d\n", sparking);
    fclose(f);

  }

}

int main(int argc, char *argv[]) {
  lpd8806_buffer buf;
  int fd;
  int ret;
  int i, j;
  int a1,a2,b1,b2;
  int m,n;
  int k;
  int color=0;
  int heat1[leds];
  int heat2[leds];
  CRGB myleds1[leds];
  CRGB myleds2[leds];
  int y;
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
    heat1[i]=0;
    heat2[i]=0;
  }

  //register signal handler
  if (signal(SIGUSR1, signal_handler) == SIG_ERR) { printf("cant catch SIGUSR1"); return 1; }
  printf("pid = %d\n", getpid());

  while(42) {

    for( i = 0; i < leds; i++) {
      a1=heat1[i];
      a2=heat2[i];
      b1=random() % x;
      b2=random() % x;
      heat1[i] = qsub8(a1,b1);
      heat2[i] = qsub8(a2,b2);
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( k= leds- 1; k >= 2; k--) {
      heat1[k] = (heat1[k - 1] + heat1[k - 2] + heat1[k - 2] ) / 3;
      heat2[k] = (heat2[k - 1] + heat2[k - 2] + heat2[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( (random() % 255) < sparking) {
      int y1 = random() % 7;
      a1=heat1[y1];
      b1=(random() % 95) + 160;
      heat1[y1] = qadd8(a1, b1);
    }

    if( (random() % 255) < sparking) {
      int y2 = random() % 7;
      a2=heat2[y2];
      b2=(random() % 95) + 160;
      heat2[y2] = qadd8(a2, b2);
    }

    //Step 4.  Map from heat cells to LED colors

    for(j = 0; j < leds; j++) {
        myleds1[j] = HeatColor( heat1[j]);
        myleds2[j] = HeatColor( heat2[j]);
	write_gamma_color(&buf.pixels[j],myleds1[j].r,myleds1[j].b,myleds1[j].g);
    }

    for(j = leds; j >=130; j--) {
	write_gamma_color(&buf.pixels[j],myleds2[leds-j].r,myleds2[leds-j].b,myleds2[leds-j].g);
    }

	send_buffer(fd,&buf);
	usleep(10000);

  }

  lpd8806_free(&buf);
  close(fd);
  return 0;
}
