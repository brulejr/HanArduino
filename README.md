HanArduino
==========

Based upon the Arduino hardware, HanArduino provides control of various home automation eqipment through a message-based, RESTful web service. 

This project is broken into three distict parts:
	* han_coordinator - the Arduino controller node that manages RESTful access and controls remote nodes through wireless messaging
	* han_remote_control - a preliminary Arduino remote node that manages a bank of eight relays for controlling irrigation valves
	* HanMessenger - a shared library based upon CmdMessenger that coordinates messaging between the controller and remote nodes of the wireless network
