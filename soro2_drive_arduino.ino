#include <Servo.h>
#include <Ethernet.h>
#include <string.h>
#include <EthernetUdp.h> 

#include "constants.h"
#include "serialize.h"

#define PIN_WHEEL_FL 10
#define PIN_WHEEL_ML 3
#define PIN_WHEEL_BL 5
#define PIN_WHEEL_FR 11
#define PIN_WHEEL_MR 9
#define PIN_WHEEL_BR 6

#define DEGREE_MIN 10
#define DEGREE_MAX 170

byte eth_mac[] = { 0xDA, 0x2D, 0x6F, 0x9D, 0xFE, 0xEE };

char _buffer[20];
unsigned long _lastMessageTime = 0;
unsigned long _lastHeartbeatTime = 0;

EthernetUDP _eth;

Servo _servoFL, _servoML, _servoBL, _servoFR, _servoMR, _servoBR;

void stopDrive() 
{
  _servoFL.write(90);
  _servoML.write(90);
  _servoBL.write(90);
  _servoFR.write(90);
  _servoMR.write(90);
  _servoBR.write(90);
}

void setup() 
{
  Serial.begin(9600);
  _servoFL.attach(PIN_WHEEL_FL);
  _servoML.attach(PIN_WHEEL_ML);
  _servoBL.attach(PIN_WHEEL_BL);
  _servoFR.attach(PIN_WHEEL_FR);
  _servoMR.attach(PIN_WHEEL_MR);
  _servoBR.attach(PIN_WHEEL_BR);
  
  stopDrive();

  // Begin ethernet with specified MAC address
  Ethernet.begin(eth_mac);
  _eth.begin(SORO_NET_DRIVE_SYSTEM_PORT);
}

void loop() 
{
  // Check for incoming packet
  int packetSize = _eth.parsePacket();
  unsigned long now = millis();
  if (packetSize == 12)
  {
    Serial.println("Got message");
    _eth.read(_buffer, 12);
    _lastMessageTime = now;
    int16_t wheelFL = deserialize<uint16_t>(_buffer);
    int16_t wheelML = deserialize<uint16_t>(_buffer + 2);
    int16_t wheelBL = deserialize<uint16_t>(_buffer + 4);
    int16_t wheelFR = deserialize<uint16_t>(_buffer + 6);
    int16_t wheelMR = deserialize<uint16_t>(_buffer + 8);
    int16_t wheelBR = deserialize<uint16_t>(_buffer + 10);

    _servoFL.write((-wheelFL / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
    _servoML.write((-wheelML / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
    _servoBL.write((wheelBL / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
    _servoFR.write((-wheelFR / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
    _servoMR.write((-wheelMR / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
    _servoBR.write((wheelBR / 32766 * ((DEGREE_MAX - DEGREE_MIN) / 2)) + 90);
  }
  else {
    Serial.println("Got invalid message");
  }
  if (_lastMessageTime > now || _lastHeartbeatTime > now)
  {
    // Millis has overflowed
    _lastMessageTime = 0;
    _lastHeartbeatTime = 0;
  }
  if (now - _lastMessageTime > 500)
  {
    // No mesages received in .5 seconds
    stopDrive();
    Serial.println("Drive timeout");
  }
  if (now - _lastHeartbeatTime > 1000)
  {
    // Send heartbeat
    Serial.println("Sending heartbeat");
    _buffer[0] = SORO_HEADER_DRIVE_HEARTBEAT_MSG;
    _eth.beginPacket(_eth.remoteIP(), SORO_NET_DRIVE_SYSTEM_PORT);
    _eth.write(_buffer, 1);
    _eth.endPacket();
    _lastHeartbeatTime = now;
  }
  
}
