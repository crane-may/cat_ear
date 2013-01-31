#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
  if (wiringPiSetup () == -1)
    exit (1) ;

  pinMode(2, OUTPUT);

  while(1) {
    digitalWrite(2, 0);
    digitalWrite(2, 1);
  }

  return 0 ;
}
