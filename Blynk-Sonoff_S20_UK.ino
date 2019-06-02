#define NAMEandVERSION "Blynk-Sonoff_V1.1_S20_1"

/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

//#define BLYNK_DEBUG
#define BLYNK_TIMEOUT_MS  500  // must be BEFORE BlynkSimpleEsp8266.h doesn't work !!!
#define BLYNK_HEARTBEAT   17   // must be BEFORE BlynkSimpleEsp8266.h works OK as 17s


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
WidgetRTC rtc;
BlynkTimer timer;

char ssid[]            = "SSID";
char pass[]            = "PASSWORD";
char auth[]            = "AUTHKEY";
//char server[]          = "blynk-cloud.com";
//char server[]          = IPAddress(192,168,1,3);
unsigned int port      = 1234; //use your own port of the server

bool online = 0;

//  App project setup:
/*
V0 - Switch Status - LED
V1 - Power On/Off - Button
V2 - GPS AutoOff - Button
V3 - GPS AutoOff Status - LED
V4 - GPS Trigger - Widget
V5 - GPS Trigger Status - LED
V6 - Manual/Scheduled - Button
V7 - Time Interval Input - Widget
V8 - Interval Status - LED
V9 - timesync
*/

WidgetLED ledIntervalStatus(V8);
WidgetLED ledSocketStatus(V0);
WidgetLED ledGPSTriggerStatus(V5);
WidgetLED ledGPSAutoOffSwitch(V3);


String Interval;
int StartHour = 0;
int StopHour = 0;
int StartMinute = 0;
int StopMinute = 0;
int Hour = 0;
int Minute = 0;

bool connection;  // monitoring the connection
int connectionattempts;

#define relay 12 //  relay
#define PushButton 0 // 
#define ConnectionStatus 13 // background light for the power button to show the connection satus


bool PushButtonState;
bool prevPushButtonState;
bool softwareButton;
bool SocketStatus;
bool GPSAutoOff;
bool GPSTrigger;
bool interval;
bool Mode;


//EC:FA:BC:8A:1F:D0
//byte arduino_mac[] = { 0xEC, 0xFA, 0xBC, 0x8A, 0x1F, 0xD0 };
IPAddress arduino_ip ( 192, 168, 1, 13);
//IPAddress dns_ip     (8, 8, 8, 8);
IPAddress gateway_ip ( 192, 168, 1, 1);
IPAddress subnet_mask(255, 255, 255, 0);


void setup()
{
  // Serial.begin(9600);
  WiFi.hostname(NAMEandVERSION);
  WiFi.mode(WIFI_STA);
  // Debug console

  WiFi.config(arduino_ip, gateway_ip, subnet_mask);
  Blynk.config(auth, IPAddress(192,168,1,2), port);  // I am using the local Server
  CheckConnection();// It needs to run first to initiate the connection.Same function works for checking the connection!
  timer.setInterval(10000L, CheckConnection); 
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
  // Blynk.begin(auth, ssid, pass, IPAddress(192, 168, 1, 3), 8441);
  timer.setInterval(10000L, timesync);
  //timer.setInterval(1000L, checkInterval);
  

  pinMode (relay, OUTPUT);
  pinMode (PushButton, INPUT);
  pinMode (ConnectionStatus, OUTPUT);

  //  Serial.println("Ready");
  //  Serial.print("IP address: ");
  //  Serial.println(WiFi.localIP());
}

void loop()
{
  if(Blynk.connected()){
    Blynk.run();
  }
  timer.run();
  PushButtonONOFF();
}


// get the VPIN values from the server
BLYNK_CONNECTED() 
{
  Blynk.syncVirtual(V1,V6,V2,V7,V9);
}

void timesync()
{
    Blynk.syncVirtual(V9);
    rtc.begin();
}

void PushButtonONOFF()
{
  PushButtonState = digitalRead(PushButton);
  if (PushButtonState != prevPushButtonState) {
    if (PushButtonState == LOW) {

      if (SocketStatus == 1)
      {
        SocketOff();
      }
      else
      {
        SocketOn();
      }
    }
    prevPushButtonState = PushButtonState;
  }
}


