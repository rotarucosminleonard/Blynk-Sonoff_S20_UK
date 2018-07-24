

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

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  This example shows how value can be pushed from Arduino to
  the Blynk App.

  WARNING :
  For this example you'll need Adafruit DHT sensor libraries:
    https://github.com/adafruit/Adafruit_Sensor
    https://github.com/adafruit/DHT-sensor-library

  App project setup:
    Value Display widget attached to V5
    Value Display widget attached to V6
 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
WidgetRTC rtc;
BlynkTimer timer;


/* Virtual Pins */
/* V1 - Scheduled/Manual */
/* V2 - Time Interval Input */
/* V3 - LedTimerInterval Status */
/* V4 - Software button for ON/OFF */
/* V5 - Hardware Push Button for ON/OFF Button */
/* V6 - LED status for ON/OFF*/
/* V7 - ImNotAtHome status */



WidgetLED ledIntervalStatus(V3);
WidgetLED ledSocketStatus(V6);

bool interval;
bool scheduled;
bool SocketStatus;
bool ImNotAtHome;

bool result;  // monitoring the connection

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SSID";
char pass[] = "PW";


#define relay 12 //  relay
#define PushButton 0 // 
#define ConnectionStatus 13


bool PushButtonState;
bool prevPushButtonState;
bool softwareButton;



//EC:FA:BC:8A:1F:D0
//byte arduino_mac[] = { 0xEC, 0xFA, 0xBC, 0x8A, 0x1F, 0xD0 };
IPAddress arduino_ip ( 192, 168, 1, 13);
//IPAddress dns_ip     (8, 8, 8, 8);
IPAddress gateway_ip ( 192, 168, 1, 1);
IPAddress subnet_mask(255, 255, 255, 0);


void setup()
{
  // Serial.begin(9600);
  WiFi.hostname("Blynk-Sonoff_S20_1");
  WiFi.mode(WIFI_STA);
  // Debug console

  WiFi.config(arduino_ip, gateway_ip, subnet_mask);

  // Blynk.begin(auth, ssid, pass);
  yield();
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
  Blynk.begin(auth, ssid, pass, IPAddress(192, 168, 1, 3), 8080);

  // Setup a function to be called every second
  timer.setInterval(2000L, timesync);
  timer.setInterval(1000L, scheduling);
  timer.setInterval(1000L, ledMonitoring);
  timer.setInterval(5000L, connecting);
  timer.setInterval(3000L, NotAtHome);

  yield();

  pinMode (relay, OUTPUT);
  pinMode (PushButton, INPUT);
  pinMode (ConnectionStatus, OUTPUT);



  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    yield();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    //   Serial.println("Start updating " + type);
  });
  //  ArduinoOTA.onEnd([]() {
  //    Serial.println("\nEnd");
  //  });
  //  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  //  });
  //  ArduinoOTA.onError([](ota_error_t error) {
  //    Serial.printf("Error[%u]: ", error);
  //    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  //    yield();
  //  });
   ArduinoOTA.setHostname("Blynk-Sonoff_S20_1"); // OPTIONAL NAME FOR OTA
  yield();
  ArduinoOTA.begin();
  yield();
  //  Serial.println("Ready");
  //  Serial.print("IP address: ");
  //  Serial.println(WiFi.localIP());


}

void loop()
{

  yield();
  ArduinoOTA.handle();
  yield();
  Blynk.run();
  yield();
  timer.run();
  yield();
  PushButtonONOFF();
  yield();
  result = Blynk.connected();
  yield();


}

void connecting()
{
  if (result == 0)
  {
    digitalWrite(ConnectionStatus, HIGH);
  }
  else
  {
    digitalWrite(ConnectionStatus, LOW);
  }

}

void ledMonitoring()
{
  if (interval == 1)
  {
    ledIntervalStatus.on();
    yield();
  }
  else
  {
    ledIntervalStatus.off();
    yield();
  }

  if (SocketStatus == 1)
  {
    ledSocketStatus.on();
  }
  else
  {
    ledSocketStatus.off();
  }
}


BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V2, V4, V5);
  yield();
}

void timesync()
{
  Blynk.syncVirtual(V1);
  rtc.begin();
  yield();
}



BLYNK_WRITE(V4) // on / off - manual or scheduled
{
  softwareButton = param.asInt();
  if (softwareButton == 1)
  {
    SocketOn();
  }
  else
  {
    SocketOff();
  }
}


BLYNK_WRITE(V1)   // ON = scheduled   - OFF = manual(check temperature allways)
{
  scheduled = param.asInt();
  yield();
}

void scheduling()
{
  if (scheduled == 1)
  {
    if (interval == 1)
    {
      SocketOn();
    }
    else
    {
      SocketOff();
    }
  }
  //  else
  //  {
  //    // listen to the software button or the hardware button
  //  }
}

BLYNK_WRITE(V7)   // if i'm not at home keep it off
{
  ImNotAtHome = param.asInt();
  yield();
}

void NotAtHome()
{
  Blynk.syncVirtual(V7);
  if (ImNotAtHome == 1)
  {
    SocketOff();
  }
  else if (ImNotAtHome == 0)
  {
    if (V4 == 1)
    {
      SocketOn();
    }
    else if(V4 == 0)
    {
      SocketOn();
    }
  }
}

void PushButtonONOFF()
{
  /* Tratează cazul când s-a apăsat up pentru incrementarea tempset. */
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
  ledSocketStatus.on();
  Blynk.virtualWrite(V4, SocketStatus);
  //  Serial.println("SocketOn");
  yield();
}

void SocketOff()
{
  digitalWrite(relay, LOW);
  SocketStatus = 0;
  ledSocketStatus.off();
  Blynk.virtualWrite(V4, SocketStatus);
  //Serial.println("SocketOff");
  yield();
}

void PeriodicSync() // from server
{
  Blynk.syncVirtual(V1);
}




