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
                        (format of the data depends upon the command)
 
  Created 05/09/2011 by Jon Brule
----------------------------------------------------------------------------- */

#include <CmdMessenger.h>
#include <Ethernet.h>
#include <HanMessenger.h>
#include <SPI.h>
#include <Streaming.h>
#include "han_coordinator.h"

// Uncomment to send debug info to serial monitor
//#define ECHO_TO_SERIAL 1

#define BROWSER_RECEIVE_DELAY 1
#define BUFFER_SIZE 255
#define HTTP_PORT 80
#define MAX_COMMAND_LENGTH 16
#define NUM_COMMANDS 6
#define SERIAL_BAUD_RATE 9600

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[]  = {192,168,102,199};

// general-purpose buffer
char buffer[BUFFER_SIZE];
int bufferIndex = 0;

// construct empty rest request
RestRequest clientRequest = { };

// Initialize the Ethernet server library
Server server(HTTP_PORT);
Client client = NULL;

// Initialze the Home Automation Network Messenger
HanMessenger hanmsgr = HanMessenger();

//------------------------------------------------------------------------------
void setup() {
  
  //  turn on serial (for debuggin)
  Serial.begin(SERIAL_BAUD_RATE);
  
  // Attach my application's user-defined callback methods
  hanmsgr.attach(kACK, web_service_return_msg);

  // start the Ethernet connection and the server
  Ethernet.begin(mac, ip);
  server.begin();
  
}

//------------------------------------------------------------------------------
void loop() {

  // listen for incoming clients
  client = server.available();
  if (client) {

    //  reset input buffer
    bufferIndex = 0;

    // handle client connection
    while (client.connected()) {
      if (client.available()) {

        // buffer character from client. if necessary to buffer, the continue
        // immediately to next character
        if (bufferClientStream(&client)) {
          continue;
        }
#if ECHO_TO_SERIAL
        Serial.print("raw buffer: ");
        Serial.println(buffer);
#endif // ECHO_TO_SERIAL
        
        // parse buffer of client stream
        RestRequest* clientRequest = parseClientBuffer();
        
        // send action message to remotes in a blocked manner
#if ECHO_TO_SERIAL
        Serial.print("Sending command(");
        Serial.print(clientRequest->command);
        Serial.print(") with data(");
        Serial.print(clientRequest->data);
        Serial.println(")");
#endif // ECHO_TO_SERIAL
        hanmsgr.sendCommand(clientRequest->command, clientRequest->data);
        
        // generate output
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();
        
        // do not loop since client request is handled
        break;
        
      }
    }

    // give the web browser time to receive the data
    delay(BROWSER_RECEIVE_DELAY);
    
    // close the connection:
    client.stop();
  }
}

//------------------------------------------------------------------------------
//----------------------- C A L L B A C K  M E T H O D S -----------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void web_service_return_msg() {

  // generate successful http response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  
  // generate json body
  String jsonOut = String();
  jsonOut += "{";
  jsonOut += "}";

  client.println(jsonOut);
}

//------------------------------------------------------------------------------
//---------------------- S U P P O R T  M E T H O D S --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool bufferClientStream(Client *client) {

  // read a character from client stream
  char c = client->read();

  //  add character to buffer if not carriage return / linefeed
  //  if we run out of buffer, overwrite the end
  if (c != '\n' && c != '\r') {
    buffer[bufferIndex++] = c;
    if (bufferIndex >= BUFFER_SIZE) {
      bufferIndex -= 1;
    }
    return true;
  } else {
    return false;
  }
  
}

//------------------------------------------------------------------------------
RestRequest* parseClientBuffer() {
  
  // convert buffer into a string for further processing
  String s = String(buffer);

  // extract http method
  s.substring(0, s.indexOf(' ')).toCharArray(clientRequest.method, 8);;
  
  // reduce string to uris
  s = s.substring(s.indexOf('/'), s.indexOf(' ', s.indexOf('/')));
  
  // parse parameters from uri
  s.toCharArray(buffer, BUFFER_SIZE);
  String(strtok(buffer, "/")).toCharArray(clientRequest.command, 16);
  String(strtok(NULL, "/")).toCharArray(clientRequest.data, 32);

#if ECHO_TO_SERIAL
  Serial.print("http method: ");
  Serial.println(clientRequest.method);
  Serial.print("command: ");
  Serial.println(clientRequest.command);
  Serial.print("data: ");
  Serial.println(clientRequest.data);
#endif // ECHO_TO_SERIAL
  
  // return pointer to the rest request
  return &clientRequest;
}

