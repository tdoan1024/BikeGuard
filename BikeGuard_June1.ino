 /******************************************************************************************************************************
 * BikeGuard Microcontroller Code
 * Authors: Austin Pischer & Tai Doan
 * For SPU Junior Design Spring 2019
 ******************************************************************************************************************************/


/*******************************************************************************************************************************
                                                        Includes
*******************************************************************************************************************************/

/************************GPS Code**************************************/
#include <board.h>
#include <BreakoutSDK.h> // Comes from https://www.twilio.com/blog/pioneer-nb-iot-with-twilios-alfa-development-kit
#include <stdio.h>
/**********************************************************************/


/************************Ultrasonic Sensor Code************************/
// Install https://github.com/Seeed-Studio/Grove_Ultrasonic_Ranger
#include <Ultrasonic.h>
/**********************************************************************/


/************************Accelerometer Code****************************/
#include "SparkFunLSM6DS3.h"  // Accelerometer Library with SPI disabled.
#include "Wire.h"             // I2C Library?
/**********************************************************************/


/************************Transmission Code*****************************/
//???
#include <WioLTEforArduino.h>
/**********************************************************************/


/*******************************************************************************************************************************
                                                        Defines
*******************************************************************************************************************************/

/************************Transmission Code*****************************/
#define INTERVAL        (1000)
#define SEND_INTERVAL (10 * 60 * 1000)
/**********************************************************************/


/************************Ultrasonic Sensor Code************************/
#define ULTRASONIC_PIN  (38)

#define LOCK_NOT_PRESENT (0)
#define LOCK_PRESENT (1)
#define LOCK_WARNING (2)

#define LOCK_COUNT_THRESHOLD (3)
#define DEFAULT_DISTANCE_CM (2)
/**********************************************************************/


/************************Accelerometer Code****************************/
#define POS_ACC_VALUE (2)
#define NEG_ACC_VALUE (-2)
#define TIMER_COUNT_RESET_VALUE (9)
#define TAMPER_COUNT_WARNING_THRESHOLD (5)
/**********************************************************************/

#define SAFE_MODE (0)
#define WARN_MODE (1)


/*******************************************************************************************************************************
                                                        Globals
*******************************************************************************************************************************/

/************************Transmission Code*****************************/
/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device
    You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
    https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "c7229ac4fe1da7b7168e46bf05125671";

/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

WioLTE Wio;
/**********************************************************************/


/************************Ultrasonic Sensor Code************************/
Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

int lockCount; 
long ultDistance;
int lockFlag;
/**********************************************************************/


/************************Accelerometer Code****************************/
//Create a instance of class LSM6DS3
LSM6DS3 myIMU( I2C_MODE, 0x6A );  //I2C device address 0x6A

int timerCount, tamperCount;
float currentX, currentY, currentZ;
/**********************************************************************/

// Timing code
unsigned long currentMS;
unsigned long lastSafeMS;
unsigned long lastWarnMS;
const unsigned long safeFreqMS = (1000 * 10);   // 20 Second updates when safe
const unsigned long warnFreqMS = (1000 * 10);   // 10 Second updates when warned
const unsigned long warnThreshMS = (1000 * 60); // 60 Second warning timeout
int warnFlag;


/*******************************************************************************************************************************
                                                        Setup
*******************************************************************************************************************************/
void setup()
{
  warnFlag = SAFE_MODE;
  /************************Transmission Code*****************************/
  // Feel free to change the log verbosity. E.g. from most critical to most verbose:
  //   - errors: L_ALERT, L_CRIT, L_ERR, L_ISSUE
  //   - warnings: L_WARN, L_NOTICE
  //   - information & debug: L_INFO, L_DB, L_DBG, L_MEM
  // When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.
  owl_log_set_level(L_INFO);
  LOG(L_WARN, "Arduino setup() starting up\r\n");

  // Set the Breakout SDK parameters
  breakout->setPurpose(device_purpose);
  breakout->setPSKKey(psk_key);
  breakout->setPollingInterval(10 * 60);  // Optional, by default set to 10 minutes

  // Powering the modem and starting up the SDK
  LOG(L_WARN, "Powering on module and registering...");
  breakout->powerModuleOn();

  LOG(L_WARN, "... done powering on and registering.\r\n");
  LOG(L_WARN, "Arduino loop() starting up\r\n");
  /**********************************************************************/


  /************************Accelerometer Code****************************/
  SerialUSB.begin(9600);  
  //Call .begin() to configure the IMUs
  if( myIMU.begin() != 0 )
  {
    SerialUSB.println("Accelerometer device error!");
  }
  else  
  {
      SerialUSB.println("Accelerometer device OK!");
  }

  // Make sure the tamperCount variable is 0 upon first entry into our infinite loop.
  tamperCount = 0;
  /**********************************************************************/


  /************************Ultrasonic Sensor Code************************/
  lockCount = 0;
  lockFlag = 0;
  /**********************************************************************/
}




