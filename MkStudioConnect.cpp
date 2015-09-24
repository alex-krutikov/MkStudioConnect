/*
  MkStudioConnect.cpp - Library for connecting to MkStudio via serial port.
  Created by Alexey Krutikov, September 21, 2015.
  Released into the public domain.
*/


#include "MkStudioConnect.h"

#include "Arduino.h"

namespace {

const uint8_t MKSTUDIO_CONNECT_FIRST_BYTE         = 0x80;
const uint8_t MKSTUDIO_CONNECT_LAST_BYTE          = 0x81;
const uint8_t MKSTUDIO_CONNECT_COMMAND_READ       = 0x00;
const uint8_t MKSTUDIO_CONNECT_COMMAND_WRITE      = 0x01;
const uint8_t MKSTUDIO_CONNECT_COMMAND_WRITE_OR   = 0x02;
const uint8_t MKSTUDIO_CONNECT_COMMAND_WRITE_AND  = 0x03;

} // namespace

MkStudioConnect::MkStudioConnect()
  : m_stream(0)
  , m_base(0)
  , m_baseSize(0)
  , m_state(0)
  , m_index(0)
  , m_extra(0)
  , m_extraIndex(0)
{
}

void MkStudioConnect::begin(Stream &stream, void *base, uint16_t baseSize)
{
  m_stream = &stream;
  m_base = static_cast<uint8_t*>(base);
  m_baseSize = baseSize;
}

void MkStudioConnect::run()
{
  switch (m_state)
  {
    case STATE_INIT:
      m_index = 0;
      m_extraIndex = 0;
      m_state = STATE_RECEIVE_FIRST_BYTE;
      break;
    case STATE_RECEIVE_FIRST_BYTE:
      if (m_stream->available() > 0) {
        const char c = m_stream->read();
        if (c == (char)MKSTUDIO_CONNECT_FIRST_BYTE) {
          m_state = STATE_RECEIVE;
          break;
        }
      }
      break;
    case STATE_RECEIVE:
      while (m_stream->available() > 0) {
        const char c = m_stream->read();
        if (c == (char)MKSTUDIO_CONNECT_FIRST_BYTE) {
          m_index = 0;
          m_extraIndex = 0;
          break;
        }
        if (c == (char)MKSTUDIO_CONNECT_LAST_BYTE) {
          if (m_stream->available() > 0) {
            m_state = STATE_INIT;
            break;
          }
          m_state = STATE_PROCESS;
          break;
        }
        if (m_extraIndex == 0) {
          m_extra = c << 1;
          m_extraIndex = 7;
          break;
        }
        else
        {
          m_buffer[m_index] = c | (m_extra & 0x80);
          m_extraIndex--;
          m_extra <<= 1;
          m_index++;
          if (m_index >= sizeof(m_buffer)) {
            m_state = STATE_INIT;
            break;
          }
        }
      }
      break;
    case STATE_PROCESS:
      processTransaction();
      m_state = STATE_TRANSMIT;
      break;
    case STATE_TRANSMIT:
      m_extraIndex = 0;
      m_stream->write(MKSTUDIO_CONNECT_FIRST_BYTE);
      for (uint8_t i = 0; i < m_index; ++i) {
        if (m_extraIndex == 0) {
          m_extra = 0;
          uint8_t mask = 0x40;
          for (uint8_t j = 0; (j < 7) && (i + j) < m_index; ++j) {
            if (m_buffer[i + j] & 0x80) {
              m_extra |= mask;
            }
            mask >>= 1;
          }
          m_extraIndex = 7;
          m_stream->write(m_extra);
        }
        m_stream->write(m_buffer[i] & 0x7F);
        m_extraIndex--;
      }
      m_stream->write(MKSTUDIO_CONNECT_LAST_BYTE);
      m_stream->flush();
      m_state = STATE_INIT;
      break;      
  }
}

void MkStudioConnect::processTransaction()
{
  if (m_index < 4) {
    m_index = 0;
    return;
  }

  const uint8_t command = m_buffer[0];
  const uint16_t addr = (static_cast<uint16_t>(m_buffer[2]) << 8) | m_buffer[1];
  const uint8_t len = m_buffer[3];

  if ((addr + len) > m_baseSize) {
    m_index = 0;
    return;
  }

  if (len  > (sizeof(m_buffer) - 4)) {
    m_index = 0;
    return;
  }
  
  if (command == MKSTUDIO_CONNECT_COMMAND_READ) {
    for (uint8_t i = 0; i < len; ++i) {
      m_buffer[4 + i] = m_base[addr + i];
    }
    m_index += len;
    return;
  }

  if (command == MKSTUDIO_CONNECT_COMMAND_WRITE) {
    for (uint8_t i = 0; i < len; ++i) {
      m_base[addr + i] = m_buffer[4 + i];
    }
    return;
  }

  if (command == MKSTUDIO_CONNECT_COMMAND_WRITE_OR) {
    for (uint8_t i = 0; i < len; ++i) {
      m_base[addr + i] |= m_buffer[4 + i];
    }
    return;
  }

  if (command == MKSTUDIO_CONNECT_COMMAND_WRITE_AND) {
    for (uint8_t i = 0; i < len; ++i) {
      m_base[addr + i] &= m_buffer[4 + i];
    }
    return;
  }

  m_index = 0;
  return;
}

