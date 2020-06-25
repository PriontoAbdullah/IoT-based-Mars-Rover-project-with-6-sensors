
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>
#include <DHT.h>


WidgetLCD lcd(V0);
WidgetLCD kcd(V2);

char auth[] = "UsnJp9J8oirbk_9FeXMqbFXAwQ8HvEV6"; //Enter the Auth code which was send by Blink


char ssid[] = "Net Nai";  //Enter your WIFI Name
char pass[] = "12345678900";  //Enter your WIFI Password

#define DHTPIN 5          // Digital pin 1


#define DHTTYPE DHT11     // DHT 11


DHT dht(DHTPIN, DHTTYPE);
SimpleTimer timer;

// for color sensor
#define S0 14
#define S1 12
#define S2 13
#define S3 15
#define OUT 16

int R,G,B = 0;



// for Soil Moisture Pin difine
const int sensorPin = 4; 
int sensorState = 0;
int lastState = 0;


void sendSensor()
{
  
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V5, h);  //V5 is for Humidity
  Blynk.virtualWrite(V6, t);  //V6 is for Temperature
}


void setup()
{

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);

  
  // Setting frequency-scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);

//-------------------------
  
  Serial.begin(9600); // See the connection status in Serial Monitor
  Blynk.begin(auth, ssid, pass);

  

  dht.begin();

  pinMode(sensorPin, INPUT);

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
}

void loop()
{
  Blynk.run(); // Initiates Blynk
  timer.run(); // Initiates SimpleTimer

// for Soil Moisture

float soil = analogRead(A0);
Serial.println(soil);
Serial.println("_");
Blynk.virtualWrite(V1, soil);

if(soil>=1000)
{
  lcd.clear();
    lcd.print(0,0,"Disconnected");

  Serial.println("Disconnected");
}
else if (soil<1000 && soil>=601)
{

    lcd.clear();
    lcd.print(0,0,"DRY");
  Serial.println("DTY");
    Blynk.notify("Water your plants");
}
else if(soil<600)
{
  lcd.clear();
    lcd.print(0,0,"Water");
  
  Serial.println("Water");
}

sensorState = digitalRead(sensorPin);
Serial.println(sensorState);

if (sensorState == 1 && lastState == 0) {
  Serial.println("needs water, send notification");
  Blynk.notify("Water your plants");
  lastState = 1;
  delay(1000);
//send notification
    
  } 
  else if (sensorState == 1 && lastState == 1) {
    //do nothing, has not been watered yet
  Serial.println("has not been watered yet");
  delay(1000);
  }
  else {
    Serial.println("does not need water");
    lastState = 0;
    delay(1000);
  }
  
  delay(100);

  //---------------------------------------------------------
 // Setting red filtered photodiodes to be read Red frequency
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  R = pulseIn(OUT, LOW);  // Reading the output Red frequency
  delay(100); 
  // Setting Green filtered photodiodes to be read Green frequency
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  G = pulseIn(OUT, LOW);  // Reading the output Green frequency
  delay(100);
  // Setting Blue filtered photodiodes to be read Blue frequency
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  B = pulseIn(OUT, LOW);  // Reading the output Blue frequency
  delay(100);
  
  //----------------------------------------------------------Detect colors based on sensor values
  if (R>120 && R<180 && G>580 && G<640 && B>370 && B<399){       // to detect red
    
    kcd.clear();
 kcd.print(0,1,"RED");
 

    Serial.print("RED");
       Serial.print("---------------------------------------------------");
  }
  else if (R>75 && R<100 && G>60 && G<85 && B>75 && B<95){  // to detect green
        kcd.clear();
 kcd.print(0,1,"GREEN");
    Serial.print("Green");
  }
  else if (R>95 && R<115 && G>70 && G<95 && B>30 && B<55){  // to detect blue
    kcd.clear();
 kcd.print(0,1,"BLUE");
    Serial.print("Blue");
  }
  
  //----------------------------------------------------------

  // Print RGB Sensor Values

    lcd.clear();    
   lcd.print(0,1,"R");
   lcd.print(1,1,R);

   lcd.print(4,1,"|");
   lcd.print(5,1," ");
      lcd.print(6,1,"G");
   lcd.print(7,1,G);

   lcd.print(10,1,"|");   
   lcd.print(11,1," ");
   lcd.print(12,1,"B");
   lcd.print(13,1,B);
   
  Serial.print("R= ");
  Serial.print(R);
  Serial.print(" | ");
  Serial.print("G= ");
  Serial.print(G);
  Serial.print(" | ");
  Serial.print("B= ");
  Serial.print(B);
  Serial.println();
  delay(200);
  
  
}
