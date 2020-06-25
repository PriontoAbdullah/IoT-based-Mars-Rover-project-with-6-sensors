// FOR SONOFF BASIC
// SELECT GENERIC ESP8266
// SELECT 1M (64K SPIFFS)
// SELECT FLASH MODE = DOUT

// FOR ESP8266-01
// SELECT GENERIC ESP8266
// SELECT 512K (64K SPIFFS)
// SELECT FLASH MODE = DIN
//
// FOR NODEMCU and WEMOS D1 MINI
// SELECT NODEMCU 1.00
// SELECT 4M (1M SPIFFS)
// SELECT FLASH MODE = DIN
//
// # # #   N O T E   # # #
//========================
//
// UNCOMMENT the first line for Sonoff Devices ONLY
// This will remap the I/O pins to the correct numbers
//
// For ESP8266-01, do NOT remap the I/O pins
//
// For NodeMCU and Wemos D1, change the I/O pins to your requirements.


//#define Sonoff 1

/*REVISION HISTORY
 * Ver 1.10
 * --------
 * ESPIFFS removed - Should work on latest ESP-01 moduls
 * Network settings now saved in EEPROM
 * 
 * Ver 10.9
 * --------
 * Buf fix line 1765
 * 
 * Ver 1.08
 * --------
 * Fixed Mode ON and OFF issues
 * 
 * Ver 1.07
 * --------
 * Added 2nd default NTP IP address
 * 
 * Ver 1.06
 * --------
 * Streamlined reading of request.
 * No more 1 second delay if no data available
 * 
 * Ver 1.05
 * --------
 * Changes to Manual Override made
 * Minor bug fixes
 * 
 * Ver 1.04
 * --------
 * Added Override-When-On timer function.
 * Slight changes to web interface made.
 * 
 * Ver 1.03
 * ---------
 * Increased wifiManager.setTimeout() from 120 to 180
 * Disable Access Point after connection
 * Changes to Override LED indications
 * 
 * Ver 1.02
 * --------
 * WiFiManagerchanged to activate only when button pressed
 * 
 * Ver 1.01
 * --------
 * NTP Time update streamlined
 * Fixed NTP time update bug
 *   NTPtimeOk == false;
 *   instead of 
 *   NTPtimeOk = false;
 * 
 * Ver 1.00
 * --------
 * New User Interface with menu
 * 
 * 
 */

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

// Defines
#define      DefaultName       "IoT Smart Timer"    // Default device name
#define      NTPfastReq        10                   // NTP time request in seconds when  time not set
#define      NTPslowReq        3600                 // NTP time request in seconds after time is  set
#ifdef Sonoff 
  #define      Version           "1.10 (Sonoff Basic)"               // Firmware version
#else
  #define      Version           "1.10 (ESP8266-01)"               // Firmware version
#endif  


// NTP Server details
//-------------------
IPAddress timeServer(129, 6, 15, 28);              // time.nist.gov NTP server
WiFiUDP Udp;
unsigned int localPort       = 3232;               // Local port to listen for UDP packets
WiFiServer server(80);

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
//-----------------------------
char static_ip[16] = "192.168.4.1";
char static_gw[16] = "192.168.4.1";
char static_sn[16] = "255.255.255.0";

//flag for saving configuration data
//----------------------------------
bool shouldSaveConfig = false;

// I/O config
//-----------
#ifdef Sonoff
  #define      Relay            12   // Output relay
  #define      Button            0   // Button
  #define      LED              13   // Satatus LED
#else
  #define      Relay             3   // Output relay
  #define      Button            2   // Button
  #define      LED               0   // Satatus LED
#endif


// Variables
//----------
#define      ErrMsg            "<font color=\"red\"> < < Error !</font>"
#define      EEPROM_chk        110
String       DevName         = DefaultName;
float        TimeZone        = 0;
byte         TimeZoneH       = 0;
byte         TimeZoneM       = 0;
boolean      ResetWiFi       = false;
long         timeNow         = 0;
long         timeOld         = 300000;
boolean      TimeOk          = false;
boolean      NTPtimeOk       = false;
String       request         = "";
byte         Page            = 1;
int          IP_1            = 129;
int          IP_2            = 6;
int          IP_3            = 15;
int          IP_4            = 28;
boolean      TimeCheck       = false;
byte         Mode            = 0;
byte         OnMode          = 0;
byte         OffMode         = 0;
int          TimerMinuteOn   = 0;
int          TimerHourOn     = 0;
int          TimerMinuteOff  = 0;
int          TimerHourOff    = 0;
boolean      TimedOff        = false;
boolean      TimedOn         = false;
int          NewHH           = 0;
int          NewMM           = 0;
int          Newdd           = 0;
int          Newmm           = 0;
int          Newyy           = 0;
int          PgmNr           = 0;
int          OnHour          = 0;
int          OnMinute        = 0;
int          OffHour         = 0;
int          OffMinute       = 0;
boolean      ManualOff       = false;
boolean      ManualOn        = false;
boolean      ManualTimeOff   = false;
boolean      ManualTimeOn    = false;
long         ManualSecOff    = 0;
long         ManualSecOn     = 0;
int          LastHH          = 0;
int          LastMM          = 0;
int          Lastdd          = 0;
int          Lastmm          = 0;
int          Lastyy          = 0;
byte         old_sec         = 0;
long         old_ms          = 0;
boolean      WebButton       = false;
boolean      PgmPrev         = false;
boolean      PgmNext         = false;
boolean      PgmSave         = false;
boolean      Error1          = false;
boolean      Error2          = false;
boolean      Error3          = false;
boolean      Error3b         = false;
boolean      Error4          = false;
boolean      Error5          = false;
boolean      Error6          = false;
boolean      Error7          = false;
boolean      Error8          = false;
boolean      D[8]            = {false, false, false, false, false, false, false, false};
int          On_Time[7];
int          Off_Time[7];
boolean      On_Days[7][8];
IPAddress   _ip,_gw,_sn;

//###############################################################################################
// Setup
//
//###############################################################################################
void setup() {
  // For debug only - REMOVE
  //----------------------------------------------------------------------
  //Serial.begin(115200);
  
  // Setup I/O pins
  //----------------------------------------------------------------------
  pinMode(Button,INPUT_PULLUP);
  pinMode(Relay,OUTPUT);
  pinMode(LED,OUTPUT);

  #ifdef Sonoff
    digitalWrite(Relay,LOW);
    digitalWrite(LED,HIGH);
  #else
    digitalWrite(Relay,LOW);
    digitalWrite(LED,LOW);
  #endif
  // If button pressed, reset WiFi connection
  //----------------------------------------------------------------------
  CheckReset();
  // Start WiFi and connect to AP
  //----------------------------------------------------------------------
  StartWiFi();
  #ifdef Sonoff
    digitalWrite(LED,HIGH);
  #else
    digitalWrite(LED,LOW);
  #endif
}


