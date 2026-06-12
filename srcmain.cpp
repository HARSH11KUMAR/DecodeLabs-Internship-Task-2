/**
 * @file main.cpp
 * @brief Autonomous Irrigation Controller - Closed-Loop Actuator Logic
 * @author DecodeLabs IoT Intern (Batch 2026)
 * * Hardware Logic Implementation:
 * - Exponential Moving Average (EMA) filtering for sensor stabilization
 * - Dual-threshold hysteresis implementation to prevent relay chatter
 * - Active-LOW safe bootup initialization to mitigate startup glitches
 */

#include <Arduino.h>

// ==========================================
// PIN CONFIGURATIONS
// ==========================================
const int SOIL_MOISTURE_PIN = 34;  // ADC1 channel pin (ESP32 GPIO34)
const int PUMP_RELAY_PIN    = 25;  // Digital output pin for relay control

// ==========================================
// CALIBRATION AND PARAMETERS
// ==========================================
// Field calibrated raw anchor values (12-bit ADC: 0 - 4095)
const int ADC_DRY = 3200; 
const int ADC_WET = 1200;

// Hysteresis limits (Percentage based)
const int HYSTERESIS_LOW_THRESHOLD  = 30; // Turn on pump below this value
const int HYSTERESIS_HIGH_THRESHOLD = 45; // Turn off pump above this value

// EMA Filter smoothing factor (Valid range: 0.0 to 1.0)
// Lower values = cleaner signal but introduces tracking delay. 0.15 is ideal for soil.
const float EMA_ALPHA = 0.15; 

// Loop execution interval 
const unsigned long SAMPLE_INTERVAL_MS = 200;

// ==========================================
// GLOBAL STATE VARIABLES
// ==========================================
float filteredAdcValue = 0.0; 
bool isPumpActive = false;

void setup() {
    // --------------------------------------------------------------------
    // SAFETY MECHANISM: ACTIVE-LOW RELAY BOOTUP TRAP
    // --------------------------------------------------------------------
    // Industrial opto-isolated relay boards switch on when driven LOW. 
    // Floating GPIO pins during bootup can cause random momentary pump firing.
    // We force the register state HIGH *before* changing the pin mode to OUTPUT.
    digitalWrite(PUMP_RELAY_PIN, HIGH); 
    pinMode(PUMP_RELAY_PIN, OUTPUT);
    
    Serial.begin(115200);
    delay(500); // Allow hardware rails to stabilize
    
    // Seed the initial filter state with a true reading to avoid ramp-up lag
    filteredAdcValue = analogRead(SOIL_MOISTURE_PIN);
    
    Serial.println(F("============================================="));
    Serial.println(F("DecodeLabs Cyber-Physical Closed-Loop System"));
    Serial.println(F("Status: Active-LOW Safe Initialization Complete"));
    Serial.println(F("============================================="));
}

void loop() {
    static unsigned long lastSampleTime = 0;
    
    // Maintain non-blocking execution pacing
    if (millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
        lastSampleTime = millis();
        
        // 1. Read the continuous physical characteristic
        int rawAdcSample = analogRead(SOIL_MOISTURE_PIN);
        
        // 2. Process through software signal filter (EMA)
        filteredAdcValue = (EMA_ALPHA * rawAdcSample) + ((1.0 - EMA_ALPHA) * filteredAdcValue);
        
        // 3. Normalize raw data into discrete percentage metrics
        // Note: Map function handles inverted slopes safely (Dry value > Wet value)
        long mappedMoisture = map((int)filteredAdcValue, ADC_DRY, ADC_WET, 0, 100);
        
        // Safety constraint block: Prevents unexpected math anomalies due to anomalous signal spikes
        int constrainedMoisture = constrain(mappedMoisture, 0, 100);
        
        // 4. Closed-Loop Hysteresis Logic Execution (State-Memory Control)
        if (constrainedMoisture < HYSTERESIS_LOW_THRESHOLD) {
            // State 1: Drought conditions detected
            isPumpActive = true;
        } 
        else if (constrainedMoisture > HYSTERESIS_HIGH_THRESHOLD) {
            // State 3: Satiated moisture level reached
            isPumpActive = false;
        }
        // State 2: Inside the Deadband (LOW_THRESHOLD <= moisture <= HIGH_THRESHOLD)
        // System ignores transients and safely holds its prior output state.

        // 5. High-Power Actuator Hardware Drive
        // Relay board uses active-low logic: LOW triggers coil, HIGH cuts circuit
        if (isPumpActive) {
            digitalWrite(PUMP_RELAY_PIN, LOW);  // Turn Pump ON
        } else {
            digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn Pump OFF
        }
        
        // Diagnostic Telemetry logging for testing verification
        Serial.print("RAW_ADC: ");      Serial.print(rawAdcSample);
        Serial.print(" | FILT_ADC: ");  Serial.print((int)filteredAdcValue);
        Serial.print(" | MOISTURE: ");  Serial.print(constrainedMoisture);
        Serial.print("% | PUMP: ");     Serial.println(isPumpActive ? "ON (IRRIGATING)" : "OFF (STANDBY)");
    }
}