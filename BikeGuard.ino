
#include <board.h>
#include <WioLTEforArduino.h>
#include <BreakoutSDK.h>
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
//poops
#define ULTRASONIC_PIN  (38)
#define ROTARY_ANGLE_PIN  (WIOLTE_20)
#define INTERVAL        (1000)

WioLTE Wio;

int  counter = 0;
int carCheck = 0;
Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

/**
   Setting up the Arduino platform. This is executed once, at reset.
*/
void setup() {

  pinMode(WIOLTE_D20, OUTPUT);

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


/**
   This is just a simple example to send a command and write out the status to the console.
*/

//void sendCommand(const char * command) {
//  if (breakout->sendTextCommand(command) == COMMAND_STATUS_OK) {
//    LOG(L_INFO, "Tx-Command [%s]\r\n", command);
//  } else {
//    LOG(L_INFO, "Tx-Command ERROR\r\n");
//  }
//}

void loop()
{
  double distance;
  distance = UltrasonicRanger.MeasureInCentimeters();
  // counters = pulseIn(ECHO_PIN, HIGH);
  if (counter == 20) {
    char message[] = "Your car has 10 minutes left before the maximum parking limit";
//    sendCommand(message);

    if (breakout->sendTextCommand(message) == COMMAND_STATUS_OK) {
      LOG(L_INFO, "Tx-Command [%s]\r\n", message);
    } else {
      LOG(L_INFO, "Tx-Command ERROR\r\n");
    }
  }
  if (counter == 35) {
    char message[] = "Your car has exceeded the time limit! You'll be ticketed.";
//    sendCommand(message);

    if (breakout->sendTextCommand(message) == COMMAND_STATUS_OK) {
      LOG(L_INFO, "Tx-Command [%s]\r\n", message);
    } else {
      LOG(L_INFO, "Tx-Command ERROR\r\n");
    }
  }
  if (distance <= 3.8 && carCheck == 0) {
    //counter++;
  /*SerialUSB.print("Time:  ");
  SerialUSB.print(counter);
  SerialUSB.println(" seconds.");*/
  //SerialUSB.print(distance);
  //SerialUSB.println("[cm]");
    carCheck = 1;
    SerialUSB.println("Bike is now locked!");

  }else if(distance > 3.8 && carCheck == 1 ) {
     // Send message to server how long car been parked
    /*SerialUSB.print("Bike is now unlock");
    SerialUSB.print((counter/3600)%60);
    SerialUSB.print(" hour ");
    SerialUSB.print((counter/60)%60);
    SerialUSB.print(" minute ");
    SerialUSB.print(counter%60);
    SerialUSB.println(" second ");*/
    carCheck = 0;
    SerialUSB.println("Bike is now unlocked!");

  } else{
    //SerialUSB.println("No car here "); // Wait 1 minute
    //SerialUSB.println("*go into Power Saving Mode, wait for one minute before next paging cycle*");
    counter = 0;

  digitalWrite(WIOLTE_D20, LOW);
  }
  breakout->spin();

  delay(INTERVAL);
}
