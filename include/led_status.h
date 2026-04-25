#pragma once

#include <Arduino.h>
#include <senseBoxIO.h>
#if defined(ARDUINO_ARCH_SAMD)
#include <sam.h>
#endif

namespace LedStatus
{
// Blink-Timing in Sekunden (für bessere Lesbarkeit)
constexpr float kBlinkOnS = 0.8f;      // LED an - gut erkennbar für Menschen
constexpr float kBlinkOffS = 0.5f;     // Pause zwischen Blinks innerhalb eines Codes
constexpr float kBlinkPauseS = 3.0f;   // Pause zwischen Wiederholungen des Codes

// Umrechnung in Millisekunden für millis()
constexpr unsigned long kBlinkOnMs = static_cast<unsigned long>(kBlinkOnS * 1000);
constexpr unsigned long kBlinkOffMs = static_cast<unsigned long>(kBlinkOffS * 1000);
constexpr unsigned long kBlinkPauseMs = static_cast<unsigned long>(kBlinkPauseS * 1000);
constexpr unsigned long kErrorRestartTimeoutMs = 3600000UL; // 1 Stunde

enum ErrorFlag : uint8_t
{
  ERROR_LORA_INIT = 1 << 0,
  ERROR_SDS011 = 1 << 1,
  ERROR_LC709203F = 1 << 2
};

enum LedMode : uint8_t
{
  LED_MODE_SOLID_GREEN,
  LED_MODE_SOLID_RED,
  LED_MODE_GREEN_BLINK,
  LED_MODE_RED_BLINK
};

static uint8_t errorFlags = 0;
static uint8_t lastErrorFlags = 0;
static bool loraJoinInProgress = false;
static bool loraJoined = false;
static LedMode lastLedMode = LED_MODE_SOLID_GREEN;

static bool blinkOn = false;
static bool blinkInPause = false;
static unsigned long blinkLastChange = 0;
static uint8_t blinkPulseCount = 0;
static uint8_t blinkTargetCount = 0;
static bool blinkIsRed = false;
static bool lastLedState = false; // true = LED an, false = LED aus
static bool errorTimerActive = false;
static unsigned long errorFirstOccurredMs = 0;
static bool peripheryDisabledForError = false;

static void triggerSoftwareReset()
{
#if defined(ARDUINO_ARCH_SAMD)
  NVIC_SystemReset();
  while (true)
  {
  }
#endif
}

static void resetErrorTimer()
{
  errorTimerActive = false;
  errorFirstOccurredMs = 0;
}

// Schaltet alle Peripherie ab (XBee1, XBee2, I2C, UART)
// Nur die Status-LED bleibt für den Blinkcode aktiv
static void disablePeripheryForError()
{
  if (peripheryDisabledForError)
  {
    return; // Bereits abgeschaltet
  }
  
  // Alle Peripherie-Ports abschalten
  senseBoxIO.powerXB1(false);
  senseBoxIO.powerXB2(false);
  senseBoxIO.powerI2C(false);
  senseBoxIO.powerUART(false);
  
  peripheryDisabledForError = true;
#ifdef ENABLE_DEBUG
  Serial.println(F("Periphery disabled for error state"));
#endif
}

static void resetBlinkState()
{
  blinkOn = false;
  blinkInPause = false;
  blinkPulseCount = 0;
  blinkLastChange = millis();
  lastLedState = false;
}

static void setErrorFlag(uint8_t flag, bool active)
{
  uint8_t before = errorFlags;
  if (active)
  {
    errorFlags |= flag;
  }
  else
  {
    errorFlags &= static_cast<uint8_t>(~flag);
  }
  if (before != errorFlags)
  {
    resetBlinkState();
  }

  if (errorFlags == 0)
  {
    resetErrorTimer();
  }
  else if (!errorTimerActive)
  {
    // Erster Fehler: Timer starten (Peripherie wird später in update() abgeschaltet)
    errorTimerActive = true;
    errorFirstOccurredMs = millis();
  }
}

static void setJoinInProgress(bool inProgress)
{
  if (loraJoinInProgress != inProgress)
  {
    loraJoinInProgress = inProgress;
    resetBlinkState();
  }
}

static void setJoined(bool joined)
{
  if (loraJoined != joined)
  {
    loraJoined = joined;
    resetBlinkState();
  }
}

static bool hasMultipleErrors()
{
  return (errorFlags & static_cast<uint8_t>(errorFlags - 1)) != 0;
}

static uint8_t blinkCountForError(uint8_t flag)
{
  if (flag == ERROR_SDS011)
  {
    return 1;
  }
  if (flag == ERROR_LC709203F)
  {
    return 2;
  }
  if (flag == ERROR_LORA_INIT)
  {
    return 3;
  }
  return 1;
}

static void updateBlink(uint8_t blinksPerCycle, bool isRed)
{
  if (blinksPerCycle == 0)
  {
    return;
  }

  if (blinkTargetCount != blinksPerCycle || blinkIsRed != isRed)
  {
    blinkTargetCount = blinksPerCycle;
    blinkIsRed = isRed;
    resetBlinkState();
  }

  unsigned long now = millis();
  
  // In Pause zwischen Wiederholungen
  if (blinkInPause)
  {
    // LED muss aus sein
    if (lastLedState)
    {
      senseBoxIO.statusNone();
      lastLedState = false;
    }
    
    if (now - blinkLastChange >= kBlinkPauseMs)
    {
      // Pause vorbei, starte neuen Zyklus
      blinkInPause = false;
      blinkPulseCount = 0;
      blinkOn = true;
      blinkLastChange = now;
      if (blinkIsRed)
      {
        senseBoxIO.statusRed();
      }
      else
      {
        senseBoxIO.statusGreen();
      }
      lastLedState = true;
    }
    return;
  }

  // LED ist an
  if (blinkOn)
  {
    // LED muss an sein
    if (!lastLedState)
    {
      if (blinkIsRed)
      {
        senseBoxIO.statusRed();
      }
      else
      {
        senseBoxIO.statusGreen();
      }
      lastLedState = true;
    }
    
    // Zeit abgelaufen? Schalte aus
    if (now - blinkLastChange >= kBlinkOnMs)
    {
      blinkOn = false;
      blinkLastChange = now;
      blinkPulseCount++;
      senseBoxIO.statusNone();
      lastLedState = false;
      
      // War das der letzte Blink? Dann in Pause gehen
      if (blinkPulseCount >= blinkTargetCount)
      {
        blinkInPause = true;
      }
    }
    return;
  }

  // LED ist aus, warte Off-Zeit
  if (!blinkOn && !blinkInPause)
  {
    // LED muss aus sein
    if (lastLedState)
    {
      senseBoxIO.statusNone();
      lastLedState = false;
    }
    
    // Off-Zeit abgelaufen? Schalte an (wenn nicht alle Blinks fertig)
    if (now - blinkLastChange >= kBlinkOffMs)
    {
      if (blinkPulseCount < blinkTargetCount)
      {
        blinkOn = true;
        blinkLastChange = now;
        if (blinkIsRed)
        {
          senseBoxIO.statusRed();
        }
        else
        {
          senseBoxIO.statusGreen();
        }
        lastLedState = true;
      }
    }
  }
}

static bool hasErrors()
{
  return errorFlags != 0;
}

static void checkAndTriggerRestart()
{
  if (!errorTimerActive)
  {
    return;
  }

  unsigned long now = millis();
  if (now - errorFirstOccurredMs >= kErrorRestartTimeoutMs)
  {
    triggerSoftwareReset();
  }
}

static void logErrorFlags()
{
#ifdef ENABLE_DEBUG
  if (errorFlags == 0)
  {
    return;
  }
  if (errorFlags & ERROR_SDS011)
  {
    Serial.println("Error: SDS011 no data");
  }
  if (errorFlags & ERROR_LC709203F)
  {
    Serial.println("Error: LC709203F communication/voltage");
  }
  if (errorFlags & ERROR_LORA_INIT)
  {
    Serial.println("Error: LoRa init");
  }
#endif
}

static void update()
{
  if (errorFlags != lastErrorFlags)
  {
    resetBlinkState();
    lastErrorFlags = errorFlags;
    logErrorFlags();
  }

  if (errorFlags == 0)
  {
    resetErrorTimer();
    peripheryDisabledForError = false; // Reset für nächsten Fehler
    if (loraJoinInProgress && !loraJoined)
    {
      if (lastLedMode != LED_MODE_GREEN_BLINK)
      {
        resetBlinkState();
        lastLedMode = LED_MODE_GREEN_BLINK;
      }
      updateBlink(1, false);
    }
    else
    {
      lastLedMode = LED_MODE_SOLID_GREEN;
      senseBoxIO.statusGreen();
    }
    return;
  }

  if (hasMultipleErrors())
  {
    lastLedMode = LED_MODE_SOLID_RED;
    senseBoxIO.statusRed();
    disablePeripheryForError(); // Peripherie abschalten bei Fehler
    return;
  }

  uint8_t singleError = errorFlags;
  if (lastLedMode != LED_MODE_RED_BLINK)
  {
    resetBlinkState();
    lastLedMode = LED_MODE_RED_BLINK;
  }
  updateBlink(blinkCountForError(singleError), true);
  disablePeripheryForError(); // Peripherie abschalten bei Fehler
}
} // namespace LedStatus
