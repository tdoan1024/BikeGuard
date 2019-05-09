
#include <board.h>
#include <WioLTEforArduino.h>
#include <BreakoutSDK.h>
#include <stdio.h>
// Install https://github.com/Seeed-Studio/Grove_Ultrasonic_Ranger
#include <Ultrasonic.h>

/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device
    You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
    https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "c7229ac4fe1da7b7168e46bf05125671";

/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

#define ULTRASONIC_PIN  (38)
#define INTERVAL        (1000)
#define SEND_INTERVAL (1 * 60 * 1000)
WioLTE Wio;

int  counter = 0;
int carCheck = 0;
Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

/**
   Setting up the Arduino platform. This is executed once, at reset.
*/
void setup() {

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
  breakout->setPollingInterval(1 * 60);  // Optional, by default set to 10 minutes

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

void loop()
{
  double distance;
  static unsigned long last_send = 0;

  gnss_data_t data;
  breakout->getGNSSData(&data);

  if  ((last_send == 0) || (millis() - last_send >= SEND_INTERVAL)) {
    last_send = millis();

    
    char gpscommandText[512];
    LOG(L_INFO, "Current Latitude [%d] degrees\r\n", data.position.latitude_degrees);
    SerialUSB.println("Current Latitude [%d] degrees\r\n", data.position.latitude_degrees);
    LOG(L_INFO, "Current Latitude [%d] minutes\r\n", data.position.latitude_minutes);
    SerialUSB.println("Current Latitude [%d] minutes\r\n", data.position.latitude_minutes);
    LOG(L_INFO, "Current Longitude [%d] degrees\r\n", data.position.longitude_degrees);
    SerialUSB.println("Current Longitude [%d] degrees\r\n", data.position.longitude_degrees);
    LOG(L_INFO, "Current Latitude [%d] minutes\r\n", data.position.longitude_minutes);
    SerialUSB.println("Current Latitude [%d] minutes\r\n", data.position.longitude_minutes);
    snprintf(gpscommandText, 512, "Current Position:  %d %7.5f %s %d %7.5f %s\r\n", 
        data.position.latitude_degrees,
        data.position.latitude_minutes, 
        data.position.is_north 
        ? "N" : "S", 
        data.position.longitude_degrees,
        data.position.longitude_minutes,
        data.position.is_west
        ? "W" : "E");
    SerialUSB.println(gps);
    if (data.valid) {
      sendCommand(gpscommandText);
    }
 
  }
  
  distance = UltrasonicRanger.MeasureInCentimeters();
  // counters = pulseIn(ECHO_PIN, HIGH);
  if (distance <= 3.8 && carCheck == 0) {
    carCheck = 1;
    SerialUSB.println("Bike is now locked!");
  
  }else if(distance > 3.8 && carCheck == 1 ) {
     // Send message to server how long car been parked
    carCheck = 0;
    SerialUSB.println("Bike is now unlocked!");

  } else{
    //SerialUSB.println("No car here "); // Wait 1 minute
    //SerialUSB.println("*go into Power Saving Mode, wait for one minute before next paging cycle*");
    counter = 0;
    

  }
  breakout->spin();

  delay(INTERVAL);
}
