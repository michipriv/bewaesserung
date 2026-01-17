// Filename: modules/pwm_control.h
// V1.0
// V1.0 Initial - PWM Pumpensteuerung entkoppelt

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#define PWM_CHANNEL     0
#define PWM_FREQ        1000
#define PWM_RESOLUTION  8

class PWMControl {
private:
    Preferences preferences;
    Adafruit_NeoPixel* pixels;
    uint8_t motorPin;
    uint8_t targetPWM;
    uint8_t activePWM;
    bool pumpRunning;

public:
    PWMControl(uint8_t pin, Adafruit_NeoPixel* led);
    void init();
    void setTargetPWM(uint8_t percent);
    void setPumpState(bool state);
    uint8_t getTargetPWM();
    uint8_t getActivePWM();
    bool isPumpRunning();

private:
    void applyPWM(uint8_t percent);
    void savePWMtoFlash(uint8_t percent);
    uint8_t loadPWMfromFlash();
};

//EOF
