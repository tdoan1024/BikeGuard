
#include <board.h>
#include <WioLTEforArduino.h>
#include <BreakoutSDK.h>
#include <stdio.h>
// Install https://github.com/Seeed-Studio/Grove_Ultrasonic_Ranger
#include <Ultrasonic.h>
#include <ublox_sara_r4_gnss.h>

/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device
    You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
    https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "c7229ac4fe1da7b7168e46bf05125671";

UBLOX_SARA_R4_GNSS gnss = UBLOX_SARA_R4_GNSS();
/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

#define ULTRASONIC_PIN  (38)
#define INTERVAL        (1000)
#define SEND_INTERVAL (10 * 60 * 1000)
WioLTE Wio;

int carCheck = 0;

Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

/**
   Setting up the Arduino platform. This is executed once, at reset.
*/
void setup() {
  gnss.open_GNSS();
  delay(3000);
  SerialDebug.println("_Start");
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
}

void sendCommand(const char * command) {
  if (breakout->sendTextCommand(command) == COMMAND_STATUS_OK) {
    LOG(L_INFO, "Tx-Command [%s]\r\n", command);
  } else {
    LOG(L_INFO, "Tx-Command ERROR\r\n");
  }
}

String queryGPS() {
  gnss_data_t data;
  LOG(L_INFO, "Getting GPS data...\r\n");
  breakout->getGNSSData(&data);
    if (data.valid) {
      LOG(L_INFO, "GPS data is OK.\r\n");
    } else {
      LOG(L_WARN, "GPS data not valid! Check reception.\r\n");
    }
  String latdeg = String(data.position.latitude_degrees);
  String latmin = String(data.position.latitude_minutes);
  String londeg = String(data.position.longitude_degrees);
  String lonmin = String(data.position.longitude_minutes);
  // default values for isNorth,isWest
  String isNorth = "N";
  String isWest = "W";
  isNorth = data.position.is_north ? "N" : "S" ;
  isWest = data.position.is_west ? "W" : "E";
  String gpsinfo = String(latdeg + "." + latmin + isNorth + "," 
          + londeg + "." + lonmin + isWest);
  return gpsinfo;
  }

 int checkUltrasonic() {
  long distance = UltrasonicRanger.MeasureInCentimeters();
  if (distance <= 3.8 && carCheck == 0) {
    carCheck = 1;
    SerialUSB.println("Bike is now locked!");
    return 1;
  
  }else if(distance > 3.8 && carCheck == 1 ) {
     // Send message to server how long car been parked
    carCheck = 0;
    SerialUSB.println("Bike is now unlocked!");
    return 0;
  }
 }

void loop()
{
  //Format: "0 0 +0.000N,+0.000W"
  double distance;
// static unsigned long last_send = 0;

//  gnss_data_t data;
//  breakout->getGNSSData(&data);
//
//  if  ((last_send == 0) || (millis() - last_send >= SEND_INTERVAL)) {
//    last_send = millis();

    String gps = queryGPS();
    String ultra;
    String acce;
    if (checkUltrasonic() == 0)
      ultra= "0";
    else ultra = "1";

    String outgoinginfo = String(ultra+" "/*+acce*/+gps);
    
    char commandText[outgoinginfo.length()+1];
    outgoinginfo.toCharArray(commandText,outgoinginfo.length()+1);
    LOG(L_INFO,"[%s]\r\n",commandText);
    if(breakout->isPowered()){
      SerialUSB.println(commandText);
      sendCommand(commandText);

      breakout->spin();
    } else {
      LOG(L_WARN, "Modem is powered down. Data not sent.\r\n");
    }
//    LOG(L_INFO, "Current Latitude [%d] degrees\r\n", data.position.latitude_degrees);

//    snprintf(gpscommandText, 512, "Current Position:  %d %7.5f %s %d %7.5f %s\r\n", 
//        data.position.latitude_degrees,
//        data.position.latitude_minutes, 
//        data.position.is_north 
//        ? "N" : "S", 
//        data.position.longitude_degrees,
//        data.position.longitude_minutes,
//        data.position.is_west
//        ? "W" : "E");
//    SerialUSB.println(gpscommandText);
//    if (data.valid) {
//      sendCommand(gpscommandText);
//    }

   

  }

  

  delay(INTERVAL);
}
