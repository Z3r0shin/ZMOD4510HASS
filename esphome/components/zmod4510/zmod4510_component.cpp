#include "zmod4510_component.h"
#include "esphome/core/log.h"
#include <Arduino.h>

namespace zmod4510 {

static const char *TAG = "zmod4510";

ZMOD4510::ZMOD4510() : PollingComponent(60000) {  // Default update interval: 60 seconds
  this->i2c_address_ = 0x33;  // Default I2C address from the config
}

void ZMOD4510::set_i2c_address(uint8_t address) {
  this->i2c_address_ = address;
}

void ZMOD4510::set_no2_sensor(sensor::Sensor *sensor) {
  this->no2_sensor_ = sensor;
}

void ZMOD4510::set_o3_sensor(sensor::Sensor *sensor) {
  this->o3_sensor_ = sensor;
}

void ZMOD4510::set_aqi_sensor(sensor::Sensor *sensor) {
  this->aqi_sensor_ = sensor;
}

void ZMOD4510::setup() {
  ESP_LOGI(TAG, "Setting up ZMOD4510 sensor");

  // Initialize the device structure.
  this->dev_.i2c_addr = this->i2c_address_;
  this->dev_.config = this->config_;
  this->dev_.prod_data = this->prod_data_;

  // Point to the pre-defined configuration arrays from zmod4510_config_no2_o3.h.
  this->dev_.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
  this->dev_.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];

  // Initialize the sensor.
  int ret = zmod4xxx_init_sensor(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_init_sensor failed with code %d", ret);
  }
  ret = zmod4xxx_init_measurement(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_init_measurement failed with code %d", ret);
  }

  // Prepare the sensor for measurements.
  ret = zmod4xxx_prepare_sensor(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_prepare_sensor failed with code %d", ret);
  }

  // Initialize the NO2/O3 algorithm.
  ret = init_no2_o3(&this->algo_handle_);
  if (ret != NO2_O3_OK) {
    ESP_LOGE(TAG, "init_no2_o3 failed with code %d", ret);
  }
}

void ZMOD4510::update() {
  ESP_LOGD(TAG, "Starting sensor update");

  // Start a new measurement.
  int ret = zmod4xxx_start_measurement(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_start_measurement failed with code %d", ret);
    return;
  }

  // Wait for the sensor to complete its measurement.
  // For NO2 O3 mode, the sample time is defined as 6000 ms.
  delay(6000);

  // Read the ADC result into adc_buffer_.
  ret = zmod4xxx_read_adc_result(&this->dev_, this->adc_buffer_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_read_adc_result failed with code %d", ret);
    return;
  }

  // Prepare the algorithm input structure.
  no2_o3_inputs_t algo_input;
  algo_input.adc_result = this->adc_buffer_;
  // Use default ambient values (could be replaced with readings from an external sensor)
  algo_input.humidity_pct = 50.0f;      // 50% relative humidity
  algo_input.temperature_degc = 25.0f;    // 25 °C ambient temperature

  // Create a results structure to receive outputs.
  no2_o3_results_t algo_results;
  ret = calc_no2_o3(&this->algo_handle_, &this->dev_, &algo_input, &algo_results);
  if (ret != NO2_O3_OK) {
    if (ret == NO2_O3_STABILIZATION) {
      ESP_LOGW(TAG, "Sensor in stabilization phase; ignoring results");
    } else {
      ESP_LOGE(TAG, "calc_no2_o3 failed with code %d", ret);
    }
    return;
  }

  ESP_LOGD(TAG, "Algorithm results: NO2: %.2f ppb, O3: %.2f ppb, FAST AQI: %d",
           algo_results.no2_1min_ppb, algo_results.o3_1min_ppb, algo_results.FAST_AQI);

  // Publish sensor values if the corresponding sensor objects have been configured.
  if (this->no2_sensor_ != nullptr) {
    this->no2_sensor_->publish_state(algo_results.no2_1min_ppb);
  }
  if (this->o3_sensor_ != nullptr) {
    this->o3_sensor_->publish_state(algo_results.o3_1min_ppb);
  }
  if (this->aqi_sensor_ != nullptr) {
    // Publish the FAST AQI value (converted to float) as the air quality index.
    this->aqi_sensor_->publish_state(static_cast<float>(algo_results.FAST_AQI));
  }
}

}  // namespace zmod4510
