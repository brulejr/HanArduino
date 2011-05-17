/*
  HanMessenger.cpp - Home Automation Messenger library
  Copyright (c) 2011 Jon R. Brule.  All right reserved.

  Based upon HanMessenger work by
  * Initial Messenger Library - By Thomas Ouellet Fredericks.
  * HanMessenger Version 1 - By Neil Dudman.
  * HanMessenger Version 2 - By Dreamcat4.

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

/******************************************************************************
 * Includes
 ******************************************************************************/
extern "C" {
  #include <stdlib.h>
}
#include "HanMessenger.h"
#include <Streaming.h>

/******************************************************************************
 * Definitions
 ******************************************************************************/
char HanMessenger::messages[MAXCALLBACKS][MAX_COMMAND_LENGTH] = {
  "system_reset",
  "system_status",
  "valve_on",
  "valve_off",
  "valve_toggle",
  "valve_state"
};

/******************************************************************************
 * Constructors
 ******************************************************************************/

//------------------------------------------------------------------------------
//
// Constructs a default messenger using the Serial stream
//
HanMessenger::HanMessenger() {
  init(Serial, DEFAULT_FIELD_SEPARATOR, DEFAULT_COMMAND_SEPARATOR);
}

HanMessenger::HanMessenger(
  Stream &stream, char fieldSeparator, char commandSeparator
) {
  init(stream, fieldSeparator, commandSeparator);
}

/******************************************************************************
 * User API
 ******************************************************************************/

//------------------------------------------------------------------------------
void HanMessenger::attach(messengerCallbackFunction newFunction) {
  default_callback = newFunction;
}

void HanMessenger::attach(byte msgId, messengerCallbackFunction newFunction) {
  if (msgId > 0 && msgId <= MAXCALLBACKS) {
    callbackList[msgId - 1] = newFunction;
  }
}

void HanMessenger::attach(char *command, messengerCallbackFunction newFunction) {
  int cmd = calculateCommandId(command);
  if (cmd > -1) {
    attach(cmd, newFunction);
  }
}

//------------------------------------------------------------------------------
uint8_t HanMessenger::available() {
  return next();
}

//------------------------------------------------------------------------------
boolean HanMessenger::blockedTillReply(int timeout) {
  unsigned long start = millis();
  unsigned long time = start;
  while(!stream->available() || (start - time) > timeout)
    time = millis();
}

//------------------------------------------------------------------------------
int HanMessenger::calculateCommandId(char *command) {
  int i = -1;
  for (i = 0; i < MAXCALLBACKS; i++) {
    String s = String(messages[i]);
    if (s.equals(command)) break;
  }
  return (i == MAXCALLBACKS) ? -1 : (kSEND_CMDS_END + i);
}

//------------------------------------------------------------------------------
uint8_t HanMessenger::checkString(char *string) {
  if (next()) {
    if ( strcmp(string,current) == 0 ) {
      dumped = 1;
      return 1;
    } else {
      return 0;
    }
  } 
}

//------------------------------------------------------------------------------
void HanMessenger::copyString(char *string, uint8_t size) {
  if (next()) {
    dumped = 1;
    strlcpy(string,current,size);
  } else {
    if ( size ) string[0] = '\0';
  }
}

//------------------------------------------------------------------------------
void HanMessenger::discard_LF_CR() {
  discardNewlines = true;
}

//------------------------------------------------------------------------------
void HanMessenger::feedinSerialData() {
  while ( !pauseProcessing && stream->available( ) ) 
    process(stream->read( ) );
}

//------------------------------------------------------------------------------
void HanMessenger::handleMessage() {

  // If we didnt want to use ASCII integer...
  // we would change the line below vv
  int id = readInt();

  //Serial << "ID+" << id << endl;
  // Because readInt() can fail and return a 0 we can't
  // start our array index at that number
  if (id > 0 && id <= MAXCALLBACKS && callbackList[id-1] != NULL)
    (*callbackList[id-1])();
  else // Cmd not registered default callback
    (*default_callback)();
}

//------------------------------------------------------------------------------
void HanMessenger::init(Stream &cstream, char fldSeparator, char cmdSeparator) {
  stream = &cstream;
  
  discardNewlines = false;
  printNewlines   = false;

  fieldSeparator   = fldSeparator;
  commandSeparator = cmdSeparator;

  bufferLength = MESSENGERBUFFERSIZE;
  bufferLastIndex = MESSENGERBUFFERSIZE -1;
  reset();

  default_callback = NULL;
  for (int i = 0; i < MAXCALLBACKS; i++)
    callbackList[i] = NULL;

  pauseProcessing = false;
}

