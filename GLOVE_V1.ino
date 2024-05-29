#define BLYNK_TEMPLATE_ID █████████████
#define BLYNK_TEMPLATE_NAME █████████
#define BLYNK_PRINT Serial
#define THERMALSENSORPIN 35 //Ambient temp sensor reading
#define LIGHTSENSORPIN 32 //Ambient light sensor reading

#define relayPin 25       // Pin where the relay is connected
#define ledPin 26         // Pin where the LED is connected

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// for PWM
const int pwmChannelLed = 0;   // PWM channel for LED (0-15)
const int pwmChannelRelay = 1; // PWM channel for Relay (0-15)
const int pwmFrequencyLed = 5000;  // PWM frequency for LED in Hz
const int pwmFrequencyRelay = 5000;  // PWM frequency for Relay /...? honestly idk
const int pwmResolution = 8;   // PWM resolution (8-bit: values from 0 to 255)

unsigned long previousMillis = 0; // Stores the last time the relay was updated
const long interval = 6000;       // Interval at which to update relay (1 minute -- 6 seconds)
float dutyCycl = 5000.0;          // Duty Cycle defaults at 5s so it doesnt break idk
int dutyCyclInt = 5000;           // actually should be 0 but idk


// for Blynk
char ssid[] = █████████████████;
char pass[] = █████████████████;

char auth[] = ████████████████████████████████;

// Define a flag to control the mode of operation
bool useDefaultLoop = true;

// idk where to put this
int temp = 0;
int light = 0; 

long pwmLightBlynk = 0;
long pwmTempBlynk = 255; //default 255 because LOW is closed xdd

// This function is called every time a Widget in the Blynk app writes a value to Virtual Pin V1
BLYNK_WRITE(V1) {
  int pinValue = param.asInt(); // Get value as integer
  if (pinValue == 0) {
    useDefaultLoop = true;
    Serial.println("Using default loop function.");
  } else {
    useDefaultLoop = false;
    Serial.println("Using Blynk exclusive function.");
  }
}

BLYNK_WRITE(V10) { //temp
  pwmTempBlynk = 255 - param.asInt(); // ??? 0 is high, flip it
}

BLYNK_WRITE(V11) { //light
  pwmLightBlynk = param.asInt();
}

void setup() {
  // Connect to Wi-Fi for Blynk
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // declare input pins for SENSORS
  pinMode(THERMALSENSORPIN, INPUT);
  pinMode(LIGHTSENSORPIN, INPUT);

  // Configure the LED PWM functionalities
  ledcSetup(pwmChannelLed, pwmFrequencyLed, pwmResolution);
  ledcAttachPin(ledPin, pwmChannelLed);

  // Configure the Relay PWM functionalities
  ledcSetup(pwmChannelRelay, pwmFrequencyRelay, pwmResolution);
  ledcAttachPin(relayPin, pwmChannelRelay);

  Serial.begin(9600);
  // Initialize Blynk
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  // Run Blynk
  Blynk.run();

  // min : ambient : max ; hopefully carry over to funtions...?
  temp = analogRead(THERMALSENSORPIN); //Read temps (lower = hotter ... ?)
  // experiment for temps. this is also called in for Relay Duty Cycle
  // for me its 2200 : 1900: 1800
  light = analogRead(LIGHTSENSORPIN); //Read light level (0.0 - 4095.0 in float... probably change to int?)
  // 0 : 200 ish : 4095 (probably root square it) (i didnt)
  // idk how the angles work but it just outputs 0 now on ambient, maybe works better in sunlight? 

  // // relic from another code
  // float square_ratio = reading / 4095.0; //Get percent of maximum value (1023)
  // square_ratio = pow(square_ratio, 2.0);
  Serial.print("Sensor value reading (temp/light): ");
  Serial.print(temp);
  Serial.print(" / ");
  Serial.println(light);
  Blynk.virtualWrite(V8, temp);
  Blynk.virtualWrite(V9, light);

  if (useDefaultLoop) {
    // Default loop code
    defaultLoopFunction();
  } else {
    // Exclusive input from Blynk
    blynkExclusiveFunction();
  }
}

void defaultLoopFunction() {
  // default will output with predetermined mapping
  int pwmTempValue = mapWithMidpoint(temp, 2200, 1900, 1800, 0, 128, 255);
  int pwmLightValue = mapWithMidpoint(light, 4095, 200, 0, 0, 128, 255); //using known mapping
  Serial.print("PWM value: ");
  Serial.print(pwmTempValue);
  Serial.print(" / ");
  Serial.println(pwmLightValue);

  // Output the PWM value to the LED
  ledcWrite(pwmChannelLed, pwmLightValue);

  // Check if 6 seconds has passed
  unsigned long currentMillis = millis();
  dutyCycl = ((255.0-(float)pwmTempValue)/255.0) * (float)interval;
  dutyCyclInt = (int)dutyCycl;
  Serial.println(dutyCyclInt);

  if (currentMillis - previousMillis >= dutyCyclInt) {
    // == not 6 seconds yet, but duty cycle already pass, so turn the relay off
    // Output the PWM value to the relay
    ledcWrite(pwmChannelRelay, 255); // 255 = 3.3V = off yes
  }
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the relay was updated == now its 6 seconds, reset previousMillis
    previousMillis = currentMillis;
    // Output the PWM value to the relay
    ledcWrite(pwmChannelRelay, pwmTempValue);
  }

  delay(1000); // faster sample rate (~100) for real world application/demonstration/debugging
}

void blynkExclusiveFunction() {
  Serial.print("PWM value (Blynk): ");
  Serial.print(pwmTempBlynk);
  Serial.print(" / ");
  Serial.println(pwmLightBlynk);

  // Output the PWM value to the LED
  ledcWrite(pwmChannelLed, pwmLightBlynk);

  // Check if 6 seconds has passed
  unsigned long currentMillis = millis();
  dutyCycl = ((255.0-(float)pwmTempBlynk)/255.0) * (float)interval;
  dutyCyclInt = (int)dutyCycl;
  Serial.println(dutyCycl);
  Serial.println(dutyCyclInt);

  if (currentMillis - previousMillis >= dutyCyclInt) {
    // == not 6 seconds yet, but duty cycle already pass, so turn the relay off
    // Output the PWM value to the relay
    ledcWrite(pwmChannelRelay, 255); // 255 = 3.3V = off yes
  }
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the relay was updated == now its 6 seconds
    previousMillis = currentMillis;
    // Output the PWM value to the relay
    ledcWrite(pwmChannelRelay, pwmTempBlynk);
  }
  
  // what else other than outputting variables from Blynk
  delay(1000); // faster sample rate (~100) for real world application/demonstration/debugging
}

long mapAdvanced(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long constrainX(long x, long out_min, long out_max) {
  if (x < out_min) return out_min;
  if (x > out_max) return out_max;
  return x;
}

long mapWithMidpoint(long x, long in_min, long in_mid, long in_max, long out_min, long out_mid, long out_max) {
  long result;
  if (in_min < in_max) { // Normal mapping
    if (x <= in_mid) {
      result = mapAdvanced(x, in_min, in_mid, out_min, out_mid);
    } else {
      result = mapAdvanced(x, in_mid, in_max, out_mid, out_max);
    }
  } else { // Inverted mapping
    if (x >= in_mid) {
      result = mapAdvanced(x, in_min, in_mid, out_min, out_mid);
    } else {
      result = mapAdvanced(x, in_mid, in_max, out_mid, out_max);
    }
  }
  return constrainX(result, out_min, out_max);
}

