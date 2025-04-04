#include <cstdarg>
#include <cstdio>

namespace esphome {
#ifndef esp_log_printf_
static inline void esp_log_printf_(int level, const char *tag, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}
#endif
}  // namespace esphome

#include "zmod4510_component.h"
#include "esphome/core/log.h"
#include <Arduino.h>
#include <cstring>

namespace zmod4510 {

static const char *TAG = "zmod4510";

ZMOD4510::ZMOD4510() : PollingComponent(60000) {  // Default update interval: 60s
  this->i2c_address_ = 0x33;
}

void ZMOD4510::set_i2c_address(uint8_t address) {
  this->i2c_address_ = address;
}

void ZMOD4510::set_no2_sensor(esphome::sensor::Sensor *sensor) {
  this->no2_sensor_ = sensor;
}

void ZMOD4510::set_o3_sensor(esphome::sensor::Sensor *sensor) {
  this->o3_sensor_ = sensor;
}

void ZMOD4510::set_aqi_sensor(esphome::sensor::Sensor *sensor) {
  this->aqi_sensor_ = sensor;
}

void ZMOD4510::setup() {
  ESP_LOGI(TAG, "Setting up ZMOD4510 sensor");

  // Configure the device structure.
  this->dev_.i2c_addr = this->i2c_address_;
  memcpy(this->dev_.config, this->config_, sizeof(this->config_));
  this->dev_.prod_data = this->prod_data_;

  // Point to the pre-defined configuration arrays.
  this->dev_.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
  this->dev_.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];

  int ret = zmod4xxx_init_sensor(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_init_sensor failed with code %d", ret);
  }
  ret = zmod4xxx_init_measurement(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_init_measurement failed with code %d", ret);
  }
  ret = zmod4xxx_prepare_sensor(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_prepare_sensor failed with code %d", ret);
  }
  ret = init_no2_o3(&this->algo_handle_);
  if (ret != NO2_O3_OK) {
    ESP_LOGE(TAG, "init_no2_o3 failed with code %d", ret);
  }
}

void ZMOD4510::update() {
  ESP_LOGD(TAG, "Starting sensor update");

  int ret = zmod4xxx_start_measurement(&this->dev_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_start_measurement failed with code %d", ret);
    return;
  }

  delay(6000);  // Wait for the measurement to complete (6000 ms for NO₂/O₃ mode).

  ret = zmod4xxx_read_adc_result(&this->dev_, this->adc_buffer_);
  if (ret != ZMOD4XXX_OK) {
    ESP_LOGE(TAG, "zmod4xxx_read_adc_result failed with code %d", ret);
    return;
  }

  // Set up algorithm input with default ambient values.
  no2_o3_inputs_t algo_input;
  algo_input.adc_result = this->adc_buffer_;
  algo_input.humidity_pct = 50.0f;      // Default: 50% RH
  algo_input.temperature_degc = 25.0f;  // Default: 25°C

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
           algo_results.NO2_conc_ppb, algo_results.O3_conc_ppb, algo_results.FAST_AQI);

  if (this->no2_sensor_ != nullptr) {
    this->no2_sensor_->publish_state(algo_results.NO2_conc_ppb);
  }
  if (this->o3_sensor_ != nullptr) {
    this->o3_sensor_->publish_state(algo_results.O3_conc_ppb);
  }
  if (this->aqi_sensor_ != nullptr) {
    this->aqi_sensor_->publish_state(static_cast<float>(algo_results.FAST_AQI));
  }
}

}  // namespace zmod4510
