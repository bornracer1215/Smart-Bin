  #define BLYNK_TEMPLATE_ID "TMPL3kXN9dvfC"
#define BLYNK_TEMPLATE_NAME "oso"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 🔹 Blynk Authentication Token
char auth[] = "DQ9vl8CJ4zXs3xn-wWaLUPxzOtu5GHQO";

// 🔹 WiFi Credentials
char ssid[] = "Ishaan's jio phone";
char pass[] = "12345678";

// 🔹 OLED Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 🔹 Define GPIO Pins
const int inductiveSensorPin = 33;  // Inductive sensor
const int irSensorPin = 32;         // IR sensor
const int raindropSensorPin = 34;   // Raindrop sensor (ADC)
const int lidServoPin = 25;         // Servo motor for lid
const int binServoPin = 26;         // Servo motor for rotating bin

Servo lidServo;
Servo binServo;

// 🔹 Blynk Virtual Pins Assignments
#define V0_SERVO V0
#define V1_IR_SENSOR V1
#define V2_INDUCTIVE_SENSOR V2
#define V3_RAINDROP_SENSOR V3
#define V4_RECYCLABLE_LED V4
#define V5_WET_LED V5
#define V6_DRY_LED V6

//// 🔹 Define Bin Rotation Angles (hexagonal bin with 3 compartments)
//#define METAL_BIN_POS 0   // Position for Metal Waste
//#define WET_BIN_POS 120   // Position for Wet Waste
//#define DRY_BIN_POS 240   // Position for Dry Waste

void setup() {
  Serial.begin(115200);

  // 🔹 Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // 🔹 Connect to Blynk
  Blynk.begin(auth, ssid, pass);

  // 🔹 Initialize Sensors
  pinMode(inductiveSensorPin, INPUT);
  pinMode(irSensorPin, INPUT);
  
  lidServo.attach(lidServoPin, 500, 2500); // Attach lid servo (PWM range for ESP32)
  lidServo.write(90); // Start position at 0°
  
  binServo.attach(binServoPin, 500, 2500); // Attach bin rotation servo
  binServo.write(90);  // Stop rotation
  delay(500);
//  binServo.write(0); // Start position at 0°

  // 🔹 Initialize OLED Display
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed!");
    while (1);
  }
  display.clearDisplay();
}

// 🔹 Function to Update OLED Display
void updateOLED(String msg1, String msg2, String msg3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println(msg1);

  display.setCursor(0, 10);
  display.println(msg2);

  display.setCursor(0, 20);
  display.println(msg3);

  display.display();
}

// 🔹 Function to Rotate the Bin
void rotateBin(int position) {
  Serial.print("Rotating bin to position: ");
  Serial.println(position);
  binServo.write(position);
  delay(2000); // Wait for bin to rotate
}

void OpenOut(){
    Serial.println("🔄 Opening lid...");
    lidServo.write(180);  // Open lid
    delay(3000);
    Serial.println("🔄 Closing lid...");
    lidServo.write(0);   // Close lid
  }

void loop() {
  Blynk.run(); // Keep Blynk connected

  int irValue = digitalRead(irSensorPin);

  if (irValue == LOW) {  // Object detected
    Serial.println("🔹 Object Detected!");

    int inductiveValue = digitalRead(inductiveSensorPin);
    bool isMetal = (inductiveValue == HIGH); // LOW means metal detected

    int rainRaw = analogRead(raindropSensorPin);
    int rainPercentage = map(rainRaw, 0, 4095, 100, 0);

    Serial.println(isMetal ? "✅ Metal Detected!" : "❌ Non-Metal");
    Serial.print("🌧 Moisture Percentage: ");
    Serial.println(rainPercentage);

    // Reset to dry bin position
    binServo.write(90); // stop
    delay(300); // settle

    if (isMetal) {
      // Rotate to metal compartment (CCW)
      binServo.write(0);
      delay(175); // approx 120°
      binServo.write(90); // stop
      delay(500);
      OpenOut();
      delay(500);
      binServo.write(180);
      delay(175); // approx 120°
      binServo.write(90); // stop
      

    } else if (rainPercentage > 0) {
      // Rotate to wet compartment (CW)
      binServo.write(180);
      delay(175); // approx 120°
      binServo.write(90); // stop
      delay(500);
      OpenOut();
      delay(500);
      binServo.write(0);
      delay(175); // approx 120°
      binServo.write(90); // stop
    } else {
      // Already at dry (default) position
      OpenOut();
    }

    // LEDs
    Blynk.virtualWrite(V4_RECYCLABLE_LED, isMetal ? 255 : 0);
    Blynk.virtualWrite(V5_WET_LED, (rainPercentage > 0 && !isMetal) ? 255 : 0);
    Blynk.virtualWrite(V6_DRY_LED, (!isMetal && rainPercentage == 0) ? 255 : 0);

    // Data to Blynk
    Blynk.virtualWrite(V1_IR_SENSOR, irValue);
    Blynk.virtualWrite(V2_INDUCTIVE_SENSOR, inductiveValue);
    Blynk.virtualWrite(V3_RAINDROP_SENSOR, rainPercentage);

    // OLED
    String inductiveMsg = isMetal ? "Metal Detected!" : "Non-Metal";
    String rainMsg = "Moisture: " + String(rainPercentage) + "% Wet";
    updateOLED("Object Detected!", inductiveMsg, rainMsg);

  } else {
    Serial.println("No object detected.");
    updateOLED("No Object Detected", "", "");

    // Turn off all LEDs if no object is present
    Blynk.virtualWrite(V4_RECYCLABLE_LED, 0);
    Blynk.virtualWrite(V5_WET_LED, 0);
    Blynk.virtualWrite(V6_DRY_LED, 0);
  }

  delay(5000); // Check every 5 seconds
}