void SocketOn()
{
  digitalWrite(relay, HIGH);
  SocketStatus = 1;
  if(Blynk.connected()){
    ledSocketStatus.on();
    Blynk.virtualWrite(V1, SocketStatus);
    //  Serial.println("SocketOn");
    yield();
    ledSocketStatus.on();
  }
}

void SocketOff()
{
  digitalWrite(relay, LOW);
  SocketStatus = 0;
  if(Blynk.connected()){
    ledSocketStatus.off();
    Blynk.virtualWrite(V1, SocketStatus);
    //Serial.println("SocketOff");
    yield();
    ledSocketStatus.off();
  }
}

BLYNK_WRITE(V1)   // 
{ 
  //It triggers TRUE on EXIT
  bool VirtualPowerSwitch = param.asInt(); 
  //Store the value on server?
  yield();
  //Blynk.virtualWrite(V17, GPSTrigger); //update state of the buttos as it is on the mcu
  if (VirtualPowerSwitch == 1 )
  {
    SocketOn(); // Turn Off the Socket if the Feature is activated and there is nobody at home 
  }
  else 
  {
    SocketOff();
  }
}



void CheckConnection(){    // check every 11s if connected to Blynk server
  if(!Blynk.connected()){
    online = 0;
    yield();
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Not connected to Wifi! Connect...");
      //Blynk.connectWiFi(ssid, pass); // used with Blynk.connect() in place of Blynk.begin(auth, ssid, pass, server, port);
      WiFi.begin(ssid, pass);
      delay(400); //give it some time to connect
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Cannot connect to WIFI!");
        online = 0;
        digitalWrite(ConnectionStatus, HIGH);
      }
      else
      {
        Serial.println("Connected to wifi!");
      }
    }
    
    if ( WiFi.status() == WL_CONNECTED && !Blynk.connected() )
    {
      Serial.println("Not connected to Blynk Server! Connecting..."); 
      Blynk.connect();  // // It has 3 attempts of the defined BLYNK_TIMEOUT_MS to connect to the server, otherwise it goes to the enxt line 
      if(!Blynk.connected()){
        Serial.println("Connection failed!"); 
        online = 0;
        digitalWrite(ConnectionStatus, HIGH);
      }
      else
      {
        online = 1;
        digitalWrite(ConnectionStatus, LOW);
        chkWifiSignal();
      }
    }
  }
  else{
    Serial.println("Connected to Blynk server!"); 
    online = 1;
    digitalWrite(ConnectionStatus, LOW); 
    chkWifiSignal();   
    checkInterval();
  }
}

void chkWifiSignal()
{
  int WifiSignal = -(WiFi.RSSI()) ;
  Blynk.virtualWrite(V10, WifiSignal);
}

BLYNK_WRITE(V2)   // ON 1 = GPSAutoOFF is activated, turn off the socket if it is on manual and there is nobody home   - OFF 0 = deactivated
{ 
  //restoring int value
  GPSAutoOff = param.asInt();
  Serial.println();
  Serial.print("GPSAutoOff=");
  Serial.println(GPSAutoOff);
  Serial.println();
  yield();
  if (GPSAutoOff == 1)
  {
    ledGPSAutoOffSwitch.on();
  }
  else
  {
    ledGPSAutoOffSwitch.off();
  }
}

BLYNK_WRITE(V6)   // ON 1 = GPSAutoOFF is activated, turn off the socket if it is on manual and there is nobody home   - OFF 0 = deactivated
{ 
  //restoring int value
  Mode = param.asInt();
  yield();
}

BLYNK_WRITE(V4)   // ON 1 = you are in the radius, youre at home  - OFF 0 = you left the home
{ 
  //It triggers TRUE on EXIT
  GPSTrigger = param.asInt(); 
  yield();
  if (GPSTrigger == 0)
  {
    ledGPSTriggerStatus.on();
  }
  else
  {
    ledGPSTriggerStatus.off();
  }

  if (GPSAutoOff == 1 )
  {
    if (GPSTrigger == 1)
    {
      SocketOff(); // Turn Off the Socket if the Feature is activated and there is nobody at home  
    }
  }
}

