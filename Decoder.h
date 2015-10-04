#pragma once

#include <stdio.h>
#include <list>

typedef enum {
  WAIT, SYNC, DATA
} EDecoderState;

typedef struct {
  int sensorType;
  const char* sensorTypeStr; 
  int address; 
  float temperature;
  float humidity;
  float wind;
  int rainSum;
  int rainDetect;
  int pressure;
} DecoderOutput;

class Decoder {
  public:
    /*
     * minLen, maxLen: min and max length of a valid pulse
     * in multiples of 1/sampleRate.
     * This default values work for a sample rate of 1/100 µs.
     */ 
    Decoder(int minLen = 5, int maxLen = 14);
    /*
     * Feed a ON-OFF pulse into the decoder.
     * len: The length of the whole pulse in samples.
     * lo: The length of the OFF part in samples.
     * return: true if a complete data packet is available.
     */
    bool pulse(int len, int lo);
    /*
     * Get the actual output of the decoder.
     * Should be called when pulse returns true.
     */
    DecoderOutput getDecoderOutput() const;
  private:
    int bitval(int len, int lo);
    int popbits(int num);
    bool expectEon();
    bool decode();

    int syncCount;
    const int minLen;
    const int maxLen;
    std::list<int> data;
    EDecoderState decoderState; 
    DecoderOutput decoderOutput;
};

void printDecoderOutput(DecoderOutput val);
