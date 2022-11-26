/*
 * Test program for trying out binary protocol and enabling SBAS
 */

#include <TinyScreen.h>
TinyScreen display = TinyScreen(TinyScreenPlus);

#include <event_timer.h>

#define SerialMonitor SerialUSB

#include <gps.h>
GPS_JF2 gps(&Serial);

Timer pollTimer;

void setup() 
{
  delay(2000);
  SerialMonitor.begin(115200);
  //while(!SerialMonitor) {};
  SerialMonitor.println("Hej");

  SerialMonitor.println(F("Init display"));
  display.begin();
  display.setBrightness(5);
  PrintScreen("Hej.");

  SerialMonitor.print(F("GPS..."));
  gps.Init();

  delay(2000);
  
  SerialMonitor.println();
  gps.SetProtocol(GPS_BINARY);

  delay(1000);
  gps.SetSBAS();

  delay(1000);
  gps.RequestFullPower();

  //gps.SetProtocol(GPS_NMEA);

  pollTimer.Start(5000ul);
  
  SerialMonitor.println(F("Done."));
}

void loop() 
{
//  if(gps.CheckSerial() & GGA) //if we have a GGA
//  {
//    PrintScreen(String(gps.GetReading().gpsFix) + String(',') + String(millis()));
//    SerialMonitor.println(gps.GetReading().MakeDataString());
//  }
  
  if(gps.CheckSerialBinary() == COMPLETE) 
  {
    vector_u8 msg = gps.GetMessage();
    
    if(msg.Length() > 0) 
    {
      SerialMonitor.print(msg[0], DEC);
      if(msg[0] == 41)
      {
        String scrStr = String(msg[1]) + String(msg[2]) + String(msg[3]) + String(msg[4]);
        PrintScreen(scrStr);
      }
    
      SerialMonitor.print(": ");
      for(uint16_t i = 0; i < msg.Length(); i++)
      {
        SerialMonitor.print(msg[i], HEX);
        SerialMonitor.print(' ');
      }
      SerialMonitor.print('\n');
    }
  }

  if(pollTimer.CheckExpired())
  {
    pollTimer.Restart();
    //gps.QueryMID(218);
    //gps.QueryMID(27);
    //gps.RequestMID(151); //does nothing...annoying
    //gps.RequestMID(133);
    //gps.QueryMID(27);
    //gps.PollPowerMode(); //does nothing...annoying

    SerialMonitor.println(String(millis()));
    PrintScreen(String(millis()));
  }
}

int PrintScreen(const String& inputString)
{
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.setCursor(0,10);
  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.print(inputString);

  display.setCursor(0,40);
  display.fontColor(TS_8b_Blue,TS_8b_Black);
  display.print(analogRead(A4));

  return 0;
}

