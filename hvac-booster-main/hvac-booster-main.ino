#include <RF24Network.h>
#include <RF24Network_config.h>
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>
#include <math.h>
#include <util/atomic.h>


#define NUM_ROOMS 4 // four rooms upstairs, not going to boost airflow to the two bathrooms
#define TURBO_LEN 300000
#define TURBO_CODE 0b1000101
#define PROBE_NUM 10

#define FAN_OFF 0
#define FAN_MIN 1
#define FAN_MED 2
#define FAN_MAX 3
#define FAN_TBO 4

struct Room {
  byte temp; // 8 bit scale where 0 represents 55F and 127 represents 87F in increments of 0.25F
  byte setTemp;
  byte fanSpeed; 
  byte fanPins[3];
  long turboEnd;
};

RF24 radio(0, 0); // placeholder until network is mapped
RF24Network network(radio);

byte mainFloorTemp;
Room rooms[NUM_ROOMS];
bool blowerIsOn;
long readTempTime;
byte minTemp; // upstairs rooms should not be able to set temps more than 1F cooler than the downstairs temperature

const uint16_t thisNode = 00;
const uint16_t channel = 90;

void setup() {
  // put your setup code here, to run once:
  radio.begin();
  network.begin(channel, thisNode); // placeholder until network is mapped
}

void loop() {
  network.update();
  while (network.available() ) {
    RF24NetworkHeader header;
    long incomingData;
    network.read(header, &incomingData, sizeof(incomingData));
    longToRoom(incomingData);
  }
  readBlowerFanState();
  sendMinTemps();
  updateFanSpeeds();
  delay(10);
}


void sendMinTemps() {
  for (int roomNum = 1; roomNum <= NUM_ROOMS; roomNum++) {
    long data = roomToLong(roomNum);
    RF24NetworkHeader header(data);
    network.write(header, &data, sizeof(rooms[roomNum]));
  }
}

void readBlowerFanState() {
  // read both main floor temp and if blower is on, placeholders for now
  blowerIsOn = true;
}

void updateFanSpeeds() {
  for (int roomNum = 0; roomNum < NUM_ROOMS; roomNum++) {
    rooms[roomNum].fanSpeed = calcFanSpeed(roomNum);
    for (int fanSpeed = 0; fanSpeed < 3; fanSpeed++) {
      digitalWrite(rooms[roomNum].fanPins[fanSpeed], fanSpeed == rooms[roomNum].fanSpeed);
    }
  }
}

byte calcFanSpeed(byte roomNumber) {
  if (millis() - rooms[roomNumber].turboEnd < 0) {
    return FAN_TBO;
  }

  if (!blowerIsOn) {
    return FAN_OFF;
  }

  float temp = rooms[roomNumber].temp;
  float setTemp = rooms[roomNumber].setTemp;

  if (temp <= setTemp - 8) {
    return FAN_OFF;
  } else if (temp > setTemp - 2 && temp <= setTemp + 2) {
    return FAN_MIN;
  } else if (temp > setTemp + 4 && temp <= setTemp + 8) {
    return FAN_MED;
  } else if (temp > setTemp + 10) {
    return FAN_MAX;
  }

  return rooms[roomNumber].fanSpeed;
}

void longToRoom(long signedData) {
  // most significant byte is the room number
  // next is the temp
  // next is the set temp
  // least significant byte is activate turbo
  unsigned long data = (unsigned long) signedData;
  byte roomNum = ((data & 0x7F000000) >> 24) - 1;
  if (roomNum >= NUM_ROOMS) {
    if (roomNum == PROBE_NUM) {
      mainFloorTemp = (data & 0x00FF0000) >> 16;
      minTemp = mainFloorTemp - 4;
    }
    return;
  }
  rooms[roomNum].temp = (data & 0x00FF0000) >> 16;
  rooms[roomNum].setTemp = (data & 0x0000FF00) >> 8;
  if (data & TURBO_CODE) {
    resetMillis();
    rooms[roomNum].turboEnd = millis() + TURBO_LEN;
  }
}

long roomToLong(byte id) {
  // most significant byte is the room number
  // next is unused
  // next is fan speed
  // least is min temperature
  if (id < 1 && id > NUM_ROOMS) {
    return 0;
  }
  long data = 0;
  data += id << 24;
  data += rooms[id - 1].fanSpeed << 8;
  data += minTemp;
  return data;
}

void resetMillis() {
  for (int i = 0; i < NUM_ROOMS; i++) {
    if (rooms[i].turboEnd - millis() > 0) {
      return;
    }
  }
  extern unsigned long timer0_millis;
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
    timer0_millis = 0;
  }
}