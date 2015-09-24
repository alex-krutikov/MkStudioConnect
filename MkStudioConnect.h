/*
  MkStudioConnect.h - Library for connecting to MkStudio via serial port.
  Created by Alexey Krutikov, September 21, 2015.
  Released into the public domain.
*/

#ifndef MkStudioConnect_h
#define MkStudioConnect_h

#include "Arduino.h"

#define MKSTUDIOCONNECT_BUFFER_SIZE 32

class MkStudioConnect
{
public:
  MkStudioConnect();
  void begin(Stream &stream, void *base, uint16_t baseSize);
  void run();

private:
  void processTransaction();

private:
  enum {STATE_INIT = 0, STATE_RECEIVE_FIRST_BYTE, STATE_RECEIVE, STATE_PROCESS, STATE_TRANSMIT};
  Stream *m_stream;
  uint8_t *m_base;
  uint16_t m_baseSize;
  uint8_t m_buffer[MKSTUDIOCONNECT_BUFFER_SIZE];
  uint8_t m_state;
  uint8_t m_index;
  uint8_t m_extra;
  uint8_t m_extraIndex;
};


#endif
