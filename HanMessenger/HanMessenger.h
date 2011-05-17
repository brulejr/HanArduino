/*
  HanMessenger.h - Home Automation Network Messenger library
  Copyright (c) 2011 Jon R. Brule.  All right reserved.

  Based upon CmdMessenger work by
  * Initial Messenger Library - By Thomas Ouellet Fredericks.
  * CmdMessenger Version 1 - By Neil Dudman.
  * CmdMessenger Version 2 - By Dreamcat4.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _HanMessenger_h
#define _HanMessenger_h

#include <inttypes.h>
#include "WProgram.h"

#include "Stream.h"

extern "C" {
  // Our callbacks are always method signature: void cmd(void);
  typedef void (*messengerCallbackFunction)(void);
}

#define MAXCALLBACKS 50         // The maximum number of unique commands
#define MAX_COMMAND_LENGTH 16
#define MESSENGERBUFFERSIZE 64  // The maximum length of the buffer (defaults to 64)
#define DEFAULT_TIMEOUT 5000    // Abandon incomplete messages if nothing heard after 5 seconds

enum {
  kCOMM_ERROR    = 000,  // Report serial port comm error back to the PC
  kACK           = 001,  // Arduino acknowledges cmd was received
  kARDUINO_READY = 002,  // After opening the comm port, send this cmd
                         // PC to check arduino is ready
  kERR           = 003,  // Reports badly formatted cmd, or cmd not known
  kSEND_CMDS_END
};

class HanMessenger {  

protected:

  static char messages[MAXCALLBACKS][MAX_COMMAND_LENGTH];

  // Polymorphism used to interact with serial class
  // Stream is an abstract base class which defines a base set
  // of functionality used by the serial classes.
  Stream *stream;

  boolean discardNewlines;
  boolean printNewlines;

  // (not implemented, generally not needed)
  // when we are sending a message and requre answer or acknowledgement
  // suspend any processing (process()) when serial intterupt is recieved
  // Even though we usually only have single processing thread we still need
  // this i think because Serial interrupts.
  // Could also be usefull when we want data larger than MESSENGERBUFFERSIZE
  // we could send a startCmd, which could pauseProcessing and read directly
  // from serial all the data, send acknowledge etc and then resume processing  
  boolean pauseProcessing;

  char buffer[MESSENGERBUFFERSIZE]; // Buffer that holds the data
  char commandSeparator;
  char fieldSeparator;

  char* current; // Pointer to current data
  char* last;

  messengerCallbackFunction default_callback;
  messengerCallbackFunction callbackList[MAXCALLBACKS];

  uint8_t bufferIndex;     // Index where to write the data
  uint8_t bufferLength;    // Is set to MESSENGERBUFFERSIZE
  uint8_t bufferLastIndex; // The last index of the buffer
  uint8_t dumped;
  uint8_t messageState;
    
  //------------------------------------------------------------------------------
  int calculateCommandId(char *command);
  void handleMessage(); 
  void init(Stream &stream, char field_separator, char command_separator);
  uint8_t process(int serialByte);
  void reset();

public:
  HanMessenger();
  HanMessenger(Stream &cstream);
  HanMessenger(Stream &cstream, char fldSeparator);
  HanMessenger(Stream &cstream, char fldSeparator, char cmdSeparator);

  void attach(messengerCallbackFunction newFunction);
  void attach(byte msgId, messengerCallbackFunction newFunction);
  void attach(char *command, messengerCallbackFunction newFunction);
  uint8_t available();
  boolean blockedTillReply(int timeout = DEFAULT_TIMEOUT);
  uint8_t checkString(char *string);
  void copyString(char *string, uint8_t size);
  void discard_LF_CR();
  void feedinSerialData();
  uint8_t next();
  void print_LF_CR();
  int readInt();
  char readChar();
  char* sendCmd(int cmdId, char *msg, 
		boolean reqAc = false, 
		char *replyBuff = NULL, int butSize = 0, 
		int timeout = DEFAULT_TIMEOUT, int retryCount = 10);
  void sendCommand(char *command, char *data);
  void sendCommand(int cmdId, char *msg);
  void sendCommand(int cmdId, char *msg, int addr);

};

#endif // _HanMessenger_H
