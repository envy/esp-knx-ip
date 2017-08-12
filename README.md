# ESP-KNX-IP #

This is a library for the ESP8266 to enable KNXnet/IP communication. It uses UDP multicast on 224.0.23.12:3671.

## How to use ##

The library is under development. API may change multiple times in the future.

A simple example:

```c++
#include <esp-knx-ip.h>

const char* ssid = "my-ssid";  //  your network SSID (name)
const char* pass = "my-pw";    // your network password

int my_GA;

void setup()
{
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}

	knx.setup(); // Setup library

	// Register a callback that is called when a configurable group address is receiving a telegram
	knx.register_callback("callback_name", my_callback);

	// Register a configurable group address for sending out answers
	my_GA = knx.register_GA("answer GA");
}

void loop()
{
	knx.loop();
}


void my_callback(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
	switch (ct)
	{
	case KNX_CT_WRITE:
		// Do something, like a digitalWrite
		// Or send a telegram like this:
		uint8_t my_msg = 42;
		knx.send1ByteInt(knx.get_GA(my_GA), KNX_CT_WRITE, my_msg);
		break;
	case KNX_CT_READ:
		// Answer with a value
		knx.send1ByteInt(received_on, KNX_CT_ANSWER, 21);
		break;
	}
}
```

## How to configure ##

Simply visit the IP of your ESP with a webserver. You can configure the following:
* KNX physical adress
* Which group address should trigger which callback
* Which group address are to be used by the program (e.g. for status replies)