BLYNK_WRITE(V7) {  
    TimeInputParam t(param);
    StartHour = t.getStartHour();
    StopHour = t.getStopHour();
    StartMinute = t.getStartMinute();
    StopMinute = t.getStopMinute();
    Hour = hour();
    Minute = minute();
    yield(); 
}

void checkInterval()
{
    timesync();
    Blynk.syncVirtual(V7);
//    String currentTime = String(Hour) + ":" + String(Minute) + ":" + String(second());
//    //String currentDate = String(day()) + " " + month() + " " + year();
//    Interval = String( String(StartHour) + ":" + String(StartMinute) + " - " + String(StopHour) + ":" + String(StopMinute));
    yield();
//    Serial.print("Current time: " + String(currentTime));
//    Serial.println();
    Blynk.syncVirtual(V12,V17); //synk the time interval and the gps trigger.
  
  /*

  *23:10 - 23:40
  *2:00 - 2:40
  *2:50 - 2:20
  
  22:10 - 2:00
  10:10 - 2:00
  3:00 - 1:00 
  
  *10:10 - 12:10
  *10:10 - 22:10
    
  */

  // 23:10 - 23:40 && 2:00 - 2:40
  if (Hour == StartHour && StartHour == StopHour && StartMinute < StopMinute) // Less than 1H
  {
    if (Minute >= StartMinute && Minute < StopMinute)
    {
      interval = 1;
      Serial.println("100");
    }
    else
    {
      interval = 0;
      Serial.println("101");
    }
  }
  
  
  // 12:50 - 12:10  12:11
  if (Hour == StartHour && StartHour == StopHour && StartMinute > StopMinute) // 24H - some Minutes
  {
    if (Hour != StartHour)
    {
      interval = 1;
      Serial.println("102");
    }
    else if (Hour == StartHour)
    {
      if (Minute < StartMinute && Minute > StopMinute) 
      {
        interval = 1;
        Serial.println("103");
      }
      else 
      {
        interval = 0;
        Serial.println("104");
      }
    }
  }
  
  // 10:10 - 12:10 && 10:10 - 22:10 - sameday
  if (StartHour < StopHour )
  {
    if (Hour == StartHour)
    {
      if (Minute < StartMinute)
      {
        interval = 0;
        Serial.println("105");
      }
      else if (Minute >= StartMinute)
      {
        interval = 1;
        Serial.println("106");
      }
    }
    
    if (Hour == StopHour)
    {
      if (Minute > StopMinute)
      {
        interval = 0;
        Serial.println("107");
      }
      else if (Minute <= StopMinute)
      {
        interval = 1;
        Serial.println("108");
      }
    }
    
    if (Hour > StartHour && Hour < StopHour)
    {
      interval = 1;
      Serial.println("109");
    }
  }
  
  
  // 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
  if (StartHour > StopHour)
  {
    if (Hour == StartHour)
    {
      if (Minute < StartMinute)
      {
        interval = 0;
        Serial.println("110");
      }
      else if (Minute >= StartMinute)
      {
        interval = 1;
        Serial.println("111");
      }
    }
    
    if (Hour == StopHour)
    {
      if (Minute > StopMinute)
      {
        interval = 0;
        Serial.println("112");
      }
      else if (Minute <= StopMinute)
      {
        interval = 1;
        Serial.println("113");
      }
    }   
    
    //23:10 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
    if (Hour > StartHour)
    {
      interval = 1;
      Serial.println("114");
    }
    // 1:00 // 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
    else if (Hour < StartHour)
    {
      if (Hour > StopHour)
      {
        interval = 0;
        Serial.println("115");
      }
      else if (Hour < StopHour)
      {
        interval = 1;
        Serial.println("116");
      }
    }
  }
  yield();
  if (interval == 1)
  {
    ledIntervalStatus.on();
  }
  else
  {
    ledIntervalStatus.off();
  }
  
  if (Mode == 1)
  {
    if (interval == 0)
    {
      SocketOff();
    }
    else
    {
      SocketOn();
    }
  }
}