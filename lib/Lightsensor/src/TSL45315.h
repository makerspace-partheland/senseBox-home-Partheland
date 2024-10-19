#ifndef TSL45315_H
#define TSL45315_H

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <LTR329.h>
#include <Wire.h>

/**
 * @class TSL45315
 * @brief Class representing the TSL45315 light sensor.
 * 
 * This class provides methods to interact with the TSL45315 light sensor.
 * It allows initializing the sensor, reading the illuminance value, and configuring sensor settings.
 */
class TSL45315 {
public:
  /**
   * @brief Default constructor for TSL45315 class.
   */
  TSL45315();

  /**
   * @brief Initializes the TSL45315 light sensor.
   */
  void begin();

  /**
   * @brief Gets the illuminance value from the TSL45315 light sensor.
   * @return The illuminance value in unsigned int.
   */
  unsigned int getIlluminance();

private:
  bool lightsensortype; //0 for tsl , 1 for ltr
  LTR329 LTR;
  unsigned char gain; //adjust sensor sensitivity based on current light conditions
  unsigned char integrationTime; //adjust how long the sensor collects light
  unsigned char measurementRate;  //adjust how frequently the sensor takes measurements

  /**
   * @brief Reads a register value from the TSL45315 light sensor.
   * @param address The I2C address of the sensor.
   * @param reg The register address to read from.
   * @return The value read from the register as an int.
   */
  int read_reg(byte address, uint8_t reg);

  /**
   * @brief Writes a value to a register of the TSL45315 light sensor.
   * @param address The I2C address of the sensor.
   * @param reg The register address to write to.
   * @param val The value to write to the register.
   */
  void write_reg(byte address, uint8_t reg, uint8_t val);
};

#endif