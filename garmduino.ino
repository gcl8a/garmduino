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

//#include "wiring_private.h" // pinPeripheral() function

#include <SdFat.h>
#include <gps.h>
#include <BME280.h>
#include <LSM9DS1.h>

const int chipSelect = 9;

char filename[16]; //12 characters max

/*
 * setup serial for the GPS on SERCOM2: 
 * Arduino pin 3 -> sercom 2:1* -> RX
 * Arduino pin 4 -> sercom 2:0* -> TX
 */
//Uart gpsSerial (&sercom2, 3, 4, SERCOM_RX_PAD_1, UART_TX_PAD_0);

GPS_EM506 gps(&Serial1);
BME280 altimeter280;
LSM9DS1 imu;

//void SERCOM2_Handler()
//{
//  gpsSerial.IrqHandler();
//}

SdFat SD;

void setup() 
{
  delay(2000);
  SerialUSB.begin(115200);
  SerialUSB.println("Hej.");

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  
  //while(!SerialUSB){}
  
  //assign pins 3 & 4 SERCOM functionality
//  gpsSerial.begin(9600);
//  pinPeripheral(3, PIO_SERCOM_ALT);
//  pinPeripheral(4, PIO_SERCOM_ALT);

  Serial1.begin(9600);

  delay(2000);

  gps.Init();
  
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
  if (!SD.begin(chipSelect, SPI_QUARTER_SPEED)) 
  {
    SerialUSB.println(F("Card failed."));
  }

  SerialUSB.println(F("Setup complete."));

  SerialUSB.println(F("Checking for signal."));
  SerialUSB.println(F("Press return at any time to skip file on fix.\n"));
  while(!(gps.CheckSerial() & RMC) && !SerialUSB.available()) {}

  GPSDatum gpsDatum = gps.GetReading();  
  
  //now we have a fix -- make a filename
  int fileCount = 0;
  bool fileSuccess = false;
  
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

  //no longer need RMC
  gps.SetActiveNMEAStrings(GGA);
  //gps.SetReportTime(2000);
  //gps.SendNMEA("PMTK314,0,5,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");

  SerialUSB.println("Altimeter...");
  //start the altimeter running...(could toggle on GPS, when active)
//  altimeter3115.toggleOneShot();

  BME280Settings settings; //use the defaults
  altimeter280.Init(settings);
  altimeter280.ForceReading();

  SerialUSB.println("IMU...");
  IMUSettings imuSettings; //use defaults
  imu.Init(imuSettings);

  SerialUSB.println(filename);
  SerialUSB.println(F("Done."));
}

void loop() 
{
  if(gps.CheckSerial() & GGA) //if we have a GGA
  {
    String reportStr = gps.GetReading().MakeDataString() + ','
                    + altimeter280.MakeDataString() + ','
                    + imu.CalcRPY().MakeDataString();

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
}

int WriteSD(String filename, String str)
{
  SerialUSB.println(str);
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
    SerialUSB.print(F("Error opening file: "));
    SerialUSB.println(filename);
    return 0;
  }
}

