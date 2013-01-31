#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sched.h>
 
#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
 
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)
 
// I/O access
volatile unsigned *gpio;
 
// Time for easy calculations
static unsigned long long epoch;
 
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
 
// these define only works for the first 32 pins!
#define GPIO_LEV *(gpio+13) // read   pin level: 0=LOW | 1=HIGH
 
#define TEST_INP 18
#define TEST_INP_LEV (GPIO_LEV & (1 << TEST_INP))
#define PASS_COUNT 3
#define	FAST_COUNT	10000000
 
int piHiPri();
unsigned int millis();
int setup_io();
 
int main(void)
{
  unsigned int i = PASS_COUNT;
  uint32_t start, diff, count, sum=0;
  
  printf ("Raspberry Pi native-C GPIO input speed test program\n") ;
  
  piHiPri();
  if (setup_io() < 0) return -1;
  
  INP_GPIO(TEST_INP);
 
  for (; i--;)
  {
    printf("  Pass: %d: ", i);
    fflush(stdout);
 
    count = FAST_COUNT;
    start = millis();
    for (; count != 0;--count) {
      if(TEST_INP_LEV) { /* do nothing */}
    }
    diff = millis() - start;
    printf(" %8dmS\n", diff);
    sum += diff;
  }
  uint32_t avg = sum / PASS_COUNT; // <- this calculates wrong w/o fix
  //printf("%d\n",sum); // fixes O3 to O0-O2 levels (from ~350 million to ~12k)
  //printf("%d / %d\n",sum, PASS_COUNT); // fixes all cases
  printf("   Average: %8dmS", avg); // w/o fix this prints ~12k on O0-O2 and ~350 million on O3
  printf(": %6d/msec\n", (int)(double)FAST_COUNT / avg);
}
 
 
 
/*
 * piHiPri:
 *      Attempt to set a high priority schedulling for the running program
 *      This function comes from wiringPi (slightly modified) !
 *********************************************************************************
 */
int piHiPri()
{
  struct sched_param sched;
 
  memset(&sched, 0, sizeof(sched));
 
  sched.sched_priority = sched_get_priority_max(SCHED_RR);
  return sched_setscheduler(0, SCHED_RR, &sched);
}
 
/*
 * millis:
 *	Return a number of milliseconds as an unsigned int.
 *********************************************************************************
 */
 
unsigned int millis (void)
{
  struct timeval tv ;
  unsigned long long t1 ;
 
  gettimeofday (&tv, NULL) ;
 
  t1 = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000 ;
 
  return (uint32_t)(t1 - epoch) ;
}
 
//
// Set up a memory regions to access GPIO
//
int setup_io()
{
  int mem_fd;
  char *gpio_mem, *gpio_map;
  struct timeval tv;
 
  /* open /dev/mem */
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
     printf("can't open /dev/mem \n");
     return -1;
  }
 
  /* mmap GPIO */
 
  // Allocate MAP block
  if ((gpio_mem = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
    printf("allocation error \n");
    return -1;
  }
 
  // Make sure pointer is on 4K boundary
  if ((unsigned long)gpio_mem % PAGE_SIZE)
    gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);
 
  // Now map it
  gpio_map = (unsigned char *)mmap(
     (caddr_t)gpio_mem,
     BLOCK_SIZE,
     PROT_READ|PROT_WRITE,
     MAP_SHARED|MAP_FIXED,
     mem_fd,
     GPIO_BASE
  );
 
  if ((long)gpio_map < 0) {
    printf("mmap error %d\n", (int)gpio_map);
    return -1;
  }
 
  // Always use volatile pointer!
  gpio = (volatile unsigned *)gpio_map;
 
  // Initialise our epoch for millis()
 
  gettimeofday (&tv, NULL);
  epoch = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
 
  return 0;
} // setup_io