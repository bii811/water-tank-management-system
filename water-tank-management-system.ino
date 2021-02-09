#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>

#define LED_BUILTIN 16
#define SENSOR  2

// Utrasonic
#define PINGPIN 5
#define INPIN 12

// Flow sensors
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;

bool isMonitorWaterFlow;
bool isMonitorWaterTankVolume;
bool isStartPump;

int pumpCountSec;
int pumpTimer = 10;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(PINGPIN, OUTPUT);
  pinMode(INPIN, INPUT);

  isMonitorWaterFlow = false;
  isMonitorWaterTankVolume = false;
  isStartPump = false;

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  main_menu();
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    if (isMonitorWaterFlow) {
      monitorWaterFlow(currentMillis, previousMillis);
    }

    if (isStartPump)
    {
      pumpOnWithDuration(pumpTimer);
    }

    if (isMonitorWaterTankVolume)
    {
      monitorTankVolume();
    }

    previousMillis = millis();
  }
}

void main_menu() {

  // Monitor
  // - Pump state
  // - Water Flow
  // - Water Tank Volume

  Serial.println("1. Monitor");
  Serial.println("2. Pump with Monitor");
  Serial.println("3. Monitor Water Tank Volume");
  Serial.println("4. Monitor All");

  // Manual
  // - Switch On/Off Pump
  // - Switch On/Off Pump with Timer
  // - Switch On/Off Pump with Water Volume

  // Automatic
  // - Automatic On/Off Pump with Scheduler Time

  while (!Serial) {}
  while (!Serial.available()) {}
  int read = Serial.parseInt();
  if (read == 1) {
    isMonitorWaterFlow = true;
  } else if (read == 2) {
    isMonitorWaterFlow = true;
    isStartPump = true;
    int pumpCountSec = 0;
  } else if (read == 3) {
    isMonitorWaterTankVolume = true;
  } else if (read == 4) {
    isMonitorWaterFlow = true;
    isMonitorWaterTankVolume = true;
  }
}

void monitorWaterFlow(long currentMillis, long previousMillis) {
  pulse1Sec = pulseCount;
  pulseCount = 0;

  // Because this loop may not complete in exactly 1 second intervals we calculate
  // the number of milliseconds that have passed since the last execution and use
  // that to scale the output. We also apply the calibrationFactor to scale the output
  // based on the number of pulses per second per units of measure (litres/minute in
  // this case) coming from the sensor.
  flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
  previousMillis = millis();

  // Divide the flow rate in litres/minute by 60 to determine how many litres have
  // passed through the sensor in this 1 second interval, then multiply by 1000 to
  // convert to millilitres.
  flowMilliLitres = (flowRate / 60) * 1000;
  flowLitres = (flowRate / 60);

  // Add the millilitres passed in this second to the cumulative total
  totalMilliLitres += flowMilliLitres;
  totalLitres += flowLitres;

  // Print the flow rate for this second in litres / minute
  Serial.print("Flow rate: ");
  Serial.print(float(flowRate));  // Print the integer part of the variable
  Serial.print("L/min");
  Serial.print("\t");       // Print tab space

  // Print the cumulative total of litres flowed since starting
  Serial.print("Output Liquid Quantity: ");
  Serial.print(totalMilliLitres);
  Serial.print("mL / ");
  Serial.print(totalLitres);
  Serial.println("L");
}


void pumpOnWithDuration(long sec_duration) {
  Serial.println("Pump starting...");
  if (pumpCountSec == sec_duration)
  {
    isStartPump = false;
    Serial.println("Pump stop.");
  }
  pumpCountSec++;
}

void monitorTankVolume() {
  long duration, cm;
  pinMode(PINGPIN, OUTPUT);
  digitalWrite(PINGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(PINGPIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(PINGPIN, LOW);
  pinMode(INPIN, INPUT);
  duration = pulseIn(INPIN, HIGH);
  cm = microsecondsToCentimeters(duration);
  float v = tankVolume(50, 40, 20, cm);
  Serial.print("Ultrasonic: ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.print(" Tank Volume: ");
  Serial.print(v);
  Serial.print(" cm3");
  Serial.println();
}


float volume_trancated_tank(int r_top, int r_bottom, int height) {
  float volume = 3.1415926535 / 3 * height * ((r_top * r_top) + (r_top * r_bottom) + (r_bottom *r_bottom));
  return volume;
}

float tankVolume(int r_top, int r_bottom, int height, int filled) {
  int d = (r_top - r_bottom) * filled / height;
  int h = filled;

  return volume_trancated_tank(r_bottom + d, r_bottom, h);
}

long microsecondsToCentimeters(long microseconds)
{
  // ความเร็วเสียงในอากาศประมาณ 340 เมตร/วินาที หรือ 29 ไมโครวินาที/เซนติเมตร
  // ระยะทางที่ส่งเสียงออกไปจนเสียงสะท้อนกลับมาสามารถใช้หาระยะทางของวัตถุได้
  // เวลาที่ใช้คือ ระยะทางไปกลับ ดังนั้นระยะทางคือ ครึ่งหนึ่งของที่วัดได้
  return microseconds / 29 / 2;
}
