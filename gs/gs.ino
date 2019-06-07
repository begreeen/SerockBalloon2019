#include <CanSatKit.h>

using namespace CanSatKit;


struct radio_frame {
  float latitude;
  float longitude;
  int16_t altitude_msl;
  int16_t temperature;
  float pressure;
  int8_t humidity;
  int8_t satellites;
} __attribute__((packed));


static_assert(sizeof(radio_frame) == 18, "align?");



// set radio receiver parameters - see comments below
// remember to set the same radio parameters in
// transmitter and receiver boards!
Radio radio(Pins::Radio::ChipSelect,
            Pins::Radio::DIO0,
            433.0,                  // frequency in MHz
            Bandwidth_125000_Hz,    // bandwidth - check with CanSat regulations to set allowed value
            SpreadingFactor_11,      // see provided presentations to determine which setting is the best
            CodingRate_4_8);        // see provided presentations to determine which setting is the best

void setup() {
  SerialUSB.begin(115200);

  // start radio module  
  radio.begin();
}

void loop() {
  uint8_t data[256];
  
  uint8_t length;
  radio.receive(data, length);
  
  if (length != sizeof(radio_frame)) {
    SerialUSB.println("Incorrect frame!");
    return;
  }
  radio_frame* f = reinterpret_cast<radio_frame*>(data);


  static uint32_t counter = 0;
  counter++;  
  SerialUSB.print(millis());
  SerialUSB.print(';');
  SerialUSB.print(counter);
  SerialUSB.print(';');
  SerialUSB.print(radio.get_rssi_last());
  SerialUSB.print(';');
  SerialUSB.print(f->satellites);
  SerialUSB.print(';');
  SerialUSB.print(f->longitude);
  SerialUSB.print(';');
  SerialUSB.print(f->latitude);
  SerialUSB.print(';');
  SerialUSB.print(f->altitude_msl);
  SerialUSB.print(';');
  SerialUSB.print(f->temperature);
  SerialUSB.print(';');
  SerialUSB.print(f->pressure);
  SerialUSB.print(';');
  SerialUSB.print(f->humidity);  
  SerialUSB.println();
}
