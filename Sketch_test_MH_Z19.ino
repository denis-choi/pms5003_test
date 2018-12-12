#define INTERVAL 5000
#define MH_Z19_RX D4
#define MH_Z19_TX D5
#define WIFI_MAX_ATTEMPTS_INIT 3 //set to 0 for unlimited, do not use more then 65535
#define WIFI_MAX_ATTEMPTS_SEND 1 //set to 0 for unlimited, do not use more then 65535
#define MAX_DATA_ERRORS 15 //max of errors, reset after them
#define USE_GOOGLE_DNS true

#include <SoftwareSerial.h>


long previousMillis = 0;
int errorCount = 0;

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19

void(*resetFunc) (void) = 0; //declare reset function @ address 0


int readCO2()
{

	byte cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
	// command to ask for data
	byte response[9]; // for answer

	co2Serial.write(cmd, 9); //request PPM CO2

	// The serial stream can get out of sync. The response starts with 0xff, try to resync.
	while (co2Serial.available() > 0 && (unsigned char)co2Serial.peek() != 0xFF) {
		co2Serial.read();
	}

	memset(response, 0, 9);
	co2Serial.readBytes(response, 9);

	if (response[1] != 0x86)
	{
		Serial.println("Invalid response from co2 sensor!");
		return -1;
	}

	byte crc = 0;
	for (int i = 1; i < 8; i++) {
		crc += response[i];
	}
	crc = 255 - crc + 1;

	if (response[8] == crc) {
		int responseHigh = (int)response[2];
		int responseLow = (int)response[3];
		int ppm = (256 * responseHigh) + responseLow;
		return ppm;
	}
	else {
		Serial.println("CRC error!");
		return -1;
	}
}


void setup() {
	Serial.begin(115200); // Init console
	Serial.println("Setup started");

	unsigned long previousMillis = millis();
	co2Serial.begin(9600); //Init sensor MH-Z19(14)

	Serial.println("Waiting for sensors to init");

	while (millis() - previousMillis < 10000)
		delay(1000);

	Serial.println("Setup finished");
	Serial.println("");
}


void loop()
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis < INTERVAL)
		return;
	previousMillis = currentMillis;
	Serial.println("loop started");

	if (errorCount > MAX_DATA_ERRORS)
	{
		Serial.println("Too many errors, resetting");
		delay(2000);
		resetFunc();
	}
	Serial.println("reading data:");
	int ppm = readCO2();
	bool dataError = false;
	Serial.println("  PPM = " + String(ppm));

	if (ppm < 100 || ppm > 6000)
	{
		Serial.println("PPM not valid");
		dataError = true;
	}

	int mem = ESP.getFreeHeap();
	Serial.println("  Free RAM: " + String(mem));

	errorCount = 0;

	Serial.println("loop finished");
	Serial.println("");
}
