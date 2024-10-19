/*
  senseBox:home - Citizen Sensingplatform
  Version: lorav2_1.6.0 - MSPL x.x2 PV Power variant without OLED display
  Date: 2024-01-30
  Homepage: https://www.sensebox.de https://www.opensensemap.org || https://makerspace-partheland.de
  Author: Reedu GmbH & Co. KG & Makerspace Partheland e.V.
  Note: Sketch for senseBox:home LoRa MCU Edition
  Model: homeV2lora
  Email: support@sensebox.de
  Code is in the public domain.
  https://github.com/sensebox/node-sketch-templater
*/
#include <pv.h>
#include <vector>
#include <algorithm>
#include <LoraMessage.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <senseBoxIO.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_HDC1000.h>
#include <VEML6070.h>
#include <SDS011-select-serial.h>
#include <LTR329.h>
#include <Adafruit_DPS310.h>    // Air pressure new
#include <Adafruit_BMP280.h>    // Air pressure old
#include "TSL45315.h"           // Illuminance
#include <Adafruit_LC709203F.h> //Batteriestatus und Füllanzeige

#ifdef ENABLE_DEBUG
#define DEBUG(str) Serial.println(str)
#else
#define DEBUG(str)
#endif

// Version of the sketch (last digit is the version of the sketch)
uint8_t _version = VERSION + 1;

