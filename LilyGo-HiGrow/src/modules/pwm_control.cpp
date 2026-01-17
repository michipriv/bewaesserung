// Filename: modules/pwm_control.cpp
// V1.0
// V1.0 Initial - PWM Pumpensteuerung entkoppelt

#include "pwm_control.h"

//*********************************
// Konstruktor
//*********************************
PWMControl::PWMControl(uint8_t pin, Adafruit_NeoPixel* led) 
    : motorPin(pin), pixels(led), targetPWM(0), activePWM(0), pumpRunning(false) {
}

//*********************************
// Initialisierung PWM und Flash
//*********************************
void PWMControl::init() {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(motorPin, PWM_CHANNEL);
    
    targetPWM = loadPWMfromFlash();
    pumpRunning = false;
    activePWM = 0;
    applyPWM(0);
    
    Serial.print("PWM Init: Target=");
    Serial.print(targetPWM);
    Serial.println("%, Pump=OFF");
}

//*********************************
// Setzt Ziel-PWM (unabhängig vom Pumpenstatus)
//*********************************
void PWMControl::setTargetPWM(uint8_t percent) {
    if (percent > 100) percent = 100;
    
    targetPWM = percent;
    savePWMtoFlash(percent);
    
    if (pumpRunning) {
        applyPWM(targetPWM);
    }
    
    Serial.print("Target PWM: ");
    Serial.print(percent);
    Serial.println("%");
}

//*********************************
// Pumpe EIN/AUS (verwendet targetPWM)
//*********************************
void PWMControl::setPumpState(bool state) {
    pumpRunning = state;
    
    if (pumpRunning) {
        applyPWM(targetPWM);
    } else {
        applyPWM(0);
    }
    
    Serial.print("Pump: ");
    Serial.println(state ? "ON" : "OFF");
}

//*********************************
// Getter
//*********************************
uint8_t PWMControl::getTargetPWM() {
    return targetPWM;
}

uint8_t PWMControl::getActivePWM() {
    return activePWM;
}

bool PWMControl::isPumpRunning() {
    return pumpRunning;
}

//*********************************
// Wendet PWM physisch an
//*********************************
void PWMControl::applyPWM(uint8_t percent) {
    if (percent > 100) percent = 100;
    
    activePWM = percent;
    uint8_t pwmValue = map(percent, 0, 100, 0, 255);
    ledcWrite(PWM_CHANNEL, pwmValue);
    
    if (pixels) {
        if (percent > 0) {
            uint8_t brightness = map(percent, 0, 100, 50, 255);
            pixels->setPixelColor(0, pixels->Color(0, brightness, 0));
        } else {
            pixels->setPixelColor(0, 0);
        }
        pixels->show();
    }
}

//*********************************
// Flash Speicher
//*********************************
void PWMControl::savePWMtoFlash(uint8_t percent) {
    preferences.begin("pump", false);
    preferences.putUChar("pwm", percent);
    preferences.end();
}

uint8_t PWMControl::loadPWMfromFlash() {
    preferences.begin("pump", true);
    uint8_t saved = preferences.getUChar("pwm", 50);
    preferences.end();
    
    Serial.print("Loaded PWM: ");
    Serial.print(saved);
    Serial.println("%");
    
    return saved;
}

//EOF
