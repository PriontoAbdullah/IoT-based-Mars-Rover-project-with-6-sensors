
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>
#include <DHT.h>

WidgetLCD lcd(V0);

char auth[] = "Di-lhiRCHjcnAP-DRwdGooe-GTL16dIc"; //Enter the Auth code which was send by Blink

char ssid[] = "AndroidAP7675";  //Enter your WIFI Name
char pass[] = "naim9239";  //Enter your WIFI Password

int n;

#define sensorPin 0 // for D3 water sensor
// NodeMCU Pin D0 > TRIGGER | Pin D1 > ECHO for ultrasonic sensor
#define trigger 16
#define echo 5


#define DHTPIN 2          // Digital pin 4 DHT 11 sensor

#define DHTTYPE DHT11     // DHT 11


DHT dht(DHTPIN, DHTTYPE);
SimpleTimer timer;


void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V2, h);  //V5 is for Humidity
  Blynk.virtualWrite(V3, t);  //V6 is for Temperature
}

// for gas
void sendUptime()
{
  Blynk.virtualWrite(V1, n);
}



void setup()
{
  Serial.begin(9600); // See the connection status in Serial Monitor
  Blynk.begin(auth, ssid, pass);
  
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  dht.begin();

   pinMode(sensorPin, INPUT);

  // Setup a function to be called every second for Temp & Hhdty
  timer.setInterval(1000L, sendSensor);
  // Setup a function to be called every second for gas
  timer.setInterval(1000L, sendUptime);
}

void loop()
{
   Blynk.run(); // Initiates Blynk
  timer.run(); // Initiates SimpleTimer

  // for gass
  n = analogRead(A0); // for MQ-02 gas sensor
  Serial.println(n);


  // for ultrasonic sensor
  long duration, distance;
  digitalWrite(trigger, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(trigger, LOW);
  duration = pulseIn(echo, HIGH);
  //distance = (duration/2) / 29.1;
  distance = ((duration * 0.034) / 2); 
  
  Serial.print(distance);
  Serial.println("Centimeter:");
  Blynk.virtualWrite(V4, distance);


  // for water sensor
//  int  sensorState;
//  sensorState = digitalRead(sensorPin);
 // Serial.println(sensorState);

//  if (sensorState == 1)
  // {
    // lcd.clear();
    // lcd.print(0, 0, "Water");

   //  Serial.println("Water");
  // }

  delay(100);

}
