/* docs
 *  https://www.sparkfun.com/datasheets/GPS/NMEA%20Reference%20Manual-Rev2.1-Dec07.pdf
 *  https://origingps.com/wp-content/uploads/2018/08/Spider-and-Hornet-Low-Power-Operating-Mode-Application-Note-SiRFStarIV.pdf
 *  https://y1cj3stn5fbwhv73k0ipk1eg-wpengine.netdna-ssl.com/wp-content/uploads/2017/09/Telit_Using_SiRF_STAR_IV_with_an_external_Host_Application_Note_r0.pdf
 *  http://www.elgps.com/public_ftp/Documentos/SIRF_Protocol.pdf
 */

#include <CanSatKit.h>
#include <TinyGPS++.h>
using namespace CanSatKit;


constexpr auto LED = 13;

constexpr auto gps_reset = A1;
constexpr auto gps_on_off = A2;
constexpr auto gps_wakeup = A4;
constexpr auto gps_pps = 8;

Radio radio(Pins::Radio::ChipSelect,
            Pins::Radio::DIO0,
            433.0,
            Bandwidth_125000_Hz,
            SpreadingFactor_9,
            CodingRate_4_8);

Frame frame;

TinyGPSPlus gps;

uint8_t calculate_checksum(const char * cmd) {
  auto len = strlen(cmd);
  len--; // without last byte (*)
  uint8_t crc = 0;
  for (int i = 1; i < len; ++i) {
    crc ^= cmd[i];
  }
  return crc;
}

// example: $PSRF100,0,115200,8,1,0*
void send_nmea_command(const char * cmd) {
   Serial.print(cmd);
   SerialUSB.print(cmd);
   
   uint8_t checksum = calculate_checksum(cmd);
   char tmp[5];
   sprintf(tmp, "%.2X\r\n", checksum);
   
   Serial.print(tmp);
   SerialUSB.print(tmp);
}

void switch_to_osp() {
  send_nmea_command("$PSRF100,0,4800,8,1,0*");
}

void set_power_mode() {
  constexpr static uint8_t table[] = 
        {0xA0, 0xA2,
         0x00, 0x10,
         0xDA,
         0x03,
         0x00, 0x64,
         0x00, 0x00, 0x01, 0xF4,
         0x00, 0x00, 0x03, 0xE8,
         0x00, 0x6d, 0xdd, 0x00,
         0x04, 0x6B,
         0xB0, 0xB3};
  Serial.write(table, sizeof(table));
}

void switch_to_nmea() {
  while (digitalRead(gps_wakeup) == LOW) {}
  constexpr static uint8_t table_nmea[] = 
        {0xA0, 0xA2, 0x00, 0x18,
         0x81, 0x02,
         0x05, 0x01,  // GGA
         0x00, 0x01,  // GLL
         0x00, 0x01,  // GSA
         0x00, 0x01,  // GSV
         0x00, 0x01,  // RMC
         0x00, 0x01,  // VTG
         0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
         0x12, 0xC0,
         0x01, 0x64,  // checksum (sum all the bytes except first four)
         0xB0, 0xB3};
  Serial.write(table_nmea, sizeof(table_nmea));
}

void setup_power_mode() {
  SerialUSB.println("Setting GPS power mode!");
  switch_to_osp();
  delay(100);
  set_power_mode();
  delay(1000);
  switch_to_nmea();
  delay(2000);
  while (Serial.available()) {
    volatile char d = Serial.read();
  }
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(gps_reset, OUTPUT);
  pinMode(gps_on_off, OUTPUT);
  pinMode(gps_wakeup, INPUT);
  pinMode(gps_pps, INPUT);

  Serial.begin(4800);
  SerialUSB.begin(9600);
  while (!SerialUSB);

  SerialUSB.println("start!");

  digitalWrite(gps_on_off, LOW);

  digitalWrite(gps_reset, LOW);
  delay(100);
  digitalWrite(gps_reset, HIGH);

  SerialUSB.println("wait for low!");
  while (digitalRead(gps_wakeup) != LOW) {}

  SerialUSB.println("wait for high!");
  while (digitalRead(gps_wakeup) != HIGH) {}

  SerialUSB.println("wait for low!");
  while (digitalRead(gps_wakeup) != LOW) {}



  SerialUSB.println("gps woke up!");
  SerialUSB.println("sending on_off pulse!");
  delay(100);
  digitalWrite(gps_on_off, HIGH);
  delay(100);
  digitalWrite(gps_on_off, LOW);


  SerialUSB.println("wait for high!");
  while (digitalRead(gps_wakeup) != HIGH) {}


  SerialUSB.println("GPS on!");
  
  radio.begin();
  radio.disable_debug();

  frame.print("testtesttesttesttesttesttesttesttest");
  digitalWrite(LED, HIGH);

  SerialUSB.print((int)digitalRead(gps_pps));
}

//void loop() {
//  // put your main code here, to run repeatedly:
//  while(Serial.available() > 0) {
////    volatile auto _ = (char)Serial.read();
//    char data = Serial.read();
//    gps.encode(data);
//    SerialUSB.print(data);
//  }
//  while(SerialUSB.available() > 0) {
//    Serial.print((char)SerialUSB.read());
//  }
//
//  static auto last_time = millis();
//  if (millis() > last_time + 1000) {
//    //SerialUSB.print((int)digitalRead(gps_pps));
//    static bool state = false;
//    state = !state;
//    digitalWrite(LED, state);
//
//    //radio.transmit(frame);
//
//    last_time = millis();
//  }
//}


static auto last_sentencesWithFix = 0;
static auto last_tx_time = 0;

bool new_data() {
  return last_sentencesWithFix != gps.sentencesWithFix();
}

bool timeout() {
  return millis() > last_tx_time + 6000;
}

void loop()
{
  
  if (new_data() || timeout()) {
    last_sentencesWithFix = gps.sentencesWithFix();
    last_tx_time = millis();
    
    printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
    printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
    printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
    printDateTime(gps.date, gps.time);
    printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
      
    printInt(gps.charsProcessed(), true, 6);
    printInt(gps.sentencesWithFix(), true, 10);
    printInt(gps.failedChecksum(), true, 9);
    SerialUSB.println();
  
    static bool gps_setted_up = false;
    if (gps.location.isValid() && !gps_setted_up) {
      gps_setted_up = true;
      setup_power_mode();
    }

    static bool state = false;
    state = !state;
    digitalWrite(LED, state);

    radio.transmit(frame);
  }
  smartDelay(0);
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial.available()) {
      char d = Serial.read();
      //SerialUSB.print(d);
      gps.encode(d);
    }
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      SerialUSB.print('*');
    SerialUSB.print(' ');
  }
  else
  {
    SerialUSB.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      SerialUSB.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  SerialUSB.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    SerialUSB.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    SerialUSB.print(sz);
  }
  
  if (!t.isValid())
  {
    SerialUSB.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    SerialUSB.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    SerialUSB.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}
