// LIBRARIES ============================================================================================================================================================
#include "MegaSquirt3.h"
#include "BMI088.h"
#include <Wire.h>
#include <SD.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "Selector.h"
#include "Display.h"

// SETTINGS =============================================================================================================================================================
bool is_dashboard = true;
const int WRITE_FREQ = 10; // ms
const int CHECK_ENGINE = 27;
const int DATA_SWITCH = 28;
const int STATUS_A = 12;
const int STATUS_B = 11;
const int STATUS_C = 10;
const int STATUS_D = 9;
const int SELECTOR_PINS[] = {35, 36, 37, 38, 39, 40};

// INSTANCES ============================================================================================================================================================
Bmi088Accel accel(Wire,0x18);
Bmi088Gyro gyro(Wire,0x68);
SFE_UBLOX_GNSS myGNSS;
Selector selector(SELECTOR_PINS);
MegaSquirt3 ecu;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can;
Adafruit_7segment matrix1 = Adafruit_7segment();
Adafruit_7segment matrix2 = Adafruit_7segment();

File file;
ROW tosend;

// GLOBAL VARS ==========================================================================================================================================================
int re;
uint32_t lastwrite = 0;
int mode = 0;
int mode_bounces = 0;
bool offset;

// MAIN INTERRUPT =======================================================================================================================================================
void handler(const CAN_message_t &msg) {
  offset = !offset;
  digitalWrite(STATUS_D, offset ? HIGH : LOW);
  //  Serial.println("message");
   // log directly wired sensors:
   if (!is_dashboard) {
     // log accelerometer
     accel.readSensor();
     tosend.ax = accel.getAccelX_mss();
     tosend.ay = accel.getAccelY_mss();
     tosend.az = accel.getAccelZ_mss();
     tosend.accel_millis = millis();
 
     // log gyro
     gyro.readSensor();
     tosend.imu_x = gyro.getGyroX_rads();
     tosend.imu_y = gyro.getGyroY_rads();
     tosend.imu_z = gyro.getGyroZ_rads();
     tosend.imu_millis = millis();
 
     // log GPS
     myGNSS.checkUblox();
     tosend.hour = myGNSS.getHour();
     tosend.minute = myGNSS.getMinute();
     tosend.second = myGNSS.getSecond();
     tosend.lat = myGNSS.getLatitude();
     tosend.lon = myGNSS.getLongitude();
     tosend.elev = myGNSS.getAltitude();
     tosend.ground_speed = myGNSS.getGroundSpeed();	
     tosend.gps_millis = millis();
   }

    // ecu.data.rpm = 90;

    // Serial.println("This is the rpm");
    // Serial.println(ecu.data.rpm);

  if (ecu.decode(msg)) {
      // read ecu data
     // =============
    //  if (is_dashboard) {
    //   int a = floor((ecu.data.tps / 75.0) * 14);
    //   for (int j = 0; j < a; j++) {  
    //     digitalWrite(ALL_LEDS[j], HIGH);
    //     Serial.print(j);
    //     Serial.print(" ");
    //   }
    //   Serial.print("^");
    //   for (int j = a; j < 14; j++) {  
    //     digitalWrite(ALL_LEDS[j], LOW);
    //     Serial.print(j);
    //     Serial.print(" ");
    //   }
    //   Serial.println();
    //   //  set_rpm((9500)*(ecu.data.tps / 75.0));
    //  }
     set_rpm(ecu.data.rpm);
     tosend.rpm = ecu.data.rpm;
     tosend.time = ecu.data.seconds;
     tosend.afr = ecu.data.AFR1;
     tosend.spark_advance = ecu.data.adv_deg;
     tosend.baro = ecu.data.baro;
     tosend.map = ecu.data.map;
     tosend.mat = ecu.data.mat;
     tosend.clnt_temp = ecu.data.clt;
     tosend.tps = ecu.data.tps;
     tosend.batt = ecu.data.batt;
     tosend.oil_press = ecu.data.sensors1;
     tosend.syncloss_count = ecu.data.synccnt;
     tosend.syncloss_code = ecu.data.syncreason;
     tosend.ltcl_timing = ecu.data.launch_timing;
     tosend.ve1 = ecu.data.ve1;
     tosend.ve2 = ecu.data.ve2;
     tosend.egt = ecu.data.egt1;
     tosend.maf = ecu.data.MAF;
     tosend.in_temp = ecu.data.airtemp;
     tosend.ecu_millis = millis();
    // check engine
    bool engine_bad = true;
    digitalWrite(CHECK_ENGINE, engine_bad ? HIGH : LOW);
    // show displays
    Serial.print(ecu.data.syncreason);
    Serial.print(", ");
    Serial.println(ecu.data.synccnt);
 
  } else if (msg.id == 935444) {
    double a = ((msg.buf[0]) + (msg.buf[1]<<8))/5024.0;
    double b = ((msg.buf[2]) + (msg.buf[3]<<8))/5024.0;
    double c = ((msg.buf[4]) + (msg.buf[5]<<8))/5024.0;
    double d = ((msg.buf[6]) + (msg.buf[7]<<8))/5024.0;
    // Serial.print(a);Serial.print("\t");
    // Serial.print(b);Serial.print("\t");
    // Serial.print(c);Serial.print("\t");
    // Serial.print(d);Serial.print("\t batt: ");
    // Serial.println(ecu.data.batt);
  }
  // write to SD card if time in time
  if (!is_dashboard && millis() > lastwrite + WRITE_FREQ) {
    tosend.write_millis = millis();
    file.write((byte*) &tosend, sizeof(tosend));
    // Serial.println("WROTE ##############################################################");
    lastwrite = millis();
  }

  String MODE_NAMES[] = {"1    OFF", "2   SHOW", "3   NORM", "4    RPM", "5   COOL", "6    OIL", "7   BRAK"};

  if (is_dashboard) {
    int newmode = selector.get();
    Serial.print("newmode: ");
    Serial.println(newmode);
    if (newmode != mode) {
      mode_bounces++;
      if (mode_bounces > 6) {
        mode = newmode;
        mode_bounces = 0;
        displayText(MODE_NAMES[mode], matrix1, matrix2);
        delay(3000);
      }
    }


    switch (mode) {
      case 0:
        // off
        break;
      case 1:
        // standby
        displayText("GO COUGS", matrix1, matrix2);
        // lightSequence();
        break;
      case 2:
        // standard running
        displayText("std mode", matrix1, matrix2);
        break;
      case 3:
        // rpm only
        displayInt(ecu.data.rpm, matrix1, matrix2);
        break;
      case 4:
        // coolant temp
        displayInt(ecu.data.clt, matrix1, matrix2);
        break;
      case 5:
        // oil pressure
        displayInt(ecu.data.sensors1, matrix1, matrix2);
        break;
      case 6:
        // brake pressure
        displayText("--------", matrix1, matrix2);
        break;
      default:
        break;
    }
  }
}

