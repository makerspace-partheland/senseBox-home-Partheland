/*
  senseBox:home - Citizen Sensingplatform
  Version: lora 2.2- MSPL x.x2 PV Power variant without OLED display
  Date: 2026-01-25
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
#include <led_status.h>

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
#define DEBUG_SECTION(label) Serial.println(label)
#define DEBUG_KV(label, value)       \
  do                                 \
  {                                  \
    Serial.print(label);             \
    Serial.println(value);           \
  } while (0)
#else
#define DEBUG(str)
#define DEBUG_SECTION(label)
#define DEBUG_KV(label, value)
#endif

#define LC709203F_CONNECTED

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
bool lcConnected = false;
#endif

// Measurement interval (seconds) derived from config, but adjustable at runtime.
unsigned measurementInterval = MEASUREMENT_INTERVAL;

// Zähler für Messungen in bestimmten Spannungsbereichen
int highCounter = 0;
int mediumCounter = 0;
int minCounter = 0;
int veryLowCounter = 0;
int critCounter = 0; // ein letztes mal vor dem Abschalten senden
int needReset = false;
void os_getDevEui(uint8_t *buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getArtEui(uint8_t *buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevKey(uint8_t *buf) { memcpy_P(buf, APPKEY, 16); }

static osjob_t sendjob;
static osjob_t measurementjob;

// Modification for Makerspace Partheland
bool doNotSendCritical = false; // Do not send if received via downlink CMD

static bool loraReady = false;
// Pin mapping
#if LORA_XBEE_PORT == 1
const lmic_pinmap lmic_pins = {
    .nss = PIN_XB1_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {PIN_XB1_INT, PIN_XB1_INT, LMIC_UNUSED_PIN},
};
#elif LORA_XBEE_PORT == 2
const lmic_pinmap lmic_pins = {
    .nss = PIN_XB2_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {PIN_XB2_INT, PIN_XB2_INT, LMIC_UNUSED_PIN},
};
#else
#error "LORA_XBEE_PORT must be 1 or 2"
#endif

static bool loraRadioPresent()
{
  SPI.begin();
  pinMode(lmic_pins.nss, OUTPUT);
  digitalWrite(lmic_pins.nss, HIGH);

  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(lmic_pins.nss, LOW);
  SPI.transfer(0x42 & 0x7F);
  uint8_t version = SPI.transfer(0x00);
  digitalWrite(lmic_pins.nss, HIGH);
  SPI.endTransaction();

  return (version == 0x12 || version == 0x22);
}

void initSensors()
{
  LedStatus::update();
  // Feinstaubsensor SDS011 an
  if (TxInterval > 0)
  {
    senseBoxIO.powerUART(true);
  }
  senseBoxIO.powerI2C(true);
  // Aufwärmphase Feinstaubsensor SDS011
  delay(1500); // Delay mind. 3000 minus was schon an delay im Code folgt...
  LedStatus::update();

  // Sensor initialization
  DEBUG(F("Initializing sensors..."));
#ifdef LC709203F_CONNECTED
  lcConnected = lc.begin();
  if (!lcConnected)
  {
    DEBUG(F("Couldnt find Adafruit LC709203F?\nMake sure a battery is plugged in!"));
    LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, true);
  }
  else
  {
    DEBUG(F("Found LC709203F"));
    lc.setPackSize(LC709203F_APA_1000MAH);
    lc.setTemperatureMode(LC709203F_TEMPERATURE_THERMISTOR);
    lc.setThermistorB(LC709203F_THERMISTOR_B);
    delay(500);
    LedStatus::update();
    cellTemp = lc.getCellTemperature();
    soc = lc.cellPercent();
    voltage = lc.cellVoltage();
    LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, voltage <= 0.01f);
    DEBUG_KV(F("Battery voltage: "), voltage);
    DEBUG_KV(F("Critical voltage: "), VOLTAGE_VERY_LOW);
    DEBUG_KV(F("Battery critical: "), TxInterval == 0 ? F("YES") : F("NO"));
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
  DEBUG_SECTION(F("=== SLEEP CHECK ==="));
  DEBUG_KV(F("Current TxInterval: "), TxInterval);
  DEBUG_KV(F("MEDIUM_INTERVAL_S: "), MEDIUM_INTERVAL_S);

  #ifdef LC709203F_CONNECTED
  if (lcConnected)
  {
    DEBUG_KV(F("Battery voltage: "), voltage);
    DEBUG_KV(F("Critical voltage: "), VOLTAGE_VERY_LOW);
    DEBUG_KV(F("Battery critical: "), TxInterval == 0 ? F("YES") : F("NO"));
  }
  #endif

  // Korrigierte Bedingung:
  // Sensoren nur abschalten, wenn:
  // 1. TxInterval >= MEDIUM_INTERVAL_S aber nicht MAX_INTERVAL_S (längeres Intervall) ODER
  // 2. TxInterval == 0 (kritischer Akkustand wurde bereits geprüft)
  if ((TxInterval >= MEDIUM_INTERVAL_S && TxInterval != MAX_INTERVAL_S) || TxInterval == 0)
  {
    // Sensoren aus wenn TxIntervall größer 15 Minuten oder kritischer Akkustand
    senseBoxIO.powerUART(false);
    senseBoxIO.powerI2C(false);
    if (TxInterval == 0)
    {
      DEBUG(F("Sensors powered off (critical battery, TxInterval=0)"));
    }
    else
    {
      DEBUG(F("Sensors powered off (TxInterval >= MEDIUM_INTERVAL_S)"));
    }
    #ifdef LC709203F_CONNECTED
    DEBUG_KV(F("Voltage: "), voltage);
    #endif
  }
  else
  {
    // Sensoren bleiben an, um kontinuierlich Max-Werte zu erfassen
    DEBUG(F("Sensors remain active for max value capture"));
    #ifdef LC709203F_CONNECTED
    DEBUG_KV(F("Voltage: "), voltage);
    DEBUG_KV(F("HighCounter: "), highCounter);
    #endif
  }
}

int currentReading = 0;
void do_measurement(osjob_t *j)
{
  LedStatus::update();
  if (LedStatus::hasErrors())
  {
  DEBUG(F("Error state: measurements stopped"));
    return;
  }
  // Read sensor values
  DEBUG_SECTION(F(""));
  DEBUG_SECTION(F("=== MEASUREMENT START ==="));

  if (static_cast<unsigned int>(currentReading) >= MEASUREMENTS)
  {
    currentReading = 0;
  }
  //-----Temperature & Humidity-----//
#if defined(HDC1080_CONNECTED) || defined(DPS310_CONNECTED) || defined(BMP280_CONNECTED)
  DEBUG_SECTION(F("--- ENVIRONMENT ---"));
#endif
#ifdef HDC1080_CONNECTED
  temperature = HDC.readTemperature();
  DEBUG_KV(F("Temperature: "), temperature);

  humidity = HDC.readHumidity();
  DEBUG_KV(F("Humidity: "), humidity);

#endif

//----Pressure ----//
//-----DPS310 Pressure-----//
#ifdef DPS310_CONNECTED
  sensors_event_t temp_event, pressure_event;
  dps.getEvents(&temp_event, &pressure_event);
  pressure = pressure_event.pressure;
  caseTemperature = temp_event.temperature;
  DEBUG_KV(F("Pressure: "), pressure);

#endif
//-----BMP280 Pressure-----//
#ifdef BMP280_CONNECTED
  pressure = BMP.readPressure() / 100;
  altitude = BMP.readAltitude(1013.25); // 1013.25 = sea level pressure
  caseTemperature = BMP.readTemperature();
  DEBUG_KV(F("Pressure: "), pressure);

#endif

//-----Case Temp-----//
#if defined BMP280_CONNECTED || defined DPS310_CONNECTED
  DEBUG_KV(F("CaseTemperature: "), caseTemperature);
#endif

//-----Lux-----//
#ifdef TSL45315_CONNECTED
  DEBUG_SECTION(F("--- LIGHT ---"));
  lux = lightSensor.getIlluminance();
  DEBUG_KV(F("Illuminance: "), lux);
#endif

//-----UV intensity-----//
#ifdef VEML6070_CONNECTED
  DEBUG_SECTION(F("--- UV ---"));
  uv = VEML.getUV();
  DEBUG_KV(F("UV: "), uv);
#endif

//-----Particulate matter-----//
#ifdef SDS011_CONNECTED
  DEBUG_SECTION(F("--- PARTICULATES ---"));
  uint8_t attempt = 0;
  bool pmReadOk = false;
  while (attempt < 10)  // Erhöht von 5 auf 10 Versuche
  {
    LedStatus::update();
    bool error = SDS.read(&pm25, &pm10);
    if (!error)
    {
      pmReadOk = true;
      DEBUG_KV(F("PM10: "), pm10);
      DEBUG_KV(F("PM2.5: "), pm25);
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
    }
    else
    {
      DEBUG(F("Error reading PM values"));
      delay(1000);  // 1 Sekunde Wartezeit zwischen den Versuchen für SDS011
    }
    attempt++;
  }
  LedStatus::setErrorFlag(LedStatus::ERROR_SDS011, !pmReadOk);
  LedStatus::update();
  if (!pmReadOk)
  {
    LedStatus::logErrorFlags();
    DEBUG(F("Error state: SDS011 -> measurements stopped"));
    return;
  }

#endif

//-----dB(A) Sound Level-----//
#ifdef SOUNDLEVELMETER_CONNECTED
  DEBUG_SECTION(F("--- SOUND ---"));
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
  DEBUG_KV(F("Sound: "), decibel);
#endif

// Akkusensor
#ifdef LC709203F_CONNECTED
  if (lcConnected)
  {
    cellTemp = lc.getCellTemperature();
    soc = lc.cellPercent();
    voltage = lc.cellVoltage();
    LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, voltage <= 0.01f);
    if (voltage <= 0.01f)
    {
      LedStatus::logErrorFlags();
      DEBUG(F("Error state: LC709203F voltage -> measurements stopped"));
      return;
    }
    DEBUG_SECTION(F("--- BATTERY ---"));
    DEBUG_KV(F("Battery voltage: "), voltage);
    DEBUG_KV(F("Battery temperature: "), cellTemp);
    DEBUG_KV(F("Critical voltage: "), VOLTAGE_VERY_LOW);
    DEBUG_KV(F("Battery critical: "), TxInterval == 0 ? F("YES") : F("NO"));
  }
  else
  {
    LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, true);
    LedStatus::logErrorFlags();
    DEBUG(F("Error state: LC709203F not connected -> measurements stopped"));
    return;
  }
#endif

  DEBUG_SECTION(F("=== MEASUREMENT END ==="));
  currentReading++;
  if (!doNotSendCritical && TxInterval == MAX_INTERVAL_S)
  {
    os_setTimedCallback(&measurementjob, os_getTime() + sec2osticks(measurementInterval), do_measurement);
  }
}


void do_send(osjob_t *j)
{
  DEBUG_SECTION(F("=== CYCLE START ==="));
  DEBUG_SECTION(F("=== SEND JOB ==="));
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    DEBUG(F("OP_TXRXPEND, not sending"));
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(60), do_send);
    return;
  }
  else if (LedStatus::hasErrors())
  {
    LedStatus::logErrorFlags();
    DEBUG(F("Error state: sending stopped"));
    return;
  }
  else if (doNotSendCritical)
  {
    DEBUG(F("RX doNotSend received, not sending"));
    sendSensors2Sleep();
    return;
  }
  else
  {
    // Bei kritischer Batteriespannung (TxInterval == 0) nicht sofort die Sensoren deaktivieren
    // Sensoren zuerst aktivieren, dann messen, dann senden und erst danach ggf. deaktivieren
    initSensors();
    if (TxInterval != MAX_INTERVAL_S)
    {
      // One-shot measurement before send for longer intervals.
      do_measurement(&measurementjob);
    }
    LoraMessage message;

// Akkusensor
#ifdef LC709203F_CONNECTED
    if (!lcConnected)
    {
      DEBUG(F("LC709203F not connected"));
      voltage = 0;
      soc = 0;
      cellTemp = 0;
      LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, true);
      TxInterval = 0;
    }
    else
    {
      // Bestimme das Sendeintervall basierend auf der Spannung
      if (voltage >= VOLTAGE_MEDIUM)
      {
        highCounter++;
        if (highCounter >= HIGH_MEASUREMENTS)
        {
          TxInterval = MAX_INTERVAL_S;
          measurementInterval = 1;
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
          measurementInterval = TxInterval / MEASUREMENTS;
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
          measurementInterval = TxInterval / MEASUREMENTS;
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
          measurementInterval = TxInterval / MEASUREMENTS;
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
        DEBUG(F("Voltage below critical threshold!"));
        DEBUG(F("Current voltage: "));
        DEBUG(voltage);
        DEBUG(F("Critical threshold: "));
        DEBUG(VOLTAGE_VERY_LOW);

        // Spannung nochmal prüfen - Sicherheitsabfrage mit neuer Messung
        const float recheckedVoltage = lc.cellVoltage();
        if (recheckedVoltage >= VOLTAGE_VERY_LOW)
        {
          DEBUG(F("WARNING: Voltage not critical! Likely sensor error."));
          DEBUG(F("Rechecked voltage: "));
          DEBUG(recheckedVoltage);
          DEBUG(F("Set TxInterval to MAX_INTERVAL_S"));
          voltage = recheckedVoltage;
          TxInterval = MAX_INTERVAL_S;
          highCounter = HIGH_MEASUREMENTS; // Erzwinge den hohen Status
          critCounter = 0;
          needReset = false;
        }
        else
        {
          TxInterval = 0;
          needReset = true;
        }
      }
    }
#else
    DEBUG(F("LC709203F not connected"));
    voltage = 0;
    soc = 0;
    cellTemp = 0;
    LedStatus::setErrorFlag(LedStatus::ERROR_LC709203F, true);
    TxInterval = 0;
#endif
    // Bei kritischem Akkustand, nur für CRITICAL_MEASUREMENTS senden, sonst warten
    if (TxInterval <= 0 && critCounter < CRITICAL_MEASUREMENTS)
    {
      DEBUG_SECTION(F("=== CRITICAL MODE ==="));
      DEBUG_KV(F("TxInterval: "), TxInterval);
      DEBUG_KV(F("CRITICAL_MEASUREMENTS: "), CRITICAL_MEASUREMENTS);
      DEBUG_KV(F("critCounter: "), critCounter);
      DEBUG_KV(F("Voltage: "), voltage);
      DEBUG_SECTION(F("=== END CRITICAL MODE ==="));
      // Bei kritischem Akkustand erst nach erfolgreicher Messung den Timer setzen
      // und erst danach die Sensoren in den Sleep-Modus versetzen
      do_measurement(&measurementjob); // Messung direkt durchführen
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(VERY_LOW_INTERVAL_S), do_send);
      sendSensors2Sleep(); // Hier Sensoren abschalten, da kritischer Akkustand
      return;
    }

    if (TxInterval > 0 || critCounter == CRITICAL_MEASUREMENTS)
    {
      DEBUG_SECTION(F("=== SEND START ==="));
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
      DEBUG_KV(F("pm25readings size: "), static_cast<unsigned int>(pm25readings.size()));

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
        DEBUG(F("canot calc median, not enough readings"));
        pm25median = pm25max;
        pm10median = pm10max;
        // Only accumulate readings at MAX_INTERVAL_S (full battery, continuous measurements)
        // At all other intervals: clear readings, use only current measurement
        if (TxInterval != MAX_INTERVAL_S)
        {
          DEBUG(F("Not MAX_INTERVAL_S: clearing readings"));
          pm25readings.clear();
          pm10readings.clear();
        }
      }
      DEBUG_KV(F("PM10_MED: "), pm10median);
      DEBUG_KV(F("PM10_MAX: "), pm10max);
      DEBUG_KV(F("PM2.5_MED: "), pm25median);
      DEBUG_KV(F("PM2.5_MAX: "), pm25max);

      message.addUint16(pm10median * 10);
      message.addUint16(pm25median * 10);
#endif

//-----dB(A) Sound Level-----//
#ifdef SOUNDLEVELMETER_CONNECTED
      DEBUG_KV(F("soundReadings size: "), static_cast<unsigned int>(soundReadings.size()));
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
        DEBUG(F("canot calc sound median, not enough readings"));
        soundmedian = soundlevelmax;
        // Only accumulate readings at MAX_INTERVAL_S (full battery, continuous measurements)
        // At all other intervals: clear readings, use only current measurement
        if (TxInterval != MAX_INTERVAL_S)
        {
          DEBUG(F("Not MAX_INTERVAL_S: clearing sound readings"));
          soundReadings.clear();
        }
      }

      message.addUint16(soundmedian * 10);
#endif

//-----Case Temp-----//
#if defined BMP280_CONNECTED || defined DPS310_CONNECTED
      message.addUint16((caseTemperature + 18) * 771);
#endif

#ifdef LC709203F_CONNECTED
      if (lcConnected)
      {
        DEBUG_SECTION(F("--- BATTERY (SEND) ---"));
        DEBUG_KV(F("SOC: "), soc);
        DEBUG_KV(F("Volt: "), voltage);
        DEBUG_KV(F("TempBattery: "), cellTemp);
        // message.addHumidity(soc); //old, we need more space for the new data 2bytes --> 1byte
        message.addUint8(soc);
        message.addUint16((voltage + 18) * 771);
        message.addUint16((cellTemp + 18) * 771);
      }
#endif

      // send max
#ifdef SDS011_CONNECTED
      message.addUint16(pm10max * 10);
      message.addUint16(pm25max * 10);
#endif

#ifdef SOUNDLEVELMETER_CONNECTED
      message.addUint16(soundlevelmax * 10);
#endif

      // TX Intervall
      DEBUG_KV(F("TX Interval: "), TxInterval);
      float intervall_tx = TxInterval / 60;
      DEBUG_KV(F("TX Interval M: "), intervall_tx);
      // message.addUint16(intervall_tx); //old, we need more space for the new data 2bytes --> 1byte
      message.addUint8(intervall_tx);
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
      DEBUG_SECTION(F("=== PACKET QUEUED ==="));

      pm10max = 0;       // reset max values
      pm25max = 0;       // reset max values
      soundlevelmax = 0; // reset max values
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

#if LORA_XBEE_PORT == 1
  #if LORA_POWER_OFF_UNUSED_PORT
  senseBoxIO.powerXB2(false); // power off unused XBEE2 port
  #endif
  delay(250);
  // RFM9X (LoRa-Bee) in XBEE1 Socket
  senseBoxIO.powerXB1(false); // power off to reset RFM9X
  delay(250);
  senseBoxIO.powerXB1(true); // power on
  delay(200);
#elif LORA_XBEE_PORT == 2
  #if LORA_POWER_OFF_UNUSED_PORT
  senseBoxIO.powerXB1(false); // power off unused XBEE1 port
  #endif
  delay(250);
  // RFM9X (LoRa-Bee) in XBEE2 Socket
  senseBoxIO.powerXB2(false); // power off to reset RFM9X
  delay(250);
  senseBoxIO.powerXB2(true); // power on
  delay(200);
#endif

  // Sensor initialization
  initSensors();

  DEBUG(F("Starting loop in 3 seconds."));
  delay(3000);

  loraReady = loraRadioPresent();
  LedStatus::setErrorFlag(LedStatus::ERROR_LORA_INIT, !loraReady);
  if (loraReady)
  {
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    do_measurement(&measurementjob);
    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
  }
  else
  {
    DEBUG(F("LoRa init failed: radio not detected"));
  }
}

void loop()
{
  LedStatus::update();
  if (LedStatus::hasErrors())
  {
    LedStatus::checkAndTriggerRestart();
    return;
  }
  if (loraReady)
  {
    os_runloop_once();
  }
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
    LedStatus::setJoinInProgress(true);
    LedStatus::setJoined(false);
    break;
  case EV_JOINED:
    DEBUG(F("EV_JOINED"));
    LedStatus::setJoinInProgress(false);
    LedStatus::setJoined(true);

    // Disable link check validation (automatically enabled
    // during join, but not supported by TTN at this time).
    LMIC_setLinkCheckMode(0);
    break;
  case EV_RFU1:
    DEBUG(F("EV_RFU1"));
    break;
  case EV_JOIN_FAILED:
    DEBUG(F("EV_JOIN_FAILED"));
    LedStatus::setJoinInProgress(false);
    LedStatus::setJoined(false);
    break;
  case EV_REJOIN_FAILED:
    DEBUG(F("EV_REJOIN_FAILED"));
    LedStatus::setJoinInProgress(false);
    LedStatus::setJoined(false);
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
      DEBUG_KV(F("Empfangen Bytes: "), LMIC.dataLen);

      int receivedmsg = (LMIC.frame[LMIC.dataBeg] << 8) + LMIC.frame[LMIC.dataBeg + 1];
      DEBUG(receivedmsg);
      if (receivedmsg == 404)
      {
        // KILLSWITCH Bytes 0194
        doNotSendCritical = true;
        TxInterval = 0;
        DEBUG(F("Received doNotSendCritical"));
      }
      else if (receivedmsg == 200)
      {
        // Bytes 00C8
        doNotSendCritical = false;
        TxInterval = MAX_INTERVAL_S;

        DEBUG(F("Received do high TX Interval and do send"));
      }
    }

    // ÄNDERUNG: Sensoren erst nach erfolgreichem Senden deaktivieren
    // Hinweis: sendSensors2Sleep() prüft intern, ob Sensoren abgeschaltet werden sollen
    sendSensors2Sleep();

    // Schedule next transmission
    if (TxInterval > 0)
    {
      // Bei normalem Akku: Planmäßiges Senden nach TxInterval
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TxInterval), do_send);
      // Continuous measurements only at MAX_INTERVAL_S for max/median
      if (TxInterval == MAX_INTERVAL_S) {
        do_measurement(&measurementjob);
      }
    }
    else
    {
      // Bei kritischem Akku: Senden nach VERY_LOW_INTERVAL_S
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(VERY_LOW_INTERVAL_S), do_send);
      // Bei kritischem Akku keine kontinuierlichen Messungen durchführen
    }
    DEBUG_SECTION(F("=== CYCLE END ==="));
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