BLYNK_WRITE(V2) {    // Status of the hours interval
  TimeInputParam t(param);

  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  yield();
  Serial.print("Current time: ");
  Serial.print(currentTime);
  Serial.print("   ");


  Serial.print(String(" Start: ") +
               t.getStartHour() + ":" +
               t.getStartMinute() + ":" +
               t.getStartSecond());
  Serial.print(String("  Stop:") +
               t.getStopHour() + ":" +
               t.getStopMinute() + ":" +
               t.getStopSecond());
  if (interval == 1)
  {
    Serial.print(" = > In Interval!");
  }
  else
  {
    Serial.print(" = > In afara intervalului!!");
  }

  Serial.println();  Serial.println();
  // Pentru cazul11 in care ora de inceput este aceeasi cu ora de sfarsit. Atat ora de inceput cat si cea de sfarsit sunt egale
  if (t.getStartHour() == hour() && hour() == t.getStopHour())   // daca ora de inceput setata este egala cu ora actuala atunci verifica si minutele
  {
    //      Serial.println("Ora se potriveste, verifica minutele");
    if (t.getStartMinute() <= minute() && t.getStopMinute() > minute() )
    {
      //        Serial.println("Ora este aceeasi, minutele sunt intre start si end");
      interval = 1;
    }
    if (t.getStartMinute() > minute() )
    {
      //        Serial.println("Ora este aceeasi, dar minutele nu inca.In afara intervalului.");
      interval = 0;
    }
    if (t.getStopMinute() < minute() )
    {
      //        Serial.println("Ora este aceeasi, dar intervalul in minute a fost depasit");
      interval = 0 ;
    }
  }
  yield();
  // Pentru cazul in care doar stoptime este in aceeasi ora cu start time
  if (t.getStopHour() == hour() ) // daca ora de sfarsit setata se potriveste cu ora actuala verifica si minutele
  {
    if ( t.getStopMinute() <= minute()  )
    {
      //       Serial.println("Intervalul este depasit cu cateva minute, opreste");
      interval = 0;
    }
    if (t.getStopMinute() > minute() && minute() > t.getStartMinute() )
    {
      //       Serial.println("Ora de sfarsit a sosit.Mai sunt cateva minute.");
      interval = 1;
    }
  }
  if (t.getStopHour() == t.getStartHour() && t.getStopHour() != hour() )
  {
    //      Serial.println("Intervalul start - stop este in aceeasi ora, dar ceasul este cu una sau mai multe ore inainte sau dupa  eveniment!");
    interval = 0;
  }


  //daca ora de sfarsit are valoarea mai mica decat cea de inceput atunci intervalul se incheie ziua urmatoare, inainte de aceeasi ora.
  if (t.getStartHour() > t.getStopHour() )
  {
    //     Serial.println("Ora de sfarsit se incheie ziua urmatoare");
    // interval: 15:00 - 3:00 - ora 15
    if (t.getStartHour() == hour())
    {
      //       Serial.println("Ora este aceeasi, verifica minutul");
      if (t.getStartMinute() <= minute() )
      {
        //        Serial.println("Ora si minutul se potrivesc.In interval");
        interval = 1;
      }
      if (t.getStartMinute() > minute() )
      {
        //         Serial.println("Aceeasi ora, dar inca nu a venit minutul");
        interval = 0;
      }
    }
    // interval: 15:00 - 3:00 - ora 04:00
    if (t.getStartHour() > hour() && hour() > t.getStopHour() )
    {
      //       Serial.println("a trecut de interval.In afara intervalului!");
      interval = 0;
    }
    if (t.getStartHour() > hour() && hour() == t.getStopHour() )
    {
      //       Serial.println("Ultima ora din interval. verifica minutele!");
      if (t.getStopMinute() <= minute() )
      {
        //        Serial.println("Au trecut cateva minute peste interval. In afara intervalului!");
        interval = 0;
      }
      else
      {
        //         Serial.println("In ultima ora, dar mai sunt cateva minute.In interval!");
        interval = 1;
      }
    }
    yield();
    if (t.getStartHour() > hour() && hour() < t.getStopHour() )
    {
      //       Serial.println("Dupa miezul noptii, inainte de incheiera intervalului");
      interval = 1;
    }

    //inainte de  miezul noptii, in interval
    if (t.getStartHour() < hour() && hour() > t.getStopHour() )
    {
      //      Serial.println("Intervalul se incheie ziua urmatoare.Acum ora este Inainte de miezul noptii, in interval");
      interval = 1;
    }

  }

  //daca ora de inceput este mai mica decat ora de sfarsit atunci intervalul se incheie in aceeasi zi
  else if (t.getStartHour() < t.getStopHour() )
  {
    //   Serial.println("Ora de sfarsit se incheie in aceeasi zi");
    if (t.getStartHour() < hour() && t.getStopHour() > hour() )  // ora este mai mare decat ora starttime dar mai mica decat stoptime.
    {
      //      Serial.println("Se afla in intervalul de ore, nu in aceeasi ora ed sfarsit, deci nu e nevoie sa verific minutele");
      interval = 1;
    }

    if (t.getStartHour() == hour() )
    {
      //      Serial.println("Valoarea orei se potriveste cu intervalul, verifica si minutele");
      if (t.getStartMinute() <= minute())
      {
        //      Serial.println("Se potrivesc si minutele. in interval");
        interval = 1;
      }
      else
      {
        //      Serial.println("Minutele nu se potrivesc, In afara intervalului@");
        interval = 0;
      }
    }
    if (t.getStartHour() > hour() )
    {
      //    Serial.println("Inca nu s-a apropiat intervalul.In afara intervalului");
      interval = 0;
    }
    if (t.getStopHour() == hour())
    {
      //    Serial.println("Ultima ora din interval.verifica minutele");
      if (t.getStopMinute() <= minute() )
      {
        //       Serial.println("A depasit minutele.In afara intervalului!");
        interval = 0;
      }
      else if (t.getStopMinute() > minute() )
      {
        //       Serial.println("Nu au depasit minutele.In interval");
        interval = 1;
      }
    }

  }


  yield();
  //  Serial.println();
}


