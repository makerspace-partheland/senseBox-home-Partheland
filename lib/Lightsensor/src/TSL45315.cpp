#include "TSL45315.h"

/**
 * Constructor for the TSL45315 class.
 * Initializes the TSL45315 object with default values for lightsensortype, gain, integrationTime, and measurementRate.
 */
TSL45315::TSL45315() : lightsensortype(0), gain(1), integrationTime(0), measurementRate(3) {}

/**
 * Reads a register from the TSL45315 sensor.
 * 
 * @param address The I2C address of the sensor.
 * @param reg The register to read from.
 * @return The value read from the register.
 */
int TSL45315::read_reg(byte address, uint8_t reg) {
  int i = 0;

  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)address, (uint8_t)1);
  delay(1);
  if(Wire.available())
  {
    i = Wire.read();
  }
  return i;
}

/**
 * @brief Writes a value to a register of the TSL45315 sensor.
 * 
 * This function sends a command to the TSL45315 sensor to write a value to a specific register.
 * 
 * @param address The I2C address of the TSL45315 sensor.
 * @param reg The register address to write to.
 * @param val The value to write to the register.
 */
void TSL45315::write_reg(byte address, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

/**
 * @brief Initializes the TSL45315 light sensor.
 * 
 * This function initializes the TSL45315 light sensor by configuring its control and configuration registers.
 * If the sensor is detected as TSL45315, it sets the power on, configures the measurement rate, and sets the lightsensortype to 0.
 * If the sensor is not detected as TSL45315, it initializes the LTR329 light sensor, sets its control and measurement rate, powers it up, and sets the lightsensortype to 1.
 * 
 * @return void
 */
void TSL45315::begin() {
  Wire.begin();
  unsigned int u = 0;
  //DEBUG(F("Checking lightsensortype"));
  u = read_reg(0x29, 0x80 | 0x0A); //id register
  if ((u & 0xF0) == 0xA0)            // TSL45315
  {
   // DEBUG(F("TSL45315"));
    write_reg(0x29, 0x80 | 0x00, 0x03); //control: power on
    write_reg(0x29, 0x80 | 0x01, 0x02); //config: M=4 T=100ms
    delay(120);
    lightsensortype = 0; //TSL45315
  }
  else
  {
   // DEBUG(F("LTR329"));
    LTR.begin();
    LTR.setControl(gain, false, false);
    LTR.setMeasurementRate(integrationTime, measurementRate);
    LTR.setPowerUp(); //power on with default settings
    delay(10); //Wait 10 ms (max) - wakeup time from standby
    lightsensortype = 1;                     //
  }
}

/**
 * @brief Get the illuminance value from the light sensor.
 * 
 * This function retrieves the illuminance value from the light sensor.
 * If the lightsensor type is TSL45315, it reads the data from the sensor registers
 * and calculates the illuminance using the formula lux = u * 4, where u is the raw data.
 * If the lightsensor type is LTR-329ALS-01, it retrieves the data from the sensor using
 * the LTR class and calculates the illuminance based on the gain, integration time, and
 * raw data values.
 * 
 * @return The illuminance value in lux.
 */
unsigned int TSL45315::getIlluminance() {
  unsigned int lux = 0;
  if (lightsensortype == 0) // TSL45315
  {
    unsigned int u = (read_reg(0x29, 0x80 | 0x04) << 0);  //data low
    u |= (read_reg(0x29, 0x80 | 0x05) << 8); //data high
    lux = u * 4; // calc lux with M=4 and T=100ms
  }
  else if (lightsensortype == 1) //LTR-329ALS-01
  {
    delay(100);
    unsigned int data0, data1;
    for (int i = 0; i < 5; i++) {
      if (LTR.getData(data0, data1)) {
        if(LTR.getLux(gain, integrationTime, data0, data1, lux));
        //else DEBUG(F("LTR sensor saturated"));
        if(lux > 0) break;
        else delay(10);
      }
      else {
        //DEBUG(F("LTR getData error "));
        byte error = LTR.getError();
        Serial.println(error);
      }
    }
  }
  return lux;
}