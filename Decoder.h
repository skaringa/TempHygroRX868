/*
 * Receiver for data of wireless weather sensors with RX868 and Raspberry Pi.
 *
 * Definition of decoder module.
 */
#pragma once

#include <stdio.h>
#include <list>

/*
 * Internal decoder state.
 */
typedef enum {
  WAIT, SYNC, DATA
} EDecoderState;

/*
 * Value object to hold the decoded sensor values.
 */
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

/*
 * The decoder.
 */
class Decoder {
  public:
    /*
     * Constructor.
     *
     * minLen, maxLen: min and max length of a valid pulse
     * in multiples of 1/sampleRate.
     * This default values work for a sample rate of 1/100 µs.
     */ 
    Decoder(int minLen = 5, int maxLen = 14);
    /*
     * Feed a ON-OFF pulse into the decoder.
     *
     * len: The length of the whole pulse in samples.
     * lo: The length of the OFF part in samples.
     * return: true if a complete data packet is available.
     */
    bool pulse(int len, int lo);
    /*
     * Get the actual output of the decoder.
     *
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

/*
 * Utility function to print DecoderOuput to standard output.
 */
void printDecoderOutput(DecoderOutput val);
