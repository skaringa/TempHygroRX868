/*
 * Receiver for data of wireless weather sensors with RX868 and Raspberry Pi.
 *
 * Implementation of decoder module.
 */
#include <stdio.h>
#include <time.h>
#include <list>
#include <vector>
#include "Decoder.h"

const char* SENSOR_TYPES[] = {"Thermo", "Thermo/Hygro", "Rain(?)", "Wind(?)", "Thermo/Hygro/Baro", "Luminance(?)", "Pyrano(?)", "Kombi"};

const int SENSOR_DATA_COUNT[] = {5, 8, 5, 8, 12, 6, 6, 14};

char dateTimeBuf[50];

Decoder::Decoder(int minLen, int maxLen) 
  : minLen(minLen), maxLen(maxLen), decoderState(WAIT), syncCount(0)
  {}

int Decoder::bitval(int len, int lo) {
  if (len >= minLen && len <= maxLen) {
    if (lo < len/2) {
      return 0;
    } else {
      return 1;
    }
  }
  return -1; // no valid bit
}

bool Decoder::pulse(int len, int lo) {
  bool hasNewValue = false;
  int val = bitval(len, lo);
  if (-1 == val) {
    if (DATA == decoderState) {
      // end of frame?
      data.push_back(1);
      hasNewValue = decode();
      data.clear();
    }
    decoderState = WAIT;
  } else if (WAIT == decoderState) {
    if (0 == val) {
      // first sync pulse
      syncCount = 1;
      decoderState = SYNC;
    }
  } else if (SYNC == decoderState) {
    if (0 == val) {
      // another sync pulse
      syncCount++;
    } else if (1 == val && syncCount > 6) {
      // got the start bit
      syncCount = 0;
      decoderState = DATA;
    }
  } else if (DATA == decoderState) {
    data.push_back(val);
  }
  
  return hasNewValue;
}

int Decoder::popbits(int num) {
  int val = 0;
  if (data.size() < num) {
#ifdef DEBUG
    printf("data exhausted\n");
#endif
    return 0;
  }
  for (int i = 0; i < num; ++i) {
    val += data.front() << i;
    data.pop_front();
  }
  return val;
}

bool Decoder::expectEon() {
  // check end of nibble (1)
  if (popbits(1) != 1) {
#ifdef DEBUG
    printf("end of nibble is not 1\n");
#endif
    return false;
  }
  return true;
}

bool Decoder::decode() {
#ifdef DEBUG
  printf("decode: ");
  for (std::list<int>::const_iterator it = data.begin(); it != data.end(); ++it) {
    printf("%d", *it);
  }
  printf("\n");
#endif
  int check = 0;
  int sum = 0;
  int sensorType = popbits(4) & 7;
  if (!expectEon()) {
    return false;
  }
  check ^= sensorType;
  sum += sensorType;

  // read data as nibbles
  int nibbleCount = SENSOR_DATA_COUNT[sensorType];
  std::vector<int> dec;
  for (int i = 0; i < nibbleCount; ++i) {
    int nibble = popbits(4);
    if (!expectEon()) return false;
    dec.push_back(nibble);
    check ^= nibble;
    sum += nibble;
  }

  // check
  if (check != 0) {
#ifdef DEBUG
    printf("Check is not 0 but %d\n", check);
#endif
    return false;
  }

  // sum
  int sumRead = popbits(4);
  sum += 5;
  sum &= 0xF;
  if (sumRead != sum) {
#ifdef DEBUG
    printf("Sum read is %d but computed is %d", sumRead, sum);
#endif
    return false;
  }

  // compute values
  decoderOutput.sensorType = sensorType;
  decoderOutput.sensorTypeStr = SENSOR_TYPES[sensorType];
  decoderOutput.address = dec[0] & 7;
  decoderOutput.temperature = (dec[3]*10.f + dec[2] + dec[1]/10.f) * (dec[0]&8?-1.f:1.f);
  decoderOutput.humidity = 0.f;
  decoderOutput.wind = 0.f;
  decoderOutput.rainSum = 0;
  decoderOutput.rainDetect = 0;
  decoderOutput.pressure = 0;
  
  if (7 == sensorType) {
    // Kombisensor
    decoderOutput.humidity = dec[5]*10.f + dec[4];
    decoderOutput.wind = dec[8]*10.f + dec[7] + dec[6]/10.f;
    decoderOutput.rainSum = dec[11]*16*16 + dec[10]*16 + dec[9];
    decoderOutput.rainDetect = dec[0]&2 == 1;
  }
  if (1 == sensorType || 4 == sensorType) {
    // Thermo/Hygro
    decoderOutput.humidity = dec[6]*10.f + dec[5] + dec[4]/10.f;
  }
  if (4 == sensorType) {
    // Thermo/Hygro/Baro
    decoderOutput.pressure = 200 + dec[9]*100 + dec[8]*10 + dec[7];
  }

  return true;
}

DecoderOutput Decoder::getDecoderOutput() const {
  return decoderOutput;
}

/* 
 * default implementation to print DecoderOutput to stdout
 */
void printDecoderOutput(DecoderOutput val) {
  // print current time
  time_t t = time(0);
  struct tm *tmp = localtime(&t);
  if (strftime(dateTimeBuf, sizeof(dateTimeBuf), "%x %X", tmp)) {
    printf("time: %s\n", dateTimeBuf);
  }
  // print sensor values depending on type of sensor
  printf("sensor type: %s\n", val.sensorTypeStr);
  printf("address: %d\n", val.address);

  printf("temperature: %.1f\n", val.temperature);

  if (7 == val.sensorType) {
    // Kombisensor
    printf("humidity: %.0f\n", val.humidity);
    printf("wind: %.1f\n", val.wind);
    printf("rain sum: %d\n", val.rainSum);
    printf("rain detector: %d\n", val.rainDetect);
  }
  if (1 == val.sensorType || 4 == val.sensorType) {
    // Thermo/Hygro
    printf("humidity: %.1f\n", val.humidity);
  }
  if (4 == val.sensorType) {
    // Thermo/Hygro/Baro
    printf("pressure: %d\n", val.pressure);
  }
  printf("\n");
}