//------------------------------------------------------------------------------
uint8_t HanMessenger::next() {
  char *temppointer= NULL;

  // Currently, cmd messenger only supports 1 char for the field seperator
  const char seperator_tokens[] = { fieldSeparator,'\0' };
  switch (messageState) {
    case 0:
    return 0;
    case 1:
    temppointer = buffer;
    messageState = 2;
    default:
    if (dumped)
      current = strtok_r(temppointer,seperator_tokens,&last);
    if (current != NULL) {
      dumped = 0;
      return 1; 
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
void HanMessenger::print_LF_CR() {
  printNewlines = true;
}

//------------------------------------------------------------------------------
uint8_t HanMessenger::process(int serialByte) {
  messageState = 0;
  char serialChar = (char)serialByte;

  if (serialByte > 0) {

    // Currently, cmd messenger only supports 1 char for the command seperator
    if (serialChar == commandSeparator) {
      buffer[bufferIndex]=0;
      if (bufferIndex > 0) {
        messageState = 1;
        current = buffer;
      }
      reset();
    } else {
      buffer[bufferIndex]=serialByte;
      bufferIndex++;
      if (bufferIndex >= bufferLastIndex) reset();

      if (discardNewlines && (serialChar != fieldSeparator))
        if ((serialChar == '\n') || (serialChar == '\r'))
          reset();
    }
  }

  if (messageState == 1) {
    handleMessage();
  }
  return messageState;

}

//------------------------------------------------------------------------------
char HanMessenger::readChar() {
  if (next()) {
    dumped = 1;
    return current[0];
  }
  return 0;
}

//------------------------------------------------------------------------------
int HanMessenger::readInt() {
  if (next()) {
    dumped = 1;
    return atoi(current);
  }
  return 0;
}

//------------------------------------------------------------------------------
void HanMessenger::reset() {
  bufferIndex = 0;
  current = NULL;
  last = NULL;
  dumped = 1;
}

//------------------------------------------------------------------------------
// if the arguments in the future could be passed in as int/long/float etc
// then it might make sense to use the above writeReal????() methods
// I've removed them for now.
char* HanMessenger::sendCmd(
  int cmdId, char *msg, 
  boolean reqAc, 
  char *replyBuff, int butSize, 
  int timeout, int retryCount
) {
  int tryCount = 0;  
  pauseProcessing = true;
  //*stream << cmdId << field_separator << msg << endl;
  stream->print(cmdId);
  stream->print(fieldSeparator);
  stream->print(msg);
  stream->print(commandSeparator);
  if (printNewlines)
    stream->println(); // should append BOTH \r\n
  if (reqAc) {    
    do {
      blockedTillReply(timeout);
      //strcpy(replyBuff, buf;
    } while( tryCount < retryCount);
  }
  
  pauseProcessing = false;
  return NULL;
}

//------------------------------------------------------------------------------
void HanMessenger::sendCommand(char *command, char *data) { 
 
  // determine command
  int cmd = calculateCommandId(command);
  if (cmd == -1) return;

  // assemble message
  strcpy(buffer, "");
  strcat(buffer, data);
  
  // send message to remotes and block for acknologement message
  sendCmd(cmd, buffer);

}

void HanMessenger::sendCommand(int cmdId, char *msg) {
  sendCmd(cmdId, msg);
}

void HanMessenger::sendCommand(int cmdId, char *msg, int addr) {
  sprintf(buffer, msg, addr);
  sendCmd(cmdId, buffer);
}

//------------------------------------------------------------------------------
// Not sure if it will work for signed.. check it out
/*
unsigned char *HanMessenger::writeRealInt(int val, unsigned char buff[2]) {
  buff[1] = (unsigned char)val;
  buff[0] = (unsigned char)(val >> 8);  
  buff[2] = 0;
  return buff;
}

//------------------------------------------------------------------------------
char* HanMessenger::writeRealLong(long val, char buff[4]) {
  //buff[1] = (unsigned char)val;
  //buff[0] = (unsigned char)(val >> 8);  
  return buff;
}

//------------------------------------------------------------------------------
char* HanMessenger::writeRealFloat(float val, char buff[4]) {
  //buff[1] = (unsigned char)val;
  //buff[0] = (unsigned char)(val >> 8);  
  return buff;
}
*/