/*******************************************************************************************************************************
                                                        Functions
*******************************************************************************************************************************/


/************************Transmission Code*****************************/
void sendCommand(const char * command) 
{
  if (breakout->sendTextCommand(command) == COMMAND_STATUS_OK) 
  {
    LOG(L_INFO, "Tx-Command [%s]\r\n", command);
  } 
  
  else 
  {
    LOG(L_INFO, "Tx-Command ERROR\r\n");
  }
}
/**********************************************************************/


/************************GPS Code**************************************/
String queryGPS() 
{
  // Create an instance of GPS data storage.
  gnss_data_t data;

  // Alert the debug that we are getting GPS data.
  LOG(L_INFO, "Getting GPS data...\r\n");
  // Grab GPS data from device.
  breakout->getGNSSData(&data);

  // Check to see if data is valid.
  if (data.valid) 
  {
    // If data is valid, tell the user so.
    LOG(L_INFO, "GPS data is valid.\r\n");
  } 

  else 
  {
    // If data is invalid, tell the user so.
    LOG(L_WARN, "GPS data is NOT valid! Check reception.\r\n");
  }

  // Generate strings based off of the data we recieved
  String latdeg = String(data.position.latitude_degrees);
  String latmin = String(data.position.latitude_minutes);
  String londeg = String(data.position.longitude_degrees);
  String lonmin = String(data.position.longitude_minutes);
  
  // default values for isNorth,isWest
  String isNorth = "N";
  String isWest = "W";

  // Determine whether or not our coordinates are N/S and W/E.
  isNorth = data.position.is_north ? "N" : "S" ;
  isWest = data.position.is_west ? "W" : "E";

  String space = " ";
  // Create the whole string from all the data strings we have now.
  String gpsInfo = String(data.position.latitude_degrees + space + data.position.latitude_minutes + isNorth + space +
                          data.position.longitude_degrees+ space + data.position.longitude_minutes+ isWest   );

  //Returned the combined string.
  return gpsInfo;
  }
/**********************************************************************/


