#include <Wire.h>
#include <Arduino.h>

void check_connected_dev(){
  for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);

        Wire.endTransmission();
    }
}

float get_body_temp(){
  Wire.beginTransmission(0x48);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0){
    return 0.0;
  }
  int bytes = Wire.requestFrom(0x48, 2);
  if (bytes != 2) {
        return NAN;
    }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  int16_t raw = (msb << 8) | lsb;
  raw &= 0x3FFF;
  float temp = raw * 0.00390625;
  return temp;

}