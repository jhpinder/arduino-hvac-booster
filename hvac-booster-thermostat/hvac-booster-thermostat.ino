#include <AM2302-Sensor.h>
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>

#define TURBO_PIN 2
#define TEMP_UP_PIN 3
#define TEMP_DOWN_PIN 4

#define FAN_OFF 0
#define FAN_MIN 1
#define FAN_MED 2
#define FAN_MAX 3
#define FAN_TBO 4

#define TURBO_CODE 0b1000101

constexpr unsigned int SENSOR_PIN {6U};
AM2302::AM2302_Sensor am2302{SENSOR_PIN};

byte minTemp = 60;
bool shouldSendTurbo;

long currentTemp = 70;
byte setTemp = 70;
byte fanSpeed = FAN_OFF;

const uint16_t thisNode = 01;
const uint16_t channel = 90;

void setup() {
  // put your setup code here, to run once:
  pinMode(7, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(TURBO_PIN, INPUT_PULLUP);
  pinMode(TEMP_UP_PIN, INPUT_PULLUP);
  pinMode(TEMP_DOWN_PIN, INPUT_PULLUP);
  digitalWrite(7, LOW);
  digitalWrite(5, HIGH);
  Serial.begin(115200);
   while (!Serial) {
      yield();
   }
   Serial.print(F("\n >>> AM2302-Sensor_Example <<<\n\n"));

      if (am2302.begin()) {
      // this delay is needed to receive valid data,
      // when the loop directly read again
      delay(3000);
   }
   else {
      while (true) {
      Serial.println("Error: sensor check. => Please check sensor connection!");
      delay(10000);
      }
   }
}

void loop() {
  network.update();
  while (network.available() ) {
    RF24NetworkHeader header;
    long incomingData;
    network.read(header, &incomingData, sizeof(incomingData));
    longToRoom(incomingData);
  }



   auto status = am2302.read();
   Serial.print("\n\nstatus of sensor read(): ");
   Serial.println(status);

   Serial.print("Temperature: ");
   Serial.println(am2302.get_Temperature() * 9 / 5 + 32);

   Serial.print("Humidity:    ");
   Serial.println(am2302.get_Humidity());
   delay(8000);

  shouldSendTurbo = shouldSendTurbo || (digitalRead(TURBO_PIN) ? 0 : TURBO_CODE);
}

void sendStatus() {
  long data = 0;
  data += thisNode << 24;
  data += 
  data += shouldSendTurbo;
  shouldSendTurbo = 0;
}

void handleUpButton() {
  if ()
  setTemp++;
  sendStatus();
}

void longToRoom(long signedData) {
  // most significant byte is the room number
  // next is unused
  // next is fan speed
  // least is min temperature
  unsigned long data = (unsigned long) signedData;
  if ((data & 0x7F000000) >> 24 != thisNode) {
    return;
  }
  fanSpeed = (data & 0xFF00) >> 8;
  minTemp = data & 0xFF;
}

void decodeMinTemp(byte &data) {
  data = map(data, 0, 127, 55, 87);
}