/*******************************************************************************************************************************
                                                        Loop
*******************************************************************************************************************************/
void loop()
{
  //Reset to safe mode after 1 minute
  if( (warnFlag == WARN_MODE) && ( ( (currentMS = millis()) - lastWarnMS ) > warnThreshMS) )
  {
    warnFlag = SAFE_MODE;
    lockFlag = LOCK_NOT_PRESENT;
    String ult = "0";
    
  }

  /************************Ultrasonic Sensor Code************************/
  String ult = "0";
  ultDistance = UltrasonicRanger.MeasureInCentimeters();
  SerialUSB.print("Ult Distance = ");
  SerialUSB.print(ultDistance);
  SerialUSB.print(" cm");
  
  // Lock is inserted, lock is not default distance and lock wasn't present
  if( ultDistance != DEFAULT_DISTANCE_CM && lockFlag == LOCK_NOT_PRESENT )
  {
    ult = "0";
    lockFlag = LOCK_PRESENT;
  }

  // Lock is removed after lock was inserted and we read default distance
  else if( ultDistance == DEFAULT_DISTANCE_CM && lockFlag == LOCK_PRESENT )
  {
    lockCount++;    
    if (lockCount > LOCK_COUNT_THRESHOLD)
    {
      ult = "1";
      lockFlag = LOCK_NOT_PRESENT;
      warnFlag = WARN_MODE;
      SerialUSB.println(" Lock removed!");
    }
  }

  else 
  {
    lockCount = 0;
    ult = "0";
    SerialUSB.println(" No lock was ever present!");
  }
  
  
  
  /**********************************************************************/


  /************************Accelerometer Code****************************/

  String acce = "0";
  
  //Grab accelerometer data
  currentX = myIMU.readFloatAccelX();
  currentY = myIMU.readFloatAccelY();
  currentZ = myIMU.readFloatAccelY();

  //Print accelerometer data on SerialUSB Monitor, with 4 digits for each value
  SerialUSB.print("Accelerometer Values: X = ");
  SerialUSB.print(currentX, 4);
  SerialUSB.print(" / Y = ");
  SerialUSB.print(currentY, 4);
  SerialUSB.print(" / Z = ");
  SerialUSB.println(currentZ, 4);

  // Check for excess acceleration
  if( 
      (currentX > POS_ACC_VALUE) || (currentX < NEG_ACC_VALUE) ||
      (currentY > POS_ACC_VALUE) || (currentY < NEG_ACC_VALUE) ||
      (currentZ > POS_ACC_VALUE) || (currentZ < NEG_ACC_VALUE)   )
  {
    tamperCount++;
    SerialUSB.print("The bike is under attack! tamperCount updated.   ");
  }
      
  else
  {
    SerialUSB.print("The bike is fine. tamperCount NOT updated.   ");
  }

  // Check to see if we have polled the accelerometer *10* times
  if( timerCount < TIMER_COUNT_RESET_VALUE )
  {
    // If we havee not yet polled *10* times, increment the number of times we have polled
    timerCount++;
    
    SerialUSB.print("tamperCount = ");
    SerialUSB.print(tamperCount);
    SerialUSB.print(" timerCount = ");
    SerialUSB.println(timerCount);
  }
  
  // If we have polled the accelerometer *10* times, evaluate the number of times that there was excess acceleration
  else
  {
    
    SerialUSB.print("tamperCount = ");
    SerialUSB.print(tamperCount);
    SerialUSB.print(" timerCount = ");
    SerialUSB.print(timerCount);

    // Warn the user if there was excess acceleration during the previous *10* polls
    if( tamperCount >= TAMPER_COUNT_WARNING_THRESHOLD )
    {
      acce = "1";
      warnFlag = WARN_MODE;
      SerialUSB.println("   We've acc warned the user!");
    }

    // Don't warn the user if there wasn't excess acceleration during the previous *10* polls
    else
    {
      SerialUSB.println("   No acc warning.");
      acce = "0";
    }

    //Reset the timer and tamper counts to prepare for the next *10* polls
    timerCount = 0;
    tamperCount = 0;
  }
  /**********************************************************************/

  /************************GPS Code**************************************/
  String gps = queryGPS(); // Get GPS data
  /**********************************************************************/


  /************************Transmission Code*****************************/
  // Combine the ultrasoic sensor data, the accelerometer data, and the gps data into a single string
  String outgoingInfo = String(ult+" "+acce+" "+gps);

  // Make a buffer for the command text that is larger than
  char commandText[outgoingInfo.length()+1];
  
  outgoingInfo.toCharArray(commandText,outgoingInfo.length()+1);
  
  if(breakout->isPowered())
  {
    SerialUSB.print("Potential Message: ");
    SerialUSB.println(commandText);

    if(acce == "1" || ult == "1")
    {
      delay(10000);
      sendCommand(commandText);
      breakout->spin();
      delay(10000);
    }
    
    //send at warn frequency
    if(warnFlag == WARN_MODE && ( (currentMS = millis()) - lastWarnMS ) > warnFreqMS )
    {
      sendCommand(commandText);
      breakout->spin();
      lastWarnMS = millis();
      SerialUSB.println("   ... Message Sent. (warn mode)\n");
    }

    //send at safe frequency
    else if (warnFlag == SAFE_MODE && ( (currentMS = millis()) - lastSafeMS ) > safeFreqMS )
    {
      sendCommand(commandText);
      breakout->spin(); 
      lastSafeMS = millis();\
      SerialUSB.println("   ... Message Sent. (safe mode)\n");
    }

    else SerialUSB.println("   ... No message sent.\n");
    
  } 
  
  else 
  {
    LOG(L_WARN, "Modem is powered down. Data not sent.\r\n");
  }
  /**********************************************************************/
  
  delay(INTERVAL);
}

/* END OF FILE */