// Load sensors / instances
#ifdef HDC1080_CONNECTED
Adafruit_HDC1000 HDC = Adafruit_HDC1000();
float temperature = 0;
float humidity = 0;
#endif
#ifdef TSL45315_CONNECTED
uint32_t lux;
TSL45315 lightSensor;
#endif
#ifdef VEML6070_CONNECTED
VEML6070 VEML;
uint16_t uv = 0;
#endif
#ifdef SDS011_CONNECTED
SDS011 SDS(SDS_UART_PORT);
std::vector<float> pm10readings;
float pm10max, pm25max, pm10, pm25 = 0;
// float pm25readings[MEASUREMENTS];
std::vector<float> pm25readings;
#endif
#ifdef SOUNDLEVELMETER_CONNECTED
float soundlevelmax, soundmedian = 0;
// float soundReadings[MEASUREMENTS];
std::vector<float> soundReadings;
#endif
#ifdef DPS310_CONNECTED
Adafruit_DPS310 dps;
double pressure = 0;
float altitude = 0;
float caseTemperature = 0;
#endif
#ifdef BMP280_CONNECTED
Adafruit_BMP280 BMP;
double pressure = 0;
float altitude = 0;
float caseTemperature = 0;
#endif
#ifdef LC709203F_CONNECTED
Adafruit_LC709203F lc;
float soc = 0;
float voltage = 0;
float cellTemp = 0;
#endif
void os_getDevEui(uint8_t *buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getArtEui(uint8_t *buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevKey(uint8_t *buf) { memcpy_P(buf, APPKEY, 16); }

static osjob_t sendjob;
static osjob_t measurementjob;

// Modification for Makerspace Partheland
bool doNotSendCritical = false; // Do not send if received via downlink CMD

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = PIN_XB1_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {PIN_XB1_INT, PIN_XB1_INT, LMIC_UNUSED_PIN},
};

void initSensors()
{
  // Feinstaubsensor SDS011 an
  if (TxInterval > 0)
  {
    senseBoxIO.powerUART(true);
  }
  senseBoxIO.powerI2C(true);
  // Aufwärmphase Feinstaubsensor SDS011
  delay(1500); // Delay mind. 3000 minus was schon an delay im Code folgt...

  // Sensor initialization
  DEBUG(F("Initializing sensors..."));
#ifdef LC709203F_CONNECTED
  if (!lc.begin())
  {
    DEBUG(F("Couldnt find Adafruit LC709203F?\nMake sure a battery is plugged in!"));
#undef LC709203F_CONNECTED // while (1) delay(10);
  }
  else
  {
    DEBUG(F("Found LC709203F"));
    lc.setPackSize(LC709203F_APA_1000MAH);
    lc.setTemperatureMode(LC709203F_TEMPERATURE_THERMISTOR);
    lc.setThermistorB(3435);
    delay(500);
  }
#endif
  if (TxInterval > 0)
  {
#ifdef VEML6070_CONNECTED
    VEML.begin();
    delay(500);
#endif
#ifdef HDC1080_CONNECTED
    HDC.begin();
#endif
#ifdef BMP280_CONNECTED
    BMP.begin(0x76);
#endif
#ifdef TSL45315_CONNECTED
    lightSensor.begin();
#endif
#ifdef SDS011_CONNECTED
    SDS_UART_PORT.begin(9600);
#endif
#ifdef DPS310_CONNECTED
    dps.begin_I2C(0x76);
    dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
    dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
#endif
    DEBUG(F("Sensor initializing done!"));
  }
  else
  {
    DEBUG(F("Sensor initializing without LoRa TX done!"));
  }
}

void sendSensors2Sleep()
{
  if (TxInterval >= MEDIUM_INTERVAL_S || TxInterval == 0)
  { // Sensoren aus wenn TxIntervall größer 15 Minuten oder 0.
    senseBoxIO.powerUART(false);
    senseBoxIO.powerI2C(false);
    DEBUG(F("Sensors powered off"));
  }
  else
  {
    //
  }
}

int currentReading = 0;
void do_measurement(osjob_t *j)
{
  // Read sensor values
  DEBUG(F("Measuring..."));

  if (static_cast<unsigned int>(currentReading) >= MEASUREMENTS)
  {
    currentReading = 0;
  }
  //-----Temperature & Humidity-----//
#ifdef HDC1080_CONNECTED
  DEBUG(F(""));
  DEBUG(F("Temperature: "));
  temperature = HDC.readTemperature();
  DEBUG(temperature);

  DEBUG(F("Humidity: "));
  humidity = HDC.readHumidity();
  DEBUG(humidity);

#endif

//----Pressure ----//
//-----DPS310 Pressure-----//
#ifdef DPS310_CONNECTED
  sensors_event_t temp_event, pressure_event;
  dps.getEvents(&temp_event, &pressure_event);
  pressure = pressure_event.pressure;
  caseTemperature = temp_event.temperature;
  DEBUG(F("Pressure: "));
  DEBUG(pressure);

#endif
//-----BMP280 Pressure-----//
#ifdef BMP280_CONNECTED
  pressure = BMP.readPressure() / 100;
  altitude = BMP.readAltitude(1013.25); // 1013.25 = sea level pressure
  caseTemperature = BMP.readTemperature();
  DEBUG(F("Pressure: "));
  DEBUG(pressure);

#endif

//-----Lux-----//
#ifdef TSL45315_CONNECTED
  DEBUG(F(""));
  DEBUG(F("Illuminance: "));
  lux = lightSensor.getIlluminance();
  DEBUG(lux);
#endif

//-----UV intensity-----//
#ifdef VEML6070_CONNECTED
  DEBUG(F(""));
  DEBUG(F("UV: "));
  uv = VEML.getUV();
  DEBUG(uv);
#endif

//-----Particulate matter-----//
#ifdef SDS011_CONNECTED
  DEBUG(F("PM..."));
  uint8_t attempt = 0;
  while (attempt < 5)
  {
    bool error = SDS.read(&pm25, &pm10);
    if (!error)
    {
      DEBUG(pm10);
      DEBUG(pm25);
      if (pm10readings.size() <= MEASUREMENTS * 3) // better safe then sorry
      {
        pm25readings.push_back(pm25);
        pm10readings.push_back(pm10);
      }
      if (pm25 > pm25max)
      {
        pm25max = pm25;
      }
      if (pm10 > pm10max)
      {
        pm10max = pm10;
      }
      break;
    }else{
      DEBUG(F("Error reading PM values"));
      senseBoxIO.statusRed();
    }
    attempt++;
  }

#endif

//-----dB(A) Sound Level-----//
#ifdef SOUNDLEVELMETER_CONNECTED
  DEBUG(F("Sound..."));
  float v = analogRead(SOUNDMETERPIN) * (3.3 / 1024.0);
  float decibel = v * 50;
  if (decibel > soundlevelmax)
  {
    soundlevelmax = decibel;
  }
  if (soundReadings.size() <= MEASUREMENTS * 3) // better safe then sorry
  {
    soundReadings.push_back(decibel);
  }
   DEBUG(F("Sound: "));
   DEBUG(decibel);
#endif


// Akkusensor
#ifdef LC709203F_CONNECTED
  senseBoxIO.statusGreen();
  cellTemp = lc.getCellTemperature();
  soc = lc.cellPercent();
  voltage = lc.cellVoltage();
#endif

  currentReading++;
  if (!doNotSendCritical && TxInterval > 0)
  {
    os_setTimedCallback(&measurementjob, os_getTime() + sec2osticks(MEASUREMENT_INTERVAL), do_measurement);
  }
}

// Zähler für Messungen in bestimmten Spannungsbereichen
int highCounter = 0;
int mediumCounter = 0;
int minCounter = 0;
int veryLowCounter = 0;
int critCounter = 0; // ein letztes mal vor dem Abschalten senden
int needReset = false;

void do_send(osjob_t *j)
{
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    DEBUG(F("OP_TXRXPEND, not sending"));
  }
  else if (doNotSendCritical)
  {
    DEBUG(F("RX do not send received, not sending"));
    sendSensors2Sleep();
  }
  else
  {

    initSensors();
    LoraMessage message;

// Akkusensor
#ifdef LC709203F_CONNECTED
    senseBoxIO.statusGreen();

    // Bestimme das Sendeintervall basierend auf der Spannung
    if (voltage >= VOLTAGE_MEDIUM)
    {
      highCounter++;
      if (highCounter >= HIGH_MEASUREMENTS)
      {
        TxInterval = MAX_INTERVAL_S;
        MEASUREMENT_INTERVAL = 1;
        mediumCounter = 0;
        minCounter = 0;
        veryLowCounter = 0;
        critCounter = 0;
        if (needReset)
        {
          initSensors();
          needReset = false;
        }
      }
      else
      {
        // nix
      }
    }
    else if (voltage >= VOLTAGE_MEDIUM_LOW)
    {
      // Wenn die Spannung x mal in diesem Bereich gemessen wird, setze das Intervall auf 15 Minuten
      if (mediumCounter >= MEDIUM_MEASUREMENTS)
      {
        highCounter = 0;
        minCounter = 0;
        veryLowCounter = 0;
        critCounter = 0;
        TxInterval = MEDIUM_INTERVAL_S;
        MEASUREMENT_INTERVAL = TxInterval / MEASUREMENTS;
        if (needReset)
        {
          initSensors();
          needReset = false;
        }
      }
      else
      {
        mediumCounter++;
      }
    }
    else if (voltage >= VOLTAGE_LOW)
    {
      // Wenn die Spannung x mal in diesem Bereich gemessen wird, setze das Intervall auf 30 Minuten
      if (minCounter >= MIN_MEASUREMENTS)
      {
        highCounter = 0;
        mediumCounter = 0;
        veryLowCounter = 0;
        critCounter = 0;
        TxInterval = MIN_INTERVAL_S;
        MEASUREMENT_INTERVAL = TxInterval / MEASUREMENTS;
        if (needReset)
        {
          initSensors();
          needReset = false;
        }
      }
      else
      {
        minCounter++;
      }
    }
    else if (voltage >= VOLTAGE_VERY_LOW)
    {
      // Wenn die Spannung x mal in diesem Bereich gemessen wird, setze das Intervall auf 60 Minuten
      if (veryLowCounter >= VERY_LOW_MEASUREMENTS)
      {
        highCounter = 0;
        mediumCounter = 0;
        minCounter = 0;
        critCounter = 0;
        TxInterval = VERY_LOW_INTERVAL_S;
        MEASUREMENT_INTERVAL = TxInterval / MEASUREMENTS;
        if (needReset)
        {
          initSensors();
          needReset = false;
        }
      }
      else
      {
        veryLowCounter++;
      }
    }
    else
    {
      critCounter++;
      TxInterval = 0;
      needReset = true;
    }
#else
    voltage = 0;
    soc = 0;
    cellTemp = 0;
    senseBoxIO.statusRed();
    TxInterval = 0;
#endif
    if (TxInterval > 0 || critCounter == CRITICAL_MEASUREMENTS)
    {
      DEBUG(F("Sending..."));
      LoraMessage message;

//-----Temperature & Humidity-----//
#ifdef HDC1080_CONNECTED
    message.addUint16((temperature + 18) * 771);
    message.addHumidity(humidity);
#endif

//----Pressure ----//
//-----DPS310 Pressure-----//
#ifdef DPS310_CONNECTED
    message.addUint16((pressure - 300) * 81.9187);
#endif
//-----BMP280 Pressure-----//
#ifdef BMP280_CONNECTED
    message.addUint16((pressure - 300) * 81.9187);
#endif

//-----Lux-----//
#ifdef TSL45315_CONNECTED
    message.addUint8(lux % 255);
    message.addUint16(lux / 255);
#endif

//-----UV intensity-----//
#ifdef VEML6070_CONNECTED
    message.addUint8(uv % 255);
    message.addUint16(uv / 255);
#endif

//-----Particulate matter-----//
#ifdef SDS011_CONNECTED
    DEBUG(F("pm25readings size:"));
    DEBUG(pm25readings.size());

    float pm25median, pm10median = 0;

    if (pm25readings.size() >= MEASUREMENTS)
    {

      // calculate median of pm25readings

      std::sort(pm25readings.begin(), pm25readings.end());

      if (pm25readings.size() % 2 == 0)
      {
        DEBUG(F("calc median even"));
        pm25median = (pm25readings[pm25readings.size() / 2 - 1] + pm25readings[pm25readings.size() / 2]) / 2.0;
        pm25readings.clear();
      }
      else
      {
        DEBUG(F("calc median odd"));
        pm25median = pm25readings[pm25readings.size() / 2];
        pm25readings.clear();
      }
      // calculate median of pm10readings
      std::sort(pm10readings.begin(), pm10readings.end());

      if (pm10readings.size() % 2 == 0)
      {
        pm10median = (pm10readings[pm10readings.size() / 2 - 1] + pm10readings[pm10readings.size() / 2]) / 2.0;
        pm10readings.clear();
      }
      else
      {
        pm10median = pm10readings[pm10readings.size() / 2];
        pm10readings.clear();
      }
    }
    else
    {
      DEBUG(F("canot calc median"));
      pm25median = pm25max;
      pm10median = pm10max;
    }
    DEBUG(F("PM10_MED: "));
    DEBUG(pm10median);
    DEBUG(F("PM10_MAX: "));
    DEBUG(pm10max);
    DEBUG(F("PM2.5_MED: "));
    DEBUG(pm25median);
    DEBUG(F("PM2.5_MAX: "));
    DEBUG(pm25max);

    message.addUint16(pm10median * 10);
    message.addUint16(pm25median * 10);
#endif

//-----dB(A) Sound Level-----//
#ifdef SOUNDLEVELMETER_CONNECTED
    DEBUG(F("soundReadings size:"));
    DEBUG(soundReadings.size());
    if (soundReadings.size() >= MEASUREMENTS)
    {
      // calculate median of soundReadings
      std::sort(soundReadings.begin(), soundReadings.end());

      if (soundReadings.size() % 2 == 0)
      {
        soundmedian = (soundReadings[soundReadings.size() / 2 - 1] + soundReadings[soundReadings.size() / 2]) / 2.0;
        soundReadings.clear();
      }
      else
      {
        soundmedian = soundReadings[soundReadings.size() / 2];
        soundReadings.clear();
      }
    }
    else
    {
      soundmedian = soundlevelmax;
    }

    message.addUint16(soundmedian * 10);
#endif

//-----Case Temp-----//
#if defined BMP280_CONNECTED || defined DPS310_CONNECTED
    message.addUint16((caseTemperature + 18) * 771);
#endif

#ifdef LC709203F_CONNECTED
      DEBUG(F(""));
      DEBUG(F("SOC: "));
      DEBUG(soc);

      DEBUG(F("Volt: "));
      DEBUG(voltage);

      DEBUG(F("TempAkku: "));
      DEBUG(cellTemp); // TODO: do we need this presision?
      // message.addHumidity(soc); //old, we need more space for the new data 2bytes --> 1byte
      message.addUint8(soc); // TODO: payloadformatter fix
      message.addUint16((voltage + 18) * 771);
      message.addUint16((cellTemp + 18) * 771);
#endif

      // TX Intervall
      DEBUG(F("TX Interval: "));
      DEBUG(TxInterval);
      DEBUG(F("TX Interval M: "));
      float intervall_tx = TxInterval / 60;
      DEBUG(intervall_tx);
      // message.addUint16(intervall_tx); //old, we need more space for the new data 2bytes --> 1byte
      message.addUint8(intervall_tx);
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
      DEBUG(F("Packet queued"));
      sendSensors2Sleep();

    // send max
#ifdef SDS011_CONNECTED
    message.addUint16(pm10max * 10);
    message.addUint16(pm25max * 10);
#endif

#ifdef SOUNDLEVELMETER_CONNECTED
    message.addUint16(soundlevelmax * 10);
#endif


    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
    DEBUG(F("Packet queued"));

    pm10max = 0; // reset max values
    pm25max = 0;  // reset max values
    soundlevelmax = 0;  // reset max values

    }
    else
    {
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(VERY_LOW_INTERVAL_S), do_send);
      sendSensors2Sleep();
    }
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup()
{
#ifdef ENABLE_DEBUG
  Serial.begin(9600);
  while (!Serial)
    ;
#endif
  delay(3000);

  // RFM9X (LoRa-Bee) in XBEE1 Socket
  senseBoxIO.powerXB1(false); // power off to reset RFM9X
  delay(250);
  senseBoxIO.powerXB1(true); // power on
  delay(200);

  // Sensor initialization
  initSensors();


  DEBUG(F("Starting loop in 3 seconds."));
  delay(3000);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  do_measurement(&measurementjob);
  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop()
{
  os_runloop_once();
}

void onEvent(ev_t ev)
{
  DEBUG(os_getTime());
  switch (ev)
  {
  case EV_SCAN_TIMEOUT:
    DEBUG(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    DEBUG(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    DEBUG(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    DEBUG(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    DEBUG(F("EV_JOINING"));
    break;
  case EV_JOINED:
    DEBUG(F("EV_JOINED"));

    // Disable link check validation (automatically enabled
    // during join, but not supported by TTN at this time).
    LMIC_setLinkCheckMode(0);
    break;
  case EV_RFU1:
    DEBUG(F("EV_RFU1"));
    break;
  case EV_JOIN_FAILED:
    DEBUG(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    DEBUG(F("EV_REJOIN_FAILED"));
    break;
  case EV_TXCOMPLETE:
    DEBUG(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
    {
      DEBUG(F("Received ack"));
    }
    // TTN Downlink to SenseBox
    if (LMIC.dataLen)
    {
      DEBUG(F("Received "));
      DEBUG(LMIC.dataLen);
      DEBUG(F(" bytes of payload"));

      int receivedmsg = (LMIC.frame[LMIC.dataBeg] << 8) + LMIC.frame[LMIC.dataBeg + 1];
      DEBUG(receivedmsg);
      if (receivedmsg == 404)
      {
        // KILLSWITCH Bytes 0194
        doNotSendCritical = true;
        TxInterval = 0;
        senseBoxIO.statusRed();
        DEBUG("Received doNotSendCritical");
      }
      else if (receivedmsg == 200)
      {
        // Bytes 00C8
        doNotSendCritical = false;
        TxInterval = MAX_INTERVAL_S;

        DEBUG("Received do high TX Interval and do send");
      }
    }
    senseBoxIO.statusNone();
    // Schedule next transmission
    if (TxInterval > 0)
    {
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TxInterval), do_send);
    }
    else
    {
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(VERY_LOW_INTERVAL_S), do_send);
    }
    break;
  case EV_LOST_TSYNC:
    DEBUG(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    DEBUG(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    DEBUG(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    DEBUG(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    DEBUG(F("EV_LINK_ALIVE"));
    break;
  default:
    DEBUG(F("Unknown event"));
    DEBUG(ev);
    break;
  }
}