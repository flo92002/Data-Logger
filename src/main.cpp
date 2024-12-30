#include <Wire.h>
#include <Arduino.h>

// ADS1115 I2C address
#define ADS1115_ADDRESS 0b1001000

// ADS1115 Register addresses
#define ADS1115_REG_CONVERSION 0b00000000
#define ADS1115_REG_CONFIG 0b00000001

// Gain settings
#define GAIN_TWOTHIRDS 0b0000000000000000 // +/- 6.144V
#define GAIN_ONE 0b0000001000000000       // +/- 4.096V (default)
#define GAIN_TWO 0b0000010000000000       // +/- 2.048V
#define GAIN_FOUR 0b0000011000000000      // +/- 1.024V
#define GAIN_EIGHT 0b0000100000000000     // +/- 0.512V
#define GAIN_SIXTEEN 0b0000101000000000   // +/- 0.256V

// Data rate settings
#define RATE_ADS1115_8SPS 0b0000000000000000   // 8 samples per second
#define RATE_ADS1115_16SPS 0b0000000000100000  // 16 samples per second
#define RATE_ADS1115_32SPS 0b0000000001000000  // 32 samples per second
#define RATE_ADS1115_64SPS 0b0000000001100000  // 64 samples per second
#define RATE_ADS1115_128SPS 0b0000000010000000 // 128 samples per second (default)
#define RATE_ADS1115_250SPS 0b0000000010100000 // 250 samples per second
#define RATE_ADS1115_475SPS 0b0000000011000000 // 475 samples per second
#define RATE_ADS1115_860SPS 0b0000000011100000 // 860 samples per second

// Set gain and data rate by modifying these variables
uint16_t gain = GAIN_TWO; // Change this to GAIN_TWOTHIRDS, GAIN_ONE, etc.
uint16_t dataRate = RATE_ADS1115_128SPS; // Change this to RATE_ADS1115_8SPS, RATE_ADS1115_16SPS, etc.

volatile bool conversionReady = false;
volatile uint8_t currentChannel = 0;
float voltages[2] = {0.0, 0.0};

void handleInterrupt() {
  conversionReady = true;
}

void writeRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(ADS1115_ADDRESS);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

uint16_t readRegister(uint8_t reg) {
  Wire.beginTransmission(ADS1115_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(ADS1115_ADDRESS, 2);
  return (Wire.read() << 8) | Wire.read();
}

void configureADS1115(uint8_t channel) {
  uint16_t config = 0b0000000000000000 | // Continuous conversion mode
                    gain |
                    dataRate;

  switch (channel) {
    case 0:
      config |= 0b0000000000000000; // Differential AIN0 and AIN1
      break;
    case 1:
      config |= 0b0011000000000000; // Differential AIN2 and AIN3
      break;
  }

  writeRegister(ADS1115_REG_CONFIG, config);
}

float readADCDifferential() {
  int16_t result = (int16_t)readRegister(ADS1115_REG_CONVERSION);

  // Calculate the voltage based on the gain setting
  float voltage = 0.0;
  switch (gain) {
    case GAIN_TWOTHIRDS:
      voltage = result * 0.0001875; // 6.144V / 32768
      break;
    case GAIN_ONE:
      voltage = result * 0.000125; // 4.096V / 32768
      break;
    case GAIN_TWO:
      voltage = result * 0.0000625; // 2.048V / 32768
      break;
    case GAIN_FOUR:
      voltage = result * 0.00003125; // 1.024V / 32768
      break;
    case GAIN_EIGHT:
      voltage = result * 0.000015625; // 0.512V / 32768
      break;
    case GAIN_SIXTEEN:
      voltage = result * 0.0000078125; // 0.256V / 32768
      break;
  }

  return voltage;
}

void setGain(uint16_t newGain) {
  gain = newGain;
}

void setDataRate(uint16_t newDataRate) {
  dataRate = newDataRate;
}

void setup() {
  // Initialize serial communication at the baud rate specified in platformio.ini
  Serial.begin(115200); // Change this to match the monitor_speed in platformio.ini
  Serial.println("ADS1115 test started");

  // Initialize I2C communication
  Wire.begin();

  // Configure ADS1115 for the first channel
  configureADS1115(currentChannel);
  Serial.println("ADS1115 initialized.");

  // Set Pin 2 as input for ADS1115 ready signal and attach interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), handleInterrupt, RISING);
}

void loop() {
  if (conversionReady) {
    conversionReady = false;

    // Read and store differential measurement for the current channel
    voltages[currentChannel] = readADCDifferential();

    // Switch to the next channel
    currentChannel = (currentChannel + 1) % 2;
    configureADS1115(currentChannel);

    // If both channels have been read, print the results
    if (currentChannel == 0) {
      Serial.print("Dif 1: ");
      Serial.print(voltages[0], 6); // Print voltage with 6 decimal places
      Serial.print(" Dif 2: ");
      Serial.print(voltages[1], 6); // Print voltage with 6 decimal places
      Serial.println();
    }
  }
}