//###############################################################################################
// Main program loop
//
//###############################################################################################
void loop() {
  // Scan Button
  //----------------------------------------------------------------------
  ScanButton();
  // Scan for NTP time changes
  //----------------------------------------------------------------------
  CheckNTPtime();
  // See if time has changed
  //----------------------------------------------------------------------
  DoTimeCheck();
  // Update LED
  //----------------------------------------------------------------------
  UpdateLED();
  
  // See if any data was received via WiFi
  //----------------------------------------------------------------------
  WiFiClient client = server.available();
  String  request = "";
  boolean DataAvailable = false;
  if (client != 0) {
    // Read requests from web page if available
    //----------------------------------------------------------------------
    request = client.readStringUntil('\r');
    DataAvailable = true;
    client.flush();
  }

  // Respond to requests from web page
  //----------------------------------------------------------------------
  if ( (DataAvailable == true) and (request.indexOf("/") != -1) ) {
    //Serial.println(request);
    if (request.indexOf("Link=1")     != -1) Page = 1;
    if (request.indexOf("Link=2")     != -1) Page = 2;
    if (request.indexOf("Link=3")     != -1) Page = 3;
    if (request.indexOf("Link=4")     != -1) Page = 4;
    if (request.indexOf("Link=5")     != -1) Page = 5;
    if (request.indexOf("GET / HTTP") != -1) Page = 1;
    Error1 = false;
    Error2 = false;
    Error3 = false;
    Error3b= false;
    Error4 = false;
    Error5 = false;
    Error6 = false;
    Error7 = false;


    // Respond to Buttons
    //==================================

    // PAGE 1 - STATUS
    //----------------
    // See if Save Button was presed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn1=") != -1) {
      Page = 1;
      WebButton = true;
      ScanButton();
    }
    // See if Refresh Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("RefreshBtn1=") != -1) {
      Page = 1;
    }

    // PAGE 2 - PROGRAMS
    //------------------
    // See if Previous Buttomn was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtnPrev=") != -1) {
      Page = 2;
      PgmPrev = true;
      PgmNext = false;
      PgmSave = true;
    }
    // See if Next Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtnNext=") != -1) {
      Page = 2;
      PgmPrev = false;
      PgmNext = true;
      PgmSave = true;
    }
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn2=") != -1) {
      Page = 2;
      PgmSave = true;
      PgmPrev = false;
      PgmNext = false;
    }
    // See if Clear Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("ClearBtn1=") != -1) {
      Page = 2;
      On_Time[PgmNr]  = 0;
      Off_Time[PgmNr] = 0;
      for (byte i = 0; i < 7; i++ ) {
        On_Days[PgmNr][i] = false;
      }
      PgmPrev = false;
      PgmNext = false;
      // Save program data
      SaveProgram();
    }
    // Get program data if any button was pressed
    //----------------------------------------------------------------------
    if (PgmSave == true) {
      PgmSave = false;
      // On Hour
      if (request.indexOf("OnH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OnH=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OnHour = Tmp.toInt();
        if ( (OnHour < 0) or (OnHour > 23) ) Error1 = true;
      }
      // On Minute
      if (request.indexOf("OnM=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OnM=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OnMinute = Tmp.toInt();
        if ( (OnMinute < 0) or (OnMinute > 59) ) Error1 = true;
      }
      // Off Hour
      if (request.indexOf("OffH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OffH=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OffHour = Tmp.toInt();
        if ( (OffHour < 0) or (OffHour > 23) ) Error2 = true;
      }
      // Off Minute
      if (request.indexOf("OffM=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OffM=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        if (t1 == -1) {
          t1 = Tmp.indexOf(" ");
        }
        Tmp.remove(t1);
        OffMinute = Tmp.toInt();
        if ( (OffMinute < 0) or (OffMinute > 59) ) Error2 = true;
      }
      // Reset day flags
      D[0] = false;
      D[1] = false;
      D[2] = false;
      D[3] = false;
      D[4] = false;
      D[5] = false;
      D[6] = false;
      D[7] = false;
      // Day 1
      if (request.indexOf("D1=on") != -1) D[0] = true;
      // Day 2
      if (request.indexOf("D2=on") != -1) D[1] = true;
      // Day 3
      if (request.indexOf("D3=on") != -1) D[2] = true;
      // Day 4
      if (request.indexOf("D4=on") != -1) D[3] = true;
      // Day 5
      if (request.indexOf("D5=on") != -1) D[4] = true;
      // Day 6
      if (request.indexOf("D6=on") != -1) D[5] = true;
      // Day 7
      if (request.indexOf("D7=on") != -1) D[6] = true;
      // Update program if no errors
      if ( (Error1 == false) and (Error2 == false) ) {
        On_Time[PgmNr]  = (OnHour  * 100) + OnMinute;
        Off_Time[PgmNr] = (OffHour * 100) + OffMinute;        
        for (byte i = 0; i < 7; i++) {
          if (D[i] == true) On_Days[PgmNr][i] = true; else On_Days[PgmNr][i] = false;
        }  
        // Save program data
        ManualOff = false;
        ManualOn = false;
        ManualTimeOn = false;
        ManualTimeOff = false;
        SaveProgram();
        timeOld = 0;
      }
      else {
        PgmPrev = false;
        PgmNext = false;
      }
    }
    // Change to Prev/Next Program
    if (PgmPrev == true) {
      PgmPrev = false;
      PgmNr = PgmNr - 1;
      if (PgmNr <0) PgmNr = 6;
    }
    if (PgmNext == true) {
      PgmNext = false;
      PgmNr = PgmNr + 1;
      if (PgmNr > 6) PgmNr = 0;        
    }

    // PAGE 3 - CONFIG
    //----------------
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn3=") != -1) {
      Page = 3;
      // Device Name
      if (request.indexOf("Dev=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("Dev=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        Tmp.replace("+"," ");
        DevName = Tmp;
      }
      // Mode select
      if (request.indexOf("mode=Auto") != -1) Mode = 2;
      if (request.indexOf("mode=On") != -1) Mode = 1;
      if (request.indexOf("mode=Off") != -1) Mode = 0;
      //get button when on
      if (request.indexOf("BtnOff=0") != -1) OffMode = 0;
      if (request.indexOf("BtnOff=1") != -1) OffMode = 1;
      if (request.indexOf("BtnOff=2") != -1) OffMode = 2;
      // get button when off
      if (request.indexOf("BtnOn=0") != -1) OnMode = 0;
      if (request.indexOf("BtnOn=1") != -1) OnMode = 1;
      if (request.indexOf("BtnOn=2") != -1) OnMode = 2;

      // get timer off hour
      if (request.indexOf("ToffH=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("ToffH=");
        Tmp.remove(0,t1+6);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerHourOff = Tmp.toInt();
        if ( (TimerHourOff > 12) or (TimerHourOff < 0) ) {
          TimerHourOff = 0;
          Error3b = true;
        }
      }
      // get timer off min
      if (request.indexOf("ToffM=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("ToffM=");
        Tmp.remove(0,t1+6);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerMinuteOff = Tmp.toInt();
        if ( (TimerMinuteOff > 59) or (TimerMinuteOff < 0) ) {
          TimerMinuteOff = 0;
          Error3b = true;
        }
      }
      
      // get timer on hour
      if (request.indexOf("TonH=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("TonH=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerHourOn = Tmp.toInt();
        if ( (TimerHourOn > 12) or (TimerHourOn < 0) ) {
          TimerHourOn = 0;
          Error3 = true;
        }
      }
      // get timer on min
      if (request.indexOf("TonM=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("TonM=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerMinuteOn = Tmp.toInt();
        if ( (TimerMinuteOn > 59) or (TimerMinuteOn < 0) ) {
          TimerMinuteOn = 0;
          Error3 = true;
        }
      }
      if ((Error3 == false) and (Error3b== false) ){
        SaveConfig();
        ManualOff = false;
        ManualOn = false;
        ManualTimeOn = false;
        ManualTimeOff = false;
        timeOld = 0;
      }
    }

    // PAGE 4 - NTP Setup
    //-------------------
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn4=") != -1) {
      Page = 4;
      // Time Zone
      if (request.indexOf("TZH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("TZH=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        if (t1 == -1) {
          t1 = Tmp.indexOf(" ");
        }
        Tmp.remove(t1);
        TimeZone = Tmp.toFloat();
        if ( (TimeZone < -12) or (TimeZone > 12) ) {
          Error4 = true;
          TimeZone = 0;
        }
      }
      // Get NTP IP address
      //--------------------------------
      if (request.indexOf("IP_1=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_1=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_1 = Tmp.toInt();
        if ( (IP_1 < 0) or (IP_1 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_2=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_2=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_2 = Tmp.toInt();
        if ( (IP_2 < 0) or (IP_2 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_3=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_3=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_3 = Tmp.toInt();
        if ( (IP_3 < 0) or (IP_3 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_4=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_4=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_4 = Tmp.toInt();
        if ( (IP_4 < 0) or (IP_4 > 255) ) Error5 = true;
      }
      if ( (Error4 == false) and (Error5 == false) ) {
        // Set new NTP IP
        timeServer[0] = IP_1;
        timeServer[1] = IP_2;
        timeServer[2] = IP_3;
        timeServer[3] = IP_4;
        ManualOff = false;
        ManualOn = false;
        ManualTimeOn = false;
        ManualTimeOff = false;
        //Save Time Server Settings
        SaveNTP();
        NTPtimeOk = false;
        setSyncInterval(NTPfastReq);
      }
    }
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn5=") != -1) {
      Page = 4;
      //Get new hour
      String Tmp = request;
      int t1 = Tmp.indexOf("TimeHour=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      NewHH = Tmp.toInt();
      if ( (NewHH < 0) or (NewHH > 23) ) Error6 = true;
      //Get new minute
      Tmp = request;
      t1 = Tmp.indexOf("TimeMinute=");
      Tmp.remove(0,t1+11);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      NewMM = Tmp.toInt();
      if ( (NewMM < 0) or (NewMM > 59) ) Error6 = true;
      //Get new date
      Tmp = request;
      t1 = Tmp.indexOf("TimeDate=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      Newdd = Tmp.toInt();
      if ( (Newdd < 1) or (Newdd > 31) ) Error7 = true;
      //Get new month
      Tmp = request;
      t1 = Tmp.indexOf("TimeMonth=");
      Tmp.remove(0,t1+10);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      Newmm = Tmp.toInt();
      if ( (Newmm < 1) or (Newmm > 12) ) Error7 = true;
      //Get new year
      Tmp = request;
      t1 = Tmp.indexOf("TimeYear=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      if (t1 == -1) {
        t1 = Tmp.indexOf(" ");
      }
      Tmp.remove(t1);
      Newyy = Tmp.toInt();
      if ( (Newyy < 2000) or (Newyy > 2069) ) Error7 = true;
      // Update time
      //------------
      setTime(NewHH, NewMM, 0, Newdd, Newmm, Newyy);
      LastHH = NewHH;
      LastMM = NewMM;
      Lastdd = Newdd;
      Lastmm = Newmm;
      Lastyy = Newyy;
      TimeOk = true;
      ManualOff = false;
      ManualOn = false;
      ManualTimeOn = false;
      ManualTimeOff = false;
      timeOld = 0;
      setSyncInterval(NTPslowReq);
    }
    // See if Refresh Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("RefreshBtn5=") != -1) {
      Page = 4;
    }

    // Check time before updating web page
    //----------------------------------------------------------------------
    DoTimeCheck();

    // Web Page HTML Code
    //==================================
    client.println("<!doctype html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    // Refresh home page every 60 sec
    //client.println("<META HTTP-EQUIV=""refresh"" CONTENT=""60"">");
    client.print("<style> body {background-color: #C3FCF7;Color: #2B276E;}</style>");
    client.println("<title>");
    client.println(DevName);
    client.println("</title>");
    client.println("</head>");
    client.print("<body>");
    client.print("<font size = \"5\"><b>");
    client.print(DevName);
    client.print("</font></b><br>"); 
    // Show time
    //----------------------------------------------------------------------
    client.print("<p style=\"color:#180BF4;\";>");  
    client.print("<font size = \"5\"><b>");
    if (hour() < 10) client.print(" ");
    client.print(hour());
    client.print(":");
    if (minute() < 10) client.print("0");
    client.print(minute());
    client.print("</font></p>"); 
    // Day of the week
    //----------------------------------------------------------------------
    switch (weekday()) {
      case 1: client.print("Sunday, ");
              break;
      case 2: client.print("Monday, ");
              break;
      case 3: client.print("Tuesday, ");
              break;  
      case 4: client.print("Wednesday, ");
              break;  
      case 5: client.print("Thursday, ");
              break;  
      case 6: client.print("Friday, ");
              break;  
      case 7: client.print("Saturday, ");
              break;  
      default: client.print("");
              break;        
    }
    // Date
    //----------------------------------------------------------------------
    client.print(day());
    // Month
    //----------------------------------------------------------------------
    switch (month()) {
      case  1: client.print(" January ");
               break;
      case  2: client.print(" February ");
               break;
      case  3: client.print(" March ");
               break;
      case  4: client.print(" April ");
               break;
      case  5: client.print(" May ");
               break;
      case  6: client.print(" June ");
               break;
      case  7: client.print(" July ");
               break;
      case  8: client.print(" August ");
               break;
      case  9: client.print(" September ");
               break;
      case 10: client.print(" October ");
               break;
      case 11: client.print(" November ");
               break;
      case 12: client.print(" December ");
               break;
      default: client.print(" ");
               break;        
    }
    // Year
    //----------------------------------------------------------------------
    client.print(year());
    client.print("<br><br>");
    // Show system status
    //----------------------------------------------------------------------
    client.print("Output: </b>");
    if ( (TimeOk == false) and (Mode == 2) ) { 
      client.print("Blocked - Time not set!");
    }
    else {
      if ( (ManualOff == true) and (Mode == 2) ){
        client.print("STANDBY until next event");
      }
      if ( (ManualOn == true) and (Mode == 2) ){
        client.print("ON until next event");
      }

      if ( (ManualTimeOn == true) and (Mode == 2) ){
        client.print("ON for ");
        if ( ( (ManualSecOn + 1)/3600) > 0) {
          client.print(( (ManualSecOn + 1)/3600));
          client.print(" hours, ");
        }
        client.print( ( ( (ManualSecOn + 1)%3600)/60) );
        client.print(" minutes");
      }

      if ( (ManualTimeOff == true) and (Mode == 2) ){
        client.print("Standby for ");
        if ( ( (ManualSecOff + 1)/3600) > 0) {
          client.print(( (ManualSecOff + 1)/3600));
          client.print(" hours, ");
        }
        client.print( ( ( (ManualSecOff + 1)%3600)/60) );
        client.print(" minutes");
      }
     
      if ( (ManualOff == false) and (ManualOn == false) and (ManualTimeOff == false) and (ManualTimeOn == false) ) {
        if (digitalRead(Relay) == 1) client.print("ON"); else client.print("OFF");
      }
    }
    client.println("<br><br>");
    //Menu
    //----------------------------------------------------------------------
    client.print("<form action= method=\"get\"><b>");
    client.print("<a href=\"Link=1\">Home</a>&emsp;");
    client.print("<a href=\"Link=2\">Programs</a>&emsp;");
    client.print("<a href=\"Link=3\">Config</a>&emsp;");
    client.print("<a href=\"Link=4\">Time</a><br>"); 
    // Draw line
    //----------------------------------------------------------------------
    client.print("</b><hr />");  
    
    // Status PAGE
    //============
    if (Page == 1) {
      client.print("<font size = \"4\"><b>Status</font></b><br><br>"); 
      client.print("<b>Mode : </b>");
      switch (Mode) {
        case 0 : client.print("Off");
                 break;
        case 1 : client.print("On");
                 break;
        case 2 : client.print("Auto");
                 break;
      }
      client.print("<br><br>");
      client.print("<b>Override : </b>");
      if ( (ManualOff  == true) or (ManualOn   == true) or (ManualTimeOn == true) or (ManualTimeOff == true) ) {
        client.print("Active");
      }
      else {
        client.print("Off");
      }
      client.print("<br>");
      //Button 1
      client.println("<br>");
      client.println("<input type=\"submit\" name =\"SaveBtn1\" value=\"Manual Override\">");
      //Button 2
      client.println("&emsp;");
      client.println("<input type=\"submit\" name =\"RefreshBtn1\" value=\"Refresh\">");
      client.println("<br>");
      // Draw line
      client.print("<hr />");        
      // Show last time synch
      client.print("<font size = \"2\">"); 
      client.print("</b>Version ");
      client.print(Version);
      client.print("<br>");
      client.print("Time was last updated on ");
      client.print(Lastdd);
      client.print("/");
      if (Lastmm < 10) client.print("0");
      client.print(Lastmm);
      client.print("/");
      client.print(Lastyy);
      client.print(" at ");
      if (LastHH < 10) client.print("0");
      client.print(LastHH);
      client.print(":");
      if (LastMM < 10) client.print("0");
      client.print(LastMM);
      client.print("</font>");
    }
  
    //Program  PAGE
    //============
    if (Page == 2) {
      // Program number
      client.print("<font size = \"4\"><b>"); 
      client.print("Program ");
      client.print(PgmNr + 1);
      client.print(" of 7");
      //Previous Button
      client.println("&emsp;<input type=\"submit\" name =\"SaveBtnPrev\" value=\" << \">");
      client.println("&emsp;");
      //Next Button
      client.println("<input type=\"submit\" name =\"SaveBtnNext\" value=\" >> \"></font></b><br><br>");
      //On time
      client.print("On  Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OnH\"value =\"");
//      if ( (On_Time[PgmNr] / 100) < 10) client.print("0");
      client.print(On_Time[PgmNr]/100);    
      client.print("\"> : <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OnM\"value =\"");
      if ( (On_Time[PgmNr] % 100) < 10) client.print("0");
      client.print(On_Time[PgmNr]%100);    
      client.print("\">");
      if (Error1 == true) client.print(ErrMsg);
      client.print("<br><br>");
      //Off time
      client.print("Off Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OffH\"value =\"");
//      if ( (Off_Time[PgmNr] / 100) < 10) client.print("0");
      client.print(Off_Time[PgmNr]/100);    
      client.print("\"> : <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OffM\"value =\"");
      if ( (Off_Time[PgmNr] % 100) < 10) client.print("0");
      client.print(Off_Time[PgmNr]%100);    
      client.print("\">");
      if (Error2 == true) client.print(ErrMsg);
      client.print("<br><br>");
      //Day 1
      client.print("<input type=\"Checkbox\" name=\"D1\"");
      if (On_Days[PgmNr][0]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Sun<br>");
      //Day 2
      client.print("<input type=\"Checkbox\" name=\"D2\"");
      if (On_Days[PgmNr][1]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Mon<br>");
      //Day 3
      client.print("<input type=\"Checkbox\" name=\"D3\"");
      if (On_Days[PgmNr][2]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Tue<br>");
      //Day 4
      client.print("<input type=\"Checkbox\" name=\"D4\"");
      if (On_Days[PgmNr][3]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Wed<br>");
      //Day 5
      client.print("<input type=\"Checkbox\" name=\"D5\"");
      if (On_Days[PgmNr][4]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Thu<br>");
      //Day 6
      client.print("<input type=\"Checkbox\" name=\"D6\"");
      if (On_Days[PgmNr][5]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Fri<br>");
      //Day 7
      client.print("<input type=\"Checkbox\" name=\"D7\"");
      if (On_Days[PgmNr][6]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Sat<br>");
      //Button
      client.println("<br>");
      client.println("<input type=\"submit\" name =\"SaveBtn2\" value=\"Save\">");
      client.print("&emsp;");
      client.println("<input type=\"submit\" name =\"ClearBtn1\" value=\"Clear\">");
    }
  
    // Config PAGE
    //============
    if (Page == 3) {
      client.print("<font size = \"4\"><b>Configuration</font></b><br><br>"); 
      // Device Name
      client.print("Device Name: ");
      client.print("<input type=\"text\"<input maxlength=\"30\" size=\"35\" name=\"Dev\" value =\"");
      client.print(DevName);
      client.println("\"><br><br>");
      //Mode Select
      client.println("<b>Mode: </b>");
      client.print("<input type=\"radio\" name=\"mode\" value=\"Auto\"");
      if (Mode == 2) client.print("checked");
      client.print("/> Auto ");
      client.print("<input type=\"radio\" name=\"mode\" value=\"On\"");
      if (Mode == 1) client.print("checked");
      client.print("/> On ");
      client.print("<input type=\"radio\" name=\"mode\" value=\"Off\"");
      if (Mode == 0) client.print("checked");
      client.print("/> Off <br><br>");
      
      client.print("<font size = \"4\"><b>Button Function</font></b><br>"); 
      // Device Name

      // When output ON
      client.print("When ");
      client.print(DevName);
      client.print(" is on:<br>");
      // do nothing
      client.print("<input type=\"radio\" name=\"BtnOff\" value=\"0\"");
      if (OffMode == 0) client.print("checked");
      client.print("/> Do nothing<br>");
      // turn off
      client.print("<input type=\"radio\" name=\"BtnOff\" value=\"1\"");
      if (OffMode == 1) client.print("checked");
      client.print("/> Turn OFF until next event<br>");
      // turn on for specific time
      client.print("<input type=\"radio\" name=\"BtnOff\" value=\"2\"");
      if (OffMode == 2) client.print("checked");
      client.print("/> Turn OFF for specific time<br>");
      // time input
      client.print("&emsp;&emsp;<input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"ToffH\"value =\"");
      client.print(TimerHourOff);
      client.print("\">");
      client.print(" : <input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"ToffM\"value =\"");
      if (TimerMinuteOff < 10) client.print("0");
      client.print(TimerMinuteOff);
      client.print("\">");
      if (Error3b == true) {
        client.print(ErrMsg);
      }
      else {
        client.print(" (hh:mm)");
      }
      client.println("<br><br>");

      //When output is OFF
      //------------------
      client.print("When ");
      client.print(DevName);
      client.print(" is off:<br>");
      // do nothing
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"0\"");
      if (OnMode == 0) client.print("checked");
      client.print("/> Do nothing<br>");
      // turn on til next event
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"1\"");
      if (OnMode == 1) client.print("checked");
      client.print("/> Turn ON until next event<br>");
      // turn off for specific time
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"2\"");
      if (OnMode == 2) client.print("checked");
      client.print("/> Turn ON for specific time<br>");
      // time input
      client.print("&emsp;&emsp;<input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"TonH\"value =\"");
      client.print(TimerHourOn);
      client.print("\">");
      client.print(" : <input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"TonM\"value =\"");
      if (TimerMinuteOn < 10) client.print("0");
      client.print(TimerMinuteOn);
      client.print("\">");
      if (Error3 == true) {
        client.print(ErrMsg);
      }
      else {
        client.print(" (hh:mm)");
      }
      //Button
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn3\" value=\"Save\">");
    }
  
    // Time Server PAGE
    //=================
    if (Page == 4) {
      client.print("<font size = \"4\"><b><u>Time Setup</u></font></b><br><br>"); 
      
      //Time Zone
      client.print("<font size = \"3\"><b>NTP Network Setup</font></b><br>"); 
      client.print("Time Zone ");
      client.print("<input type=\"text\"<input maxlength=\"6\" size=\"7\" name=\"TZH\" value =\"");
      client.print(TimeZone,2);
      client.println("\">");
//      if (Error4 == true) client.print(ErrMsg); else client.print(" (hours)");
      client.print("<br><br>");
      //IP Addtess if time server
      client.print("Time Server IP : <i>(default 129.6.15.28 or 196.4.160.4)</i><br>");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_1\"value =\"");
      client.print(IP_1);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_2\"value =\"");
      client.print(IP_2);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_3\"value =\"");
      client.print(IP_3);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_4\"value =\"");
      client.print(IP_4);    
      client.print("\">");
//      if (Error5 == true) client.print(ErrMsg);
      //Button 1
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn4\" value=\"Save\"><br>");
      // Draw line
      client.print("<hr />");        

      // Set Time Inputs
      client.print("<font size = \"3\"><b>Local Time Adjust</font></b><br>"); 
      client.print("<br>Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"TimeHour\"value =\"");
      client.print(hour());
      client.print("\">");
      client.print(" : <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"TimeMinute\"value =\"");
      if (minute() < 10) client.print("0");
      client.print(minute());
      client.print("\">");
//      if (Error6 == true) client.print(ErrMsg); else client.print(" (hh:mm)");
      // Set Date Inputs
      client.print("<br><br>");
      client.print("Date: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"TimeDate\"value =\"");
      client.print(day());
      client.print("\">");
      client.print(" / <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"TimeMonth\"value =\"");
      if (month() < 10) client.print("0");
      client.print(month());
      client.print("\">");
      client.print(" / <input type=\"text\"<input maxlength=\"4\" size=\"4\" name=\"TimeYear\"value =\"");
      client.print(year());
      client.print("\">");
//      if (Error7 == true) client.print(ErrMsg); else client.print(" (dd/mm/yyyy)");
      //Button 2
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn5\" value=\"Update Time\">");
      //Button 3
      client.println("&emsp;");
      client.println("<input type=\"submit\" name =\"RefreshBtn\" value=\"Refresh\">");
      
      // Draw line
      client.print("<hr />");        
      // Show last time synch
      client.print("<font size = \"2\">"); 
      client.print("</b>Time last updated ");
      client.print(Lastdd);
      client.print("/");
      if (Lastmm < 10) client.print("0");
      client.print(Lastmm);
      client.print("/");
      client.print(Lastyy);
      client.print(" at ");
      if (LastHH < 10) client.print("0");
      client.print(LastHH);
      client.print(":");
      if (LastMM < 10) client.print("0");
      client.print(LastMM);
      client.print("</font>");
    }
    
    client.println("</body>");
    client.println("</html>");
    // End of Web Page
  }
}


//###############################################################################################
// Save Config Data
// 
//###############################################################################################
void SaveConfig() {
  EEPROM.write( 0,EEPROM_chk);        // EEPROM Check
  EEPROM.write( 1,Mode);              // Mode
  EEPROM.write( 2,OnMode);            // OnMode
  EEPROM.write( 3,OffMode);           // Off Mode
  EEPROM.write( 4,TimerHourOn);       // On Hour
  EEPROM.write( 5,TimerMinuteOn);     // On minute
  EEPROM.write( 6,TimerHourOff);      // Off Hour
  EEPROM.write( 7,TimerMinuteOff);    // Off minute
  SaveString(DevName,1);              // Device name (110-139)
  EEPROM.commit();
}


//###############################################################################################
// Save Program Data
// 
//###############################################################################################
void SaveProgram() {
  for (byte i = 0; i < 7; i++) {
    byte nr = i;
    byte t1 = 20 + (nr * 12);
    //On Time
    EEPROM.write(t1,On_Time[nr]/100);
    EEPROM.write(t1 +  1,On_Time[nr]%100);
    // Off time
    EEPROM.write(t1 +  2,Off_Time[nr]/100);
    EEPROM.write(t1 +  3,Off_Time[nr]%100);
    // On days
    EEPROM.write(t1 +  4,On_Days[nr][0]);  
    EEPROM.write(t1 +  5,On_Days[nr][1]);  
    EEPROM.write(t1 +  6,On_Days[nr][2]);  
    EEPROM.write(t1 +  7,On_Days[nr][3]);  
    EEPROM.write(t1 +  8,On_Days[nr][4]);  
    EEPROM.write(t1 +  9,On_Days[nr][5]);  
    EEPROM.write(t1 + 10,On_Days[nr][6]);  
    // Spare
    EEPROM.write(t1 + 11,On_Days[nr][7]);  
    EEPROM.commit();
  }
}


//###############################################################################################
// Save NTP Server Data
// 
//###############################################################################################
void SaveNTP() {
  int Tz = (TimeZone * 100);
  Tz = Tz + 1200;
  TimeZoneH = Tz/100;
  TimeZoneM = Tz%100;
  EEPROM.write( 8,TimeZoneH);
  EEPROM.write( 9,TimeZoneM); 
  EEPROM.write(10,IP_1);
  EEPROM.write(11,IP_2);
  EEPROM.write(12,IP_3);
  EEPROM.write(13,IP_4);
  EEPROM.commit();
}


//###############################################################################################
// SAVE string to EEPROM
// 
//###############################################################################################
void SaveString(String s1, unsigned int s2) {
  // Adjust string length to 20 characters
  //-----------------------------------------------
  s1.trim();
  // String EEPROM address starts at 110
  //-----------------------------------------------
  s2 = ( (s2 - 1) * 30) + 110;
  byte l = s1.length();
  if (s1.length() > 30) {
    s1.remove(30,l+1);
  }
  while(s1.length() < 30) {
    s1 = s1 + " ";  
  }
  // Save 20 chracters to EEPROM
  //------------------------------------------------
  for (byte i = 0;i < 30;i++) {
    EEPROM.write(s2 + i, s1[i]);
  }
}


//###############################################################################################
// READ program data from EEPROM
// 
//###############################################################################################
void ReadData() {
  // See if EEPROM contains valid data.
  // EEPROM location 0 will contain EEPROM_chk if data is valid
  // If not valid, store defult settings
  if (EEPROM.read(0) != EEPROM_chk) {
    EEPROM.write( 0,EEPROM_chk);  // EEPROM check
    EEPROM.write( 1,0  );         // Mode
    EEPROM.write( 2,0  );         // OnMode
    EEPROM.write( 3,0  );         // OffMode

    EEPROM.write( 4,0  );         // TimerHourOn
    EEPROM.write( 5,0  );         // TimerMinuteOn
    EEPROM.write( 6,0  );         // TimerHourOff
    EEPROM.write( 7,0  );         // TimerMinuteOff
    
    EEPROM.write( 8,12 );         // Time Zone Hour
    EEPROM.write( 9,0  );         // TimZone Minute
    EEPROM.write(10,129);         // NTP IP 1
    EEPROM.write(11,6  );         // NTP IP 2
    EEPROM.write(12,15 );         // NTP IP 3
    EEPROM.write(13,28 );         // NTP IP 4
    // Clear all programs
    for (byte i = 20; i < 105; i++) {  
      EEPROM.write(i,0);
    }
    // Save default name
    SaveString(DefaultName,1);
    EEPROM.commit();
  }
  // Read programs
  for (byte i = 0; i < 7; i++) {
    byte nr = i;
    byte t1 = 20 + ( (i) * 12);
    //On Time
    //---------------------
    On_Time[nr]  = EEPROM.read(t1);
    On_Time[nr] = On_Time[nr] * 100;
    On_Time[nr] = On_Time[nr] + EEPROM.read(t1 + 1);
    // Off time
    //---------------------
    Off_Time[nr]  = EEPROM.read(t1 + 2);
    Off_Time[nr] = Off_Time[nr] * 100;
    Off_Time[nr] = Off_Time[nr] + EEPROM.read(t1 + 3);
    // On days
    //---------------------
    On_Days[nr][0] = EEPROM.read(t1 +  4);  
    On_Days[nr][1] = EEPROM.read(t1 +  5);  
    On_Days[nr][2] = EEPROM.read(t1 +  6);  
    On_Days[nr][3] = EEPROM.read(t1 +  7);  
    On_Days[nr][4] = EEPROM.read(t1 +  8);  
    On_Days[nr][5] = EEPROM.read(t1 +  9);  
    On_Days[nr][6] = EEPROM.read(t1 + 10);  
    // Spare
    //---------------------
    On_Days[nr][7] = EEPROM.read(t1 + 11);  
  }
  Mode           = EEPROM.read(1);
  OnMode         = EEPROM.read(2);
  OffMode        = EEPROM.read(3);
  TimerHourOn    = EEPROM.read(4);
  TimerMinuteOn  = EEPROM.read(5);
  TimerHourOff   = EEPROM.read(6);
  TimerMinuteOff = EEPROM.read(7);
  TimeZoneH      = EEPROM.read(8);
  TimeZoneM      = EEPROM.read(9);
  IP_1           = EEPROM.read(10);
  IP_2           = EEPROM.read(11);
  IP_3           = EEPROM.read(12);
  IP_4           = EEPROM.read(13);
  // Setup timeServer IP
  timeServer[0] = IP_1;
  timeServer[1] = IP_2;
  timeServer[2] = IP_3;
  timeServer[3] = IP_4;
  DevName = ReadString(1);
  // Assemble Timezone value
  TimeZone = TimeZoneM;
  TimeZone = TimeZone / 100;
  TimeZone = TimeZone + TimeZoneH;
  TimeZone = TimeZone -12;
}


//###############################################################################################
// READ string from EEPROM
// 
//###############################################################################################
String ReadString(int s1) {
  String s2 = "";
  // String EEPROM address starts at 110
  //-----------------------------------------------
  s1 = ( (s1 - 1) * 30) + 110;
    // Read 30 characters
  //-----------------------------------------------
  for (byte i = 0;i < 30;i++) {
    s2 = s2 + char(EEPROM.read(s1 + i));
  }
  s2.trim();
  return s2;
}


//###############################################################################################
// NTP Code - do not change
//
//###############################################################################################
const int NTP_PACKET_SIZE = 48;                 // NTP time is in the first 48 bytes of message
byte      packetBuffer[NTP_PACKET_SIZE];        //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ;               // discard any previously received packets
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 5000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      if (TimeOk ==false) {
        TimeOk = true;
      }
      TimeCheck   = true;
      return secsSince1900 - 2208988800UL + TimeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}


//###############################################################################################
// send an NTP request to the time server at the given address
//
//###############################################################################################
void sendNTPpacket(IPAddress & address) {
  // set all bytes in the buffer to 0
  //------------------------------------------------
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //------------------------------------------------
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  //------------------------------------------------
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  //NTP requests are to port 123
  //------------------------------------------------
  Udp.beginPacket(address, 123); 
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


//###############################################################################################
// callback notifying us of the need to save config
// 
//###############################################################################################
void saveConfigCallback () {
  shouldSaveConfig = true;
}


//###############################################################################################
// Check if WiFi reset button pressed after power up
// 
//###############################################################################################
void CheckReset() {
  
  old_ms = millis();
  // 10 sec delay to allow user to configure Network 
  //----------------------------------------------------------------------
  while ( ((millis() - old_ms) < 10000 ) and (ResetWiFi == false) ) {
    //Wait for button to be pressed
    //----------------------------------------------------------------------
    digitalWrite(LED,LOW); //1
    delay(100);
    digitalWrite(LED,HIGH); //1
    delay(100);
    if (digitalRead(Button) == 0) {
      // Reset WiFi
      old_ms = millis();
      ResetWiFi = true;
      //Wait for button to be released
      //----------------------------------------------------------------------
      while (digitalRead(Button) == 0) {
        #ifdef Sonoff
          digitalWrite(LED,HIGH);
        #else
          digitalWrite(LED,LOW);
        #endif
      }
    }
  }
  #ifdef Sonoff
    digitalWrite(LED,LOW);
  #else
    digitalWrite(LED,HIGH);
  #endif
}


//###############################################################################################
// Scan for NTP Time changes
// 
//###############################################################################################
void CheckNTPtime() {

  // This line needed to keep NTP Time Synch active
  //------------------------------------------------
  timeNow = (10000 * hour()) + (minute() * 100) + second();

  // See if NTP time was set
  //------------------------
  if ( (TimeOk == true) and (NTPtimeOk == false) and (TimeCheck == true) ){
      setSyncInterval(NTPslowReq);
      NTPtimeOk = true;
  }
  // See if NTP Time was updated
  //----------------------------
  if (TimeCheck == true) {
    LastHH = hour();
    LastMM = minute();
    Lastdd = day();
    Lastmm = month();
    Lastyy = year();
    TimeCheck = false;
  }
}


//###############################################################################################
// Scan for Button presses
// 
//###############################################################################################
void ScanButton() {
  if ( (digitalRead(Button) == 0) or (WebButton == true) ) {
    WebButton= false;
    delay(50);
    boolean Armed = digitalRead(Relay);
    
    // Check manual OFF
    if ( (ManualOn == false) and (ManualTimeOn == false) and (ManualTimeOff == false) ) {
      if (Armed == 1) {
        if (ManualOff == false) {
          if (OffMode == 1) {
            ManualOff = true;
            ManualOn = false;
            ManualTimeOn = false;
            ManualTimeOff = false;
            goto doLED;
          }
        }
      }
      else {
        if (ManualOff == true) {
          ManualOff = false;
          ManualOn = false;
          ManualTimeOn = false;
          ManualTimeOff = false;
          goto doLED;
        }
     }
   }
    
    // Check manual ON
    if ( (ManualOff == false) and (ManualTimeOn == false) and (ManualTimeOff == false) ) {
      if (Armed == 0) {
        if (ManualOn == false) {
          if (OnMode == 1) {
            ManualOn = true;
            ManualOff = false;
            ManualTimeOn = false;
            ManualTimeOff = false;
            goto doLED;
          }
        }
      }
      else {
        if (ManualOn == true) {
          ManualOn = false;
          ManualOff = false;
          ManualTimeOn = false;
          ManualTimeOff = false;
          goto doLED;
        }
      }
    }

    
    // Check manual Timed On
    if ( (ManualOff == false) and (ManualOn == false) and (ManualTimeOff == false) ) {
      if (Armed == 0) {
        if (ManualTimeOn == false) {
          if (OnMode == 2) {
            ManualSecOn = (TimerHourOn * 3600) + (TimerMinuteOn * 60);
            ManualOn = false;
            ManualOff = false;
            ManualTimeOn = true;
            ManualTimeOff = false;
            goto doLED;
          }  
        }
      }
      else {
        if (ManualTimeOn == true) {
          ManualOn = false;
          ManualOff = false;
          ManualTimeOn = false;
          ManualTimeOff = false;
          goto doLED;
        }
      }
    }
        
    // Check manual Timed Off
    if ( (ManualOff == false) and (ManualOn == false) and (ManualTimeOn == false) ) {
      if (Armed == 1) {
        if (ManualTimeOff == false) {
          if (OffMode == 2) {
            ManualSecOff = (TimerHourOff * 3600) + (TimerMinuteOff * 60);
            ManualOn = false;
            ManualOff = false;
            ManualTimeOn = false;
            ManualTimeOff = true;
            goto doLED;
          }  
        }
      }
      else {
        if (ManualTimeOff == true) {
          ManualOn = false;
          ManualOff = false;
          ManualTimeOn = false;
          ManualTimeOff = false;
          goto doLED;
        }
      }
    }

    doLED:
    digitalWrite(LED,!digitalRead(LED)); //1

    // wait for button to be released
    while(digitalRead(Button) == 0) {
      delay(200);
    }
    // button relesed, change status of LED
    digitalWrite(LED,!digitalRead(LED)); //1
    timeOld = 0;
  }
}


//###############################################################################################
// Update LED Status
// 
//###############################################################################################
void UpdateLED() {
  //LED Status
  if (TimeOk == false) {
    if ( (millis() - old_ms) > 3000) {
      
      #ifdef Sonoff
        digitalWrite(LED,LOW); //1
        delay(50);
        digitalWrite(LED,HIGH); //1
        delay(50);
      #else
        digitalWrite(LED,HIGH); //1
        delay(50);
        digitalWrite(LED,LOW); //1
        delay(50);
      #endif

      old_ms - millis();
    }
  }
}


//###############################################################################################
// See if time has changed and update output according to programs
// 
//###############################################################################################
void DoTimeCheck() {
  boolean Output = false;
  // Is mode = Off
  if (Mode == 0) {
    Output = false;
    // clear button overrides
    ManualOff  = false;
    ManualOn   = false;
    ManualTimeOn = false;
    ManualTimeOff = false;
    //Turn off Output
    digitalWrite(Relay,LOW);
    //Turn off LED
    #ifdef Sonoff 
      digitalWrite(LED,HIGH); 
    #else 
      digitalWrite(LED,LOW);
    #endif
    return;
  }
  
  // Is mode = On
  if (Mode == 1) {
    Output = true;
    // clear button overrides
    ManualOff  = false;
    ManualOn   = false;
    ManualTimeOn = false;
    ManualTimeOff = false;
    //Turn on Output
    digitalWrite(Relay,HIGH);
    //Turn on LED
    #ifdef Sonoff 
      digitalWrite(LED,LOW); 
    #else 
      digitalWrite(LED, HIGH);
    #endif
    return;
  }
  
  // Is mode invalid
  if (Mode != 2) {
    return;
  }
  
  // Mode = 2, check Manual On Timer
  if (ManualTimeOn == true) {
    if ( (second() != old_sec) ){
      old_sec = second();
      if (ManualSecOn > 0) ManualSecOn = ManualSecOn - 1;
      if (ManualSecOn == 0) {
        ManualTimeOn = false; 
        timeOld = 0;
      }
    }
  }

  // Mode = 2, check Manual Off Timer
  if (ManualTimeOff == true) {
    if ( (second() != old_sec) ){
      old_sec = second();
      if (ManualSecOff > 0) ManualSecOff = ManualSecOff - 1;
      if (ManualSecOff == 0) {
        ManualTimeOff = false; 
        timeOld = 0;
      }
    }
  }

  
  // Mode = 2, See if time changed
  timeNow = (100 * hour()) + minute();
  if (timeOld != timeNow) {
    // Time changed - check outputs
    timeOld = timeNow;
    for (byte i = 0; i < 7; i++) {
      // See if Buzzer can be controlled
      if (TimeOk != false) {
        //Time Ok, check if output must be on
        // See if Ontime < OffTime (same day)
        if (On_Time[i] < Off_Time[i]) {
          if ( (timeNow >= On_Time[i]) and (timeNow < Off_Time[i]) ) {
            // See if current day is selected
            if (On_Days[i][weekday() - 1] == true) {
              Output = true;
            }
          }
        }
        // See if Ontime > OffTime (over two days)
        if (On_Time[i] > Off_Time[i]) {
          if ( (timeNow < Off_Time[i]) or (timeNow>= On_Time[i]) ) {
            int PrevDay = weekday() - 2;
            if (PrevDay < 0) PrevDay = 6;
            // Check current day
            if (timeNow >= On_Time[i]) {
              if (On_Days[i][weekday() - 1] == true) {
                Output = true;
              }
            }
            // Check previous day
            if (timeNow < Off_Time[i]) {
              if (On_Days[i][PrevDay] == true) {
                 Output = true;
              }
            }
          }
        } 
      }
    }
    
    // Check manual off 
    if ( (ManualOn == false) and (ManualTimeOn == false) ) {
      if (ManualOff == true) {
        if (Output == true) {
          Output = false;
        }
        else {
          ManualOff = false;
        }
      }
    }
    // Check manual on 
    if ( (ManualOff == false) and (ManualTimeOff == false) ) {
      if (ManualOn == true) {
        if (Output == false) {
          Output = true;
        }
        else {
          ManualOn = false;
        }
      }
    }
    // Check manual On time 
    if ( (ManualOff == false) and (ManualOn == false) ) {
      if (ManualTimeOn == true) { 
        if (Output == false) {
          Output = true;
        }
        else {
          ManualTimeOn = false;
        }
      }
    }

    // Check manual Off time 
    if ( (ManualOff == false) and (ManualOn == false) ) {
      if (ManualTimeOff == true) { 
        if (Output == true) {
          Output = false;
        }
        else {
          ManualTimeOff = false;
        }
      }
    }
    // Set output
    if (Output == true) digitalWrite(Relay,HIGH); else digitalWrite(Relay,LOW);
  } 
  // Update LED
  #ifdef Sonoff 
    digitalWrite(LED,!digitalRead(Relay)); //1
  #else 
  digitalWrite(LED,digitalRead(Relay)); //1
  #endif
}


//###############################################################################################
// READ network settings from EEPROM
// 
//###############################################################################################
void ReadNetwork() {
  // See if EEPROM contains valid data.
  // EEPROM location 499 will contain 123 if data is valid
  // If not valid, store defult settings
  if (EEPROM.read(499) != 012) {
    EEPROM.write(499, 012);  // Network Settings check
    // Default IP
    EEPROM.write(500, 192);
    EEPROM.write(501, 168);
    EEPROM.write(502, 4);
    EEPROM.write(503, 1);
    // Default Gateway
    EEPROM.write(504, 192);
    EEPROM.write(505, 168);
    EEPROM.write(506, 4);
    EEPROM.write(507, 1);
    // Default Subnet
    EEPROM.write(508, 255);
    EEPROM.write(509, 255);
    EEPROM.write(510, 255);
    EEPROM.write(511, 0);
    // Force EEPROM save
    EEPROM.commit();
  }
  // Fixed IP
  _ip[0] = EEPROM.read(500);
  _ip[1] = EEPROM.read(501);
  _ip[2] = EEPROM.read(502);
  _ip[3] = EEPROM.read(503);
  // Gateway IP
  _gw[0] = EEPROM.read(504);
  _gw[1] = EEPROM.read(505);
  _gw[2] = EEPROM.read(506);
  _gw[3] = EEPROM.read(507);
  // Subnet Mask 
  _sn[0] = EEPROM.read(508);
  _sn[1] = EEPROM.read(509);
  _sn[2] = EEPROM.read(510);
  _sn[3] = EEPROM.read(511);
}


//###############################################################################################
// SAVE network settings to EEPROM
// 
//###############################################################################################
void SaveNetwork() {
  // Default IP
  EEPROM.write(500, _ip[0]);
  EEPROM.write(501, _ip[1]);
  EEPROM.write(502, _ip[2]);
  EEPROM.write(503, _ip[3]);
  // Gateway
  EEPROM.write(504, _gw[0]);
  EEPROM.write(505, _gw[1]);
  EEPROM.write(506, _gw[2]);
  EEPROM.write(507, _gw[3]);
  // Subnet Mask  
  EEPROM.write(508, _sn[0]);
  EEPROM.write(509, _sn[1]);
  EEPROM.write(510, _sn[2]);
  EEPROM.write(511, _sn[3]);
  // Force EEPROM save
  EEPROM.commit();
}


//###############################################################################################
// Start WiFi
// 
//###############################################################################################
void StartWiFi() {
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  // Read Network settings from EEPROM
  //----------------------------------------------------------------------
  EEPROM.begin(512);
  ReadNetwork();    
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  //----------------------------------------------------------------------
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //----------------------------------------------------------------------
  WiFiManager wifiManager;
  //set config save notify callback
  //----------------------------------------------------------------------
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //set static ip
  //----------------------------------------------------------------------
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //reset settings
  //----------------------------------------------------------------------
  if (ResetWiFi == true) {
    wifiManager.resetSettings();
    ResetWiFi == false;
    _ip[0] = 192;
    _ip[1] = 168;
    _ip[2] = 4;
    _ip[3] = 1;
    _gw[0] = 192;
    _gw[1] = 168;
    _gw[2] = 4;
    _gw[3] = 1;
    _sn[0] = 255;
    _sn[1] = 255;
    _sn[2] = 255;
    _sn[3] = 0;
  }
  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //----------------------------------------------------------------------
  wifiManager.setMinimumSignalQuality();
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  //----------------------------------------------------------------------
  wifiManager.setTimeout(180);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "IoT Timer", password = password
  //and goes into a blocking loop awaiting configuration
  //----------------------------------------------------------------------
  if (!wifiManager.autoConnect(DefaultName)) {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  _ip.fromString(WiFi.localIP().toString());
  _gw.fromString(WiFi.gatewayIP().toString());
  _sn.fromString(WiFi.subnetMask().toString());
  //save the custom parameters to EEPROM
  //----------------------------------------------------------------------
  if (shouldSaveConfig) {
    SaveNetwork();
  }
  //if you get here you have connected to the WiFi
  //Once connected, disable Access Point
  //------------------------------------
  WiFi.mode(WIFI_STA);
  // Read settings from EEPROM
  //----------------------------------------------------------------------
  ReadData();    
  // Setup NTP time requests
  //----------------------------------------------------------------------
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(NTPfastReq);
  // Begin IoT server
  //----------------------------------------------------------------------
  server.begin();
}