// SETUP ================================================================================================================================================================
void setup() {
  Serial.begin(9600);
  matrix2.begin(0x70, &Wire);
  matrix1.begin(0x70, &Wire1);

  if (is_dashboard) {
    // lightSequence();

    for (int i = 0; i < 14; i++) {
      pinMode(ALL_LEDS[i], OUTPUT);
    }
    pinMode(CHECK_ENGINE, OUTPUT);
    selector.initialize();
  } else {
    pinMode(STATUS_A, OUTPUT);
    digitalWrite(STATUS_A, LOW);
    pinMode(STATUS_B, OUTPUT);
    digitalWrite(STATUS_B, LOW);
    pinMode(STATUS_C, OUTPUT);
    digitalWrite(STATUS_C, LOW);
    pinMode(STATUS_D, OUTPUT);
    digitalWrite(STATUS_D, LOW);


    if (!SD.begin(BUILTIN_SDCARD)) {
      Serial.println(F("SD CARD FAILED, OR NOT PRESENT!"));
      while(1);
    }
    digitalWrite(STATUS_A, HIGH);

    Serial.println("@@@@@@@@@@@@@@@@@@");
    accel.begin();
    gyro.begin();

    pinMode(24, INPUT_PULLUP);
    pinMode(25, INPUT_PULLUP);

    Wire2.begin();
    if (myGNSS.begin(Wire2) == false) { //Connect to the u-blox module using Wire port {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
      // while (1);
    }
    digitalWrite(STATUS_B, HIGH);

    myGNSS.checkUblox();
    String filename = String(myGNSS.getYear()) + "-" + String(myGNSS.getMonth()) + "-" + String(myGNSS.getDay()) +\
                    "-" + String(myGNSS.getHour()) + ":" + String(myGNSS.getMinute()) + ":" + String(myGNSS.getSecond() + ".bin");
    // file = SD.open(filename.c_str(), FILE_WRITE);
    file = SD.open("coletest2.txt", FILE_WRITE);
    file.write((byte*) &tosend, sizeof(tosend));
    // file.write("hello\n", 6);

    // file.close();
    Serial.println(filename);
  }

  Can.begin();
  Can.setBaudRate(500000); //set to 500000 for normal Megasquirt usage - need to change Megasquirt firmware to change MS CAN baud rate
  Can.setMaxMB(16); //sets maximum number of mailboxes for FlexCAN_T4 usage
  Can.enableFIFO();
  Can.enableFIFOInterrupt();
  Can.mailboxStatus();
  Can.onReceive(handler); //when a CAN message is received, runs the canMShandler function
}
void loop(){
  if (!is_dashboard) {
    digitalWrite(STATUS_C, HIGH);
  }
  CAN_message_t a;
  // handler(a);
  // matrix2.print(selector.get(), DEC);
  // matrix2.writeDisplay();
  // delay(100);
  // Serial.println(millis());
  Can.events();
}