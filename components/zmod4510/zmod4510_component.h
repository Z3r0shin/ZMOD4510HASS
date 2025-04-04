#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include <cstring>  // For memcpy

// Wrap Renesas C headers in extern "C" to avoid C++ name mangling.
extern "C" {
  #include "zmod4xxx_types.h"
  #include "hal.h"
  #include "zmod4xxx_hal.h"
  #include "zmod4xxx.h"
  #include "no2_o3.h"
  #include "zmod4510_config_no2_o3.h"
  #include "zmod4xxx_cleaning.h"
}

namespace zmod4510 {

class ZMOD4510 : public esphome::PollingComponent, public esphome::i2c::I2CDevice {
 public:
  ZMOD4510();

  void set_i2c_address(uint8_t address);
  void set_no2_sensor(esphome::sensor::Sensor *sensor);
  void set_o3_sensor(esphome::sensor::Sensor *sensor);
  void set_aqi_sensor(esphome::sensor::Sensor *sensor);

  void setup() override;
  void update() override;

 protected:
  uint8_t i2c_address_;
  esphome::sensor::Sensor *no2_sensor_{nullptr};
  esphome::sensor::Sensor *o3_sensor_{nullptr};
  esphome::sensor::Sensor *aqi_sensor_{nullptr};

  // Renesas device structure and algorithm state.
  zmod4xxx_dev_t dev_;
  no2_o3_handle_t algo_handle_;

  // Buffers for configuration and ADC measurement data.
  uint8_t config_[6];
  uint8_t prod_data_[ZMOD4510_PROD_DATA_LEN];  // From config header.
  uint8_t adc_buffer_[32];                     // Based on ADC data length.
};

}  // namespace zmod4510
