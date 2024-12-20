#pragma once

// This file is used to define the configuration of the sensebox
// The sensebox is a LoRaWAN device and the configuration is done in this file

#include <Arduino.h>
#include <avr/pgmspace.h>

// Uncomment the next line to get debugging messages printed on the Serial port
// Do not leave this enabled for long time use
// #define ENABLE_DEBUG

/* --- Sensebox Credentials and IDs ---- */
#define SENSEBOX_ID " "
//#define SENSEBOX_AUTH_TOKEN "Authentication token"
#define SENSEBOX_NAME "senseBox:home"

/* --- LoRaWAN Configuration ---- */
// DEVEUI in lsb format
static const PROGMEM uint8_t DEVEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// APPEUI in lsb format
static const PROGMEM uint8_t APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// APPKEY in msb format
static const PROGMEM uint8_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// The interval (in seconds) at which data is transmitted.
static const unsigned TX_INTERVAL = 120;
static const unsigned MEASUREMENTS = 30;        // Number of measurements per TX_INTERVAL
static const unsigned MEASUREMENT_INTERVAL = 1; 

// Angeschlossene Sensoren
// Temperatur & rel. Luftfeuchte - HDC1080
#define HDC1080_CONNECTED
// Beleuchtungsstärke - TSL45315
#define TSL45315_CONNECTED
// UV-Intensität - VEML6070
#define VEML6070_CONNECTED
// PM10 & PM2.5 - SDS 011
#define SDS011_CONNECTED
// Lautstärke - SOUNDLEVELMETER
#define SOUNDLEVELMETER_CONNECTED
// Luftdruck - DPS310 (neu) ODER BMP280 (alt)
#define DPS310_CONNECTED
//#define BMP280_CONNECTED


// Display enabled
// Uncomment the next line to get values of measurements printed on display
//#define DISPLAY128x64_CONNECTED


// Number of serial port the SDS011 is connected to. Either Serial1 or Serial2
#ifdef SDS011_CONNECTED
#define SDS_UART_PORT (Serial1)
#endif

#ifdef SOUNDLEVELMETER_CONNECTED
  #define SOUNDMETERPIN 1
#endif