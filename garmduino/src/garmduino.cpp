/*
 * Forked code for SAMD21 dev. Makes a "traditional" SPI on sercom 1 (Arduino pins 11, 12, 13):
 * 
 * 11: MOSI on sercom 1:0
 * 12: MISO on sercom 1:3
 * 13: CLK on sercom 1:1
 * 
 * When it first makes connection over GPS, it uses the month and data
 * to create a filename, then writes data to it. Multiple files on the same day are numbered 
 * sequentially.
 */

#include <TinyScreen.h>
//define the display
TinyScreen display = TinyScreen(TinyScreenPlus);

#include <event_timer.h>

#define SerialMonitor SerialUSB

#include <SdFat.h>
#include <gps.h>
#include <BME280.h>
#include <LSM9DS1.h>

const int SD_CS = 10;

String filename;

/*
 * setup serial for the GPS on SERCOM2: 
 * Arduino pin 3 -> sercom 2:1* -> RX
 * Arduino pin 4 -> sercom 2:0* -> TX
 */
//Uart gpsSerial (&sercom2, 3, 4, SERCOM_RX_PAD_1, UART_TX_PAD_0);

GPS_JF2 gps(&Serial);
BME280 altimeter280;
LSM9DS1 imu;

SdFat SD;

uint8_t screenState = ON;
Timer screenTimer;

void setup() 
{
  delay(2000);
  SerialMonitor.begin(115200);
  //while(!SerialMonitor) {};
  SerialMonitor.println("Hej.");

  SerialMonitor.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS, SPI_QUARTER_SPEED)) 
  {
    SerialMonitor.println(F("Card failed."));
  }
  else SerialMonitor.println("done");

  SerialMonitor.println(F("Init display"));
  
  display.begin();

  //sets main current level, valid levels are 0-15
  display.setBrightness(5);

  PrintScreen("Hej.");

  SerialMonitor.print(F("GPS..."));
  gps.Init();
  
  filename = CreateNewFile();

  SerialMonitor.println("Altimeter...");
  //start the altimeter running...(could toggle on GPS, when active)
//  altimeter3115.toggleOneShot();

  BME280Settings settings; //use the defaults
  altimeter280.Init(settings);
  altimeter280.ForceReading();

  SerialMonitor.println("IMU...");
  IMUSettings imuSettings; //use defaults
  imu.Init(imuSettings);
  display.clearScreen();
  String message(filename);
  PrintScreen(message);

  screenTimer.Start(1000);

  SerialMonitor.println(F("Done."));
}

void loop() 
{
  if(gps.CheckSerial() & GGA) //if we have a GGA
  {
    String reportStr = gps.GetReading().MakeDataString() + ','
                    + altimeter280.MakeDataString() + ','
                    + imu.CalcRPY().MakeDataString();

    reportStr += ',' + String(analogRead(A4) * 0.006445);

    WriteSD(filename, reportStr);  
  }

  if(imu.IsAvailableAccelAndGyro())
  {
    imu.ProcessReadings();
  }

  if(altimeter280.CheckForNewDatum())
  {
    altimeter280.ReadDatum();
    altimeter280.ForceReading();
  }

  static uint8_t llButtonPrev = 0;
  uint8_t llButton = display.getButtons(TSButtonLowerLeft);
  if(llButton != llButtonPrev)
  { 
    if(llButton) filename = CreateNewFile();
    llButtonPrev = llButton;
  }

  static uint8_t urButtonPrev = 0;
  uint8_t urButton = display.getButtons(TSButtonUpperRight);
  if(urButton != urButtonPrev)
  { 
    if(urButton) 
    {
      if(screenState)
      {
        //turn it off
        screenState = 0;
        display.off();
      }
      else
      {
        //turn it on
        screenState = 1;
        display.on();
      }
    }
    
    urButtonPrev = urButton;
  }

  static uint8_t lrButtonPrev = 0;
  uint8_t lrButton = display.getButtons(TSButtonLowerRight);
  if(lrButton != lrButtonPrev)
  { 
    if(lrButton) TogglePower();
    lrButtonPrev = lrButton;
  }
  
  if(screenTimer.CheckExpired())
  {
    if(screenState) DrawScreen();
    //SerialUSB.println(millis());
    screenTimer.Restart();
  }
  
}

int WriteSD(String filename, String str)
{
  SerialMonitor.println(str);
  File dataFile = SD.open(filename, O_WRITE | O_CREAT | O_APPEND);

  // if the file is available, write to it:
  if (dataFile) 
  {
    dataFile.println(str);
    dataFile.close();
    // print to the serial port too:

    return 1;
  }
  
  // if the file isn't open, pop up an error:
  else 
  {
    SerialMonitor.print(F("Error opening file: "));
    SerialMonitor.println(filename);
    
    return 0;
  }
}

int PrintScreen(String inputString)
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

int DrawScreen(void)
{
  display.clearScreen();

  //display the gps fix
  int fix = gps.GetReading().gpsFix;
  
  display.drawRect(76, 10, 10, 10, TSRectangleFilled, fix == 2 ? TS_8b_Green : (fix == 1 ? TS_8b_Yellow : TS_8b_Red));

  //filename
  display.setFont(thinPixel7_10ptFontInfo);
  display.setCursor(10, 20);
  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.print(filename);

  //battery level
  display.setFont(thinPixel7_10ptFontInfo);
  display.setCursor(10, 35);
  display.fontColor(TS_8b_Blue,TS_8b_Black);
  display.print(analogRead(A4) * 0.006445);

  //altitude
  display.setFont(thinPixel7_10ptFontInfo);
  display.setCursor(10, 50);
  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.print(altimeter280.GetReading().altitude);

  return 0;
}

String CreateNewFile(void) //creates a new file of the form MMDDxxxx.csv; user can override GPS fix to get 0000xxxx.csv
{
  PrintScreen("Creating new file.");

  //RMC has month and dat in it  
  gps.SetActiveNMEAStrings(RMC);

  SerialMonitor.println(F("Press return at any time to skip file on fix.\n"));
  while(!(gps.CheckSerial() & RMC) && !display.getButtons(TSButtonUpperLeft)) {}

  GPSDatum gpsDatum = gps.GetReading();
  
  //now we have a fix -- make a filename
  int fileCount = 0;
  bool fileSuccess = false;

  char filename[16]; //12 characters max
  while(!fileSuccess)
  {
    sprintf(filename, "%02i%02i%04i.csv", gpsDatum.month, gpsDatum.day, fileCount);
    if(SD.exists(filename))
    {
      fileCount++;
    }
    else
    {
      fileSuccess = 1;
    }
  }

  PrintScreen(filename);
  
  //no longer need RMC
  gps.SetActiveNMEAStrings(GGA);

  return String(filename);
}

void TogglePower(void)
{
  Serial.println("Power!");
  gps.SetProtocol(GPS_BINARY);
  delay(100);
  gps.RequestTricklePower();
  delay(100);
  gps.SetProtocol(GPS_NMEA);
}


