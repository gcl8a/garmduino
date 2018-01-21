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

//#define USE_TRADITIONAL_SPI //someday I'll move this to a library
//
//#include <SPI.h>
#include "wiring_private.h" // pinPeripheral() function

#include <SD.h>
#include <gps.h>

const int chipSelect = 10;

char filename[16]; //12 characters max

/*
 * setup serial for the GPS on SERCOM2: 
 * Arduino pin 3 -> sercom 2:1* -> RX
 * Arduino pin 4 -> sercom 2:0* -> TX
 */
Uart gpsSerial (&sercom2, 3, 4, SERCOM_RX_PAD_1, UART_TX_PAD_0);

GPS_MTK3339 gps1(&Serial1);

void SERCOM2_Handler()
{
  gpsSerial.IrqHandler();
}

void setup() 
{
  delay(3000);
  SerialUSB.begin(115200);
  SerialUSB.println("Hej.");
  //while(!SerialUSB){}
  
  //assign pins 3 & 4 SERCOM functionality
  gpsSerial.begin(9600);
  pinPeripheral(3, PIO_SERCOM_ALT);
  pinPeripheral(4, PIO_SERCOM_ALT);

  Serial1.begin(9600);

  delay(2000);

  gps1.Init();
  
  //set up sercom1 with SPI
//  SPIClass spi1 (&sercom1, 12, 13, 11, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
//
//  spi1.begin(); //why first?????
// 
//  // Assign pins 11, 12, 13 to SERCOM functionality
//  pinPeripheral(11, PIO_SERCOM);
//  pinPeripheral(12, PIO_SERCOM);
//  pinPeripheral(13, PIO_SERCOM);

  SerialUSB.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) 
  {
    SerialUSB.println(F("Card failed."));
  }

  SerialUSB.println(F("Setup complete."));
  
  SerialUSB.println(F("Waiting for signal."));

  while(!(gps1.CheckSerial() & RMC)) {} //keep checking until we get a fix
  SerialUSB.println(F("Fix."));

  //now we have a fix -- make a filename
  int monthday = gps1.GetReading().MakeMonthDay();
  int fileCount = 0;
  int fileSuccess = 0;
  
  while(!fileSuccess)
  {
    sprintf(filename, "%04i%04i.csv", monthday, fileCount);
    if(SD.exists(filename))
    {
      fileCount++;
    }
    else
    {
      fileSuccess = 1;
    }
  }

  gps1.SendNMEA("PMTK314,0,5,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");

  SerialUSB.println(filename);
  SerialUSB.println(F("Done."));
}

void loop() 
{
  if(gps1.CheckSerial() == (GGA | RMC)) //careful on the comparison, both GGA and RMC must be enabled
  {
    //gps1.ReadTemperature();
    WriteSD(filename, gps1.GetReading().MakeShortDataString());  
  }
}
//
//float GetTemperature(int pin)
//{
//  int adc = analogRead(pin);
//  float temperature = (((adc / 1024.) * 5.0) - 1.375) / 0.0225;
//
//  return temperature;
//}

int WriteSD(String filename, String str)
{
  SerialUSB.println(str);
  File dataFile = SD.open(filename, FILE_WRITE);

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
    SerialUSB.print(F("Error opening file: "));
    SerialUSB.println(filename);
    return 0;
  }
}

