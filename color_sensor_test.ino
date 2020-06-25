
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>

char auth[] = "-_ZC-523FFB-AHSqctMZf4e2nEJMGrne"; //Enter the Auth code which was send by Blink


char ssid[] = "Net Nai";  //Enter your WIFI Name
char pass[] = "12345678900";  //Enter your WIFI Password

SimpleTimer timer;

#define S0 14
#define S1 12
#define S2 13
#define S3 15
#define OUT 16

int R,G,B = 0;

void setup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);

  
  // Setting frequency-scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);


   Serial.begin(9600); // See the connection status in Serial Monitor
  Blynk.begin(auth, ssid, pass);

  // Setup a function to be called every second
 // timer.setInterval(1000L, OUT);
}
void loop() {

  Blynk.run(); // Initiates Blynk
 // timer.run(); // Initiates SimpleTimer
  
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
  if (R>20 && R<35 && G>80 && G<105 && B>70 && B<90){       // to detect red
    Blynk.virtualWrite(V1, OUT);
  }
  else if (R>75 && R<100 && G>60 && G<85 && B>75 && B<95){  // to detect green
    Blynk.virtualWrite(V2, OUT);
  }
  else if (R>95 && R<115 && G>70 && G<95 && B>30 && B<55){  // to detect blue
    Blynk.virtualWrite(V3, OUT);
  }
  
  //----------------------------------------------------------

  // Print RGB Sensor Values
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