#include <wiringPi.h>
#include <stdio.h>
#include <signal.h>
#include "Decoder.h"

static volatile int keepRunning = 1;
static DecoderOutput out;
static volatile int hasOut = 0;

void sigIntHandler(int dummy) {
  keepRunning = 0;
} 

PI_THREAD (decoderThread) {
  piHiPri(50);

  // init decoder for a sample rate of 1/200µs
  Decoder decoder(4,10);
  int x;
  int len = 0;
  int lo = 0;
  int px = 0;

  while (keepRunning) {
    x = digitalRead(2);

    len++;
    if (0 == x) {
      lo++;
    }
    if (x != px && 0 != x) {
      // slope low->high
      if (decoder.pulse(len, lo)) {
        piLock(1);
        out = decoder.getDecoderOutput();
        hasOut = 1;
        piUnlock(1);
      }
      len = 1;
      lo = 0;
    }
    px = x;

    delayMicroseconds(200);
  }
}

int main() {
  wiringPiSetup();
/*
  BCM GPIO 27: DATA (IN) == WiPin 2
  BCM GPIO 22: EN (OUT)  == WiPin 3
*/
  pinMode(3, OUTPUT);
  digitalWrite(3, 1); // enable rx
  pinMode(2, INPUT);
  pullUpDnControl(2, PUD_DOWN);

  signal(SIGINT, sigIntHandler);

  piThreadCreate(decoderThread);

  while (keepRunning) {
    piLock(1);
    if (hasOut) { 
      hasOut = 0;
      piUnlock(1);
      printDecoderOutput(out);
    }
    piUnlock(1);
    delay(100);
  }

  printf("clean up and exit\n");
  digitalWrite(3, 0); // disable rx
}       
