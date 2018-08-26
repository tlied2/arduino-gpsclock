#include <SoftwareSerial.h>
#include <Wire.h>
#include <Time.h>
#include <Timezone.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GPS.h"
#include <Adafruit_SHT31.h>

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// Timezone rules.  Select yours from the following.  If you need space, comment out the rest.
// Atlantic
TimeChangeRule aDST = { "ADT", Second, Sun, Mar, 2, -180 };  // UTC - 3 hrs.
TimeChangeRule aSTD = { "AST", First, Sun, Nov, 2, -240 };   // UTC - 4 hrs.
// Eastern
TimeChangeRule eDST = { "EDT", Second, Sun, Mar, 2, -240 };  // UTC - 4 hrs.
TimeChangeRule eSTD = { "EST", First, Sun, Nov, 2, -300 };   // UTC - 5 hrs.
// Central
TimeChangeRule cDST = { "CDT", Second, Sun, Mar, 2, -300 };  // UTC - 5 hrs.
TimeChangeRule cSTD = { "CST", First, Sun, Nov, 2, -360 };   // UTC - 6 hrs.
// Mountain
TimeChangeRule mDST = { "MDT", Second, Sun, Mar, 2, -360 };  // UTC - 6 hrs.
TimeChangeRule mSTD = { "MST", First, Sun, Nov, 2, -420 };   // UTC - 7 hrs.
// Arizona - Does not observe daylight time
TimeChangeRule azDST = { "MST", Second, Sun, Mar, 2, -420 };  // UTC - 7 hrs.
TimeChangeRule azSTD = { "MST", First, Sun, Nov, 2, -420 };   // UTC - 7 hrs.
// Pacific
TimeChangeRule pDST = { "PDT", Second, Sun, Mar, 2, -420 };  // UTC - 7 hrs.
TimeChangeRule pSTD = { "PST", First, Sun, Nov, 2, -480 };   // UTC - 8 hrs.
// Alaska
TimeChangeRule akDST = { "AKDT", Second, Sun, Mar, 2, -480 }; // UTC - 8 hrs.
TimeChangeRule akSTD = { "AKST", First, Sun, Nov, 2, -540 };  // UTC - 9 hrs.
// Hawaii-Aleution
TimeChangeRule haDST = { "HADT", Second, Sun, Mar, 2, -540 }; // UTC - 9 hrs.
TimeChangeRule haSTD = { "HAST", First, Sun, Nov, 2, -600 };  // UTC - 10 hrs.
// Hawaii - Does not observe daylight time
TimeChangeRule hiDST = { "HDT", Second, Sun, Mar, 2, -600 };  // UTC - 10 hrs.
TimeChangeRule hiSTD = { "HST", First, Sun, Nov, 2, -600 };   // UTC - 10 hrs.

// Set to Eastern Time Zone
TimeChangeRule myDST = eDST;
TimeChangeRule mySTD = eSTD;
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, used to get TZ abbrev
time_t utc, local;          // Universal and local time in time_t format

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

// Create display and GPS objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();

// GPS breakout/shield will use a
// software serial connection with
// TX = pin 8 and RX = pin 7.
SoftwareSerial gpsSerial(8, 7);
Adafruit_GPS gps(&gpsSerial);

// Temperature and Humidity Sensor
Adafruit_SHT31 sht31 = Adafruit_SHT31();

int displayValue = 8888;
int tmpsec, lastsec = 0;
int temp, humid = 0;

void setup() {
  // Setup function runs once at startup to initialize the display and GPS.

  // Setup Serial port to print debug output.
  Serial.begin(115200);
  Serial.println("Clock starting!");

  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);
  clockDisplay.setBrightness(0);

  sht31.begin();

  // Setup the GPS using a 9600 baud connection (the default for most
  // GPS modules).
  gps.begin(9600);

  // Configure GPS to onlu output minimum data (location, time, fix).
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // Use a 1 hz, once a second, update rate.
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // Turn all segments on if no gps fix
  clockDisplay.print(8888, DEC);
  clockDisplay.writeDigitNum(2, 0);
  clockDisplay.writeDisplay();

  temp = (int)(sht31.readTemperature() * 1.8) + 32.5;
  humid = (int)(sht31.readHumidity() + 0.5);

  enableGPSInterrupt();

  while (!gps.newNMEAreceived()) { }
  while (!gps.parse(gps.lastNMEA())) { }
  setTime(gps.hour, gps.minute, gps.seconds, gps.day, gps.month, gps.year);
}

void loop() {
  // Loop function runs over and over again to implement the clock logic.

  // Check if GPS has new data and parse it.
  if (gps.newNMEAreceived()) {
    //Serial.println("NMEA received");
    if (gps.parse(gps.lastNMEA())) {
      // Set the Arduino software RTC
      setTime(gps.hour, gps.minute, gps.seconds, gps.day, gps.month, gps.year);
      //Serial.println("RTC Updated");
    }
  }

  utc = now();
  local = myTZ.toLocal(utc, &tcr);
  tmpsec = second(local);

  // Display Temp and Rel Humid
  if ((tmpsec >= 10 and tmpsec <= 15) or (tmpsec >= 40 and tmpsec <= 45)) {
    clockDisplay.print((temp * 100) + humid, DEC);
  }
  else {
    // Show the time on the display by turning it into a numeric
    // value, like 3:30 turns into 330, by multiplying the hour by
    // 100 and then adding the minutes.
    if (TIME_24_HOUR) {
      displayValue = hour(local) * 100 + minute(local);
    } else {
      displayValue = hourFormat12(local) * 100 + minute(local);
    }

    // Now print the time value to the display.
    clockDisplay.print(displayValue, DEC);

    if (lastsec != tmpsec) {
      lastsec = tmpsec;
      Serial.print(hour(local));
      Serial.print(":");
      Serial.print(minute(local));
      Serial.print(":");
      Serial.println(second(local));
    }

    // 0 pad for 24 hr
    if (TIME_24_HOUR && hour(local) == 0) {
      clockDisplay.writeDigitNum(1, 0);
      if (minute(local) < 10)
        clockDisplay.writeDigitNum(3, 0);
    }
  }

  // Blink the colon by turning it on every even second and off
  // every odd second.  The modulus operator is very handy here to
  // check if a value is even (modulus 2 equals 0) or odd (modulus 2
  // equals 1).
  clockDisplay.drawColon(tmpsec % 2 == 0);

  // Now push out to the display the new values that were set above.
  clockDisplay.writeDisplay();

  if (tmpsec % 30 == 0) {
    temp = (int)(sht31.readTemperature() * 1.8) + 32.5;
    humid = (int)(sht31.readHumidity() + 0.5);
    Serial.println("Temp/Humid updated");
  }

  // Loop code is finished, it will jump back to the start of the loop
  // function again! Don't add any delays because the parsing needs to
  // happen all the time!
}

SIGNAL(TIMER0_COMPA_vect) {
  // Use a timer interrupt once a millisecond to check for new GPS data.
  // This piggybacks on Arduino's internal clock timer for the millis()
  // function.
  gps.read();
}

void enableGPSInterrupt() {
  // Function to enable the timer interrupt that will parse GPS data.
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function above
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}
