/* -----------------------------------------------------------------------------
  Home Automation - Remote Control node
  
  A Home Automation device controller for irrigation valves.
 
  Based upon work by
  - TBD
 
  Circuit:
  * 74HC595 connected to pins 7, 8, 9
  * 8 relays connected 74HC595 data pins through 2N2222 switching transitors
 
  URIs:
  * /valve_on[/{addr}]   - turns on all valves or a single one if {addr} given
  * /valve_off[/{addr}]  - turns off all valves or a single one if {addr} given
 
  Created 05/12/2011 by Jon Brule
----------------------------------------------------------------------------- */

#include <HanMessenger.h>
#include <Streaming.h>

#define BUFFER_SIZE 255
#define PIN_ERROR 2
#define PIN_HEARTBEAT 13
#define PIN_RELAY_CLOCK 7
#define PIN_RELAY_DATA 9
#define PIN_RELAY_LATCH 8

#define HEARTBEAT_INTERVAL 20
#define SAMPLE_INTERVAL 100

#define NUM_RELAYS 8
#define RELAY_OFF LOW
#define RELAY_ON HIGH

// general-purpose buffer
char buffer[BUFFER_SIZE] = { '\0' };

// initialze Home Automation Network Messenger
HanMessenger hanmsgr = HanMessenger();

// shift-out register for relays
boolean relays[NUM_RELAYS];

// timeout handling
long previousMillis = 0;
int counter = 0;
int errorCount = 0;

//------------------------------------------------------------------------------
void setup() {
  // Listen on serial connection for messages from the pc
  // Serial.begin(57600);  // Arduino Duemilanove, FTDI Serial
  //Serial.begin(115200); // Arduino Uno, Mega, with AT8u2 USB
  Serial.begin(9600);

  // Make output more readable whilst debugging in Arduino Serial Monitor
  hanmsgr.print_LF_CR();

  // Attach my application's user-defined callback methods
  hanmsgr.attach("valve_off", valve_off_msg);
  hanmsgr.attach("valve_on", valve_on_msg);

  // setup physical interface
  pinMode(PIN_ERROR, OUTPUT);
  pinMode(PIN_HEARTBEAT, OUTPUT);
  pinMode(PIN_RELAY_CLOCK, OUTPUT);
  pinMode(PIN_RELAY_DATA, OUTPUT);  
  pinMode(PIN_RELAY_LATCH, OUTPUT);

}

//------------------------------------------------------------------------------
void loop() {
  // Process incoming serial data, if any
  hanmsgr.feedinSerialData();

  // handle timeout function, if any
  if (millis() - previousMillis > SAMPLE_INTERVAL) {
    timeout();
    previousMillis = millis();
  }
}

//------------------------------------------------------------------------------
//----------------------- C A L L B A C K  M E T H O D S -----------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void valve_off_msg() {

  // read relay device address
  int addr = calculateAddress();
  if (addr == -1) return;
  
  // clear any existing error
  clearError();

  // turn single relay off
  if (addr > 0) {
    relays[addr - 1] = RELAY_OFF;
    updateRelays();
    hanmsgr.sendCommand(kACK, "Relay [%d] turned off", addr);
  } 
  
  // turn all relays off
  else {
    for (int i = 0; i < NUM_RELAYS; i++) relays[i] = RELAY_OFF;
    updateRelays();
    hanmsgr.sendCommand(kACK, "All relays turned off");
  }

}

//------------------------------------------------------------------------------
void valve_on_msg() {

  // read relay device address
  int addr = calculateAddress();
  if (addr == -1) return;

  // clear any existing error
  clearError();
  
  // turn single relay on
  if (addr > 0) {
    relays[addr - 1] = RELAY_ON;
    updateRelays();
    hanmsgr.sendCommand(kACK, "Relay [%d] turned off", addr);
  } 
  
  // turn all relay on
  else {
    for (int i = 0; i < NUM_RELAYS; i++) relays[i] = RELAY_ON;
    updateRelays();
    hanmsgr.sendCommand(kACK, "All relays turned on");
  }

}

//------------------------------------------------------------------------------
//---------------------- S U P P O R T  M E T H O D S --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int calculateAddress() {
  int addr = 0;
  if (hanmsgr.available()) {
    addr = hanmsgr.readInt();
    if (addr < 1 || addr > NUM_RELAYS) {
      setError();
      hanmsgr.sendCommand(kERR, "Invalid Relay Address [%d]", addr);
      return -1;
    }
    return addr;
  } else {
    return 0;
  }
}

//------------------------------------------------------------------------------
void clearError() {
  errorCount = 0;
}


//------------------------------------------------------------------------------
void setError() {
  ++errorCount;
}

//------------------------------------------------------------------------------
void timeout() {
  
  // handle hearbeat
  if ((counter++ % HEARTBEAT_INTERVAL) == 0) 
    digitalWrite(PIN_HEARTBEAT, HIGH);
  else 
    digitalWrite(PIN_HEARTBEAT, LOW);
    
  // handle error counter
  if (errorCount > 0 && (counter % lround(HEARTBEAT_INTERVAL / errorCount)) == 0) 
    digitalWrite(PIN_ERROR, HIGH);
  else 
    digitalWrite(PIN_ERROR, LOW);
    
}  

//------------------------------------------------------------------------------
void updateRelays() {

  // the bits to send
  byte bitsToSend = 0;

  // turn off the output so relays don't adjust while shifting bits
  digitalWrite(PIN_RELAY_LATCH, LOW);

  // turn on the next highest bit in bitsToSend
  for (int i = 0; i < NUM_RELAYS; i++) {
    bitWrite(bitsToSend, i, relays[i]);
  }

  // shift the bits out:
  shiftOut(PIN_RELAY_DATA, PIN_RELAY_CLOCK, MSBFIRST, bitsToSend);

  // turn on output so the relays can adjust
  digitalWrite(PIN_RELAY_LATCH, HIGH);

}
