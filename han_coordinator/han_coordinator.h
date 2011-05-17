/* -----------------------------------------------------------------------------
  RESTful - Home Automation Coordinator
  
  A REST-style Home Automation Control XBee coordinator for the Arduino Home 
  Automation Network controller via the Wiznet Ethernet shield.
 
  Based upon work by
  - David A. Mellis's "Web Server" ethernet shield example sketch.
  - Jason J. Gullickson's "RESTduino" sketch.
 
  Circuit:
  * Ethernet shield attached to pins 10, 11, 12, 13
 
  URIs:
  * /{cmd}[/{data}]   - send {cmd} to {addr} remote with optional data
 
  Created 05/02/2011 by Jon Brule
----------------------------------------------------------------------------- */

struct RestRequest {
  char method[8];
  char command[16];
  char data[32];
};

RestRequest* parseClientBuffer();
