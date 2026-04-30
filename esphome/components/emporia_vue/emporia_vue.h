#pragma once

#ifdef USE_ESP32

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace emporia_vue {

struct __attribute__((__packed__)) ReadingPowerEntry {
  int32_t phase_black;
  int32_t phase_red;
  int32_t phase_blue;
};

struct __attribute__((__packed__)) SensorReading {
  bool is_unread;
  uint8_t checksum;
  uint8_t unknown;
  uint8_t sequence_num;

  ReadingPowerEntry power[19];

  uint16_t voltage[3];
  uint16_t frequency;
  uint16_t degrees[2];

  uint16_t current[19];

  uint16_t end;
};

class PhaseConfig;
class VirtualPhaseConfig;
class PhaseTargetConfig;
class CTClampConfig;

class EmporiaVueComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void dump_config() override;

  float get_setup_priority() const override { return esphome::setup_priority::HARDWARE; }

  void set_phases(std::vector<PhaseConfig *> phases) { this->phases_ = std::move(phases); }
  void set_virtual_phases(std::vector<VirtualPhaseConfig *> virtual_phases) {
    this->virtual_phases_ = std::move(virtual_phases);
  }
  void set_ct_clamps(std::vector<CTClampConfig *> ct_clamps) { this->ct_clamps_ = std::move(ct_clamps); }

  void update() override;

  void add_on_update_callback(std::function<void()> &&callback);

 protected:
  uint8_t last_sequence_num_ = 0;
  std::vector<PhaseConfig *> phases_;
  std::vector<VirtualPhaseConfig *> virtual_phases_;
  std::vector<CTClampConfig *> ct_clamps_;
  CallbackManager<void()> callback_;
};

enum PhaseInputWire : uint8_t {
  BLACK = 0,
  RED = 1,
  BLUE = 2,
};

class PhaseTargetConfig {
 public:
  virtual ~PhaseTargetConfig() = default;
  virtual int32_t extract_power_for_phase(const ReadingPowerEntry &power_entry) const = 0;
  virtual float get_calibrated_power(const ReadingPowerEntry &power_entry, float correction_factor) const = 0;
  virtual bool is_virtual() const = 0;
};

class PhaseConfig : public PhaseTargetConfig {
 public:
  void set_input_wire(PhaseInputWire input_wire) { this->input_wire_ = input_wire; }
  PhaseInputWire get_input_wire() const { return this->input_wire_; }
  void set_calibration(float calibration) { this->calibration_ = calibration; }
  float get_calibration() const { return this->calibration_; }
  void set_voltage_sensor(sensor::Sensor *voltage_sensor) { this->voltage_sensor_ = voltage_sensor; }
  sensor::Sensor *get_voltage_sensor() const { return this->voltage_sensor_; }
  void set_frequency_sensor(sensor::Sensor *frequency_sensor) { this->frequency_sensor_ = frequency_sensor; }
  sensor::Sensor *get_frequency_sensor() const { return this->frequency_sensor_; }
  void set_phase_angle_sensor(sensor::Sensor *phase_angle_sensor) { this->phase_angle_sensor_ = phase_angle_sensor; }
  sensor::Sensor *get_phase_angle_sensor() const { return this->phase_angle_sensor_; }

  void update_from_reading(const SensorReading &sensor_reading);
  float extract_calibrated_voltage(const SensorReading &sensor_reading) const;

  int32_t extract_power_for_phase(const ReadingPowerEntry &power_entry) const override;
  float get_calibrated_power(const ReadingPowerEntry &power_entry, float correction_factor) const override;
  bool is_virtual() const override { return false; }

 protected:
  PhaseInputWire input_wire_;
  float calibration_;
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *frequency_sensor_{nullptr};
  sensor::Sensor *phase_angle_sensor_{nullptr};
};

class VirtualPhaseConfig : public PhaseTargetConfig {
 public:
  void set_phase_a(PhaseConfig *phase_a) { this->phase_a_ = phase_a; }
  const PhaseConfig *get_phase_a() const { return this->phase_a_; }
  void set_phase_b(PhaseConfig *phase_b) { this->phase_b_ = phase_b; }
  const PhaseConfig *get_phase_b() const { return this->phase_b_; }
  void set_voltage_sensor(sensor::Sensor *voltage_sensor) { this->voltage_sensor_ = voltage_sensor; }
  sensor::Sensor *get_voltage_sensor() const { return this->voltage_sensor_; }

  void update_from_reading(const SensorReading &sensor_reading);

  int32_t extract_power_for_phase(const ReadingPowerEntry &power_entry) const override;
  float get_calibrated_power(const ReadingPowerEntry &power_entry, float correction_factor) const override;
  bool is_virtual() const override { return true; }

 protected:
  PhaseConfig *phase_a_{nullptr};
  PhaseConfig *phase_b_{nullptr};
  sensor::Sensor *voltage_sensor_{nullptr};
};

enum CTInputPort : uint8_t {
#if defined(EMPORIA_VUE_VARIANT_VUE3)
  A = 2,
  B = 1,
  C = 0,
#else
  A = 0,
  B = 1,
  C = 2,
#endif
  ONE = 3,
  TWO = 4,
  THREE = 5,
  FOUR = 6,
  FIVE = 7,
  SIX = 8,
  SEVEN = 9,
  EIGHT = 10,
  NINE = 11,
  TEN = 12,
  ELEVEN = 13,
  TWELVE = 14,
  THIRTEEN = 15,
  FOURTEEN = 16,
  FIFTEEN = 17,
  SIXTEEN = 18,
};

class CTClampConfig : public sensor::Sensor {
 public:
  void set_phase(PhaseTargetConfig *phase) { this->phase_ = phase; };
  const PhaseTargetConfig *get_phase() const { return this->phase_; }
  void set_input_port(CTInputPort input_port) { this->input_port_ = input_port; };
  CTInputPort get_input_port() const { return this->input_port_; }
  void set_power_sensor(sensor::Sensor *power_sensor) { this->power_sensor_ = power_sensor; }
  sensor::Sensor *get_power_sensor() const { return this->power_sensor_; }
  void set_current_sensor(sensor::Sensor *current_sensor) { this->current_sensor_ = current_sensor; }
  sensor::Sensor *get_current_sensor() const { return this->current_sensor_; }

  void update_from_reading(const SensorReading &sensor_reading);
  float get_correction_factor() const;

 protected:
  PhaseTargetConfig *phase_;
  CTInputPort input_port_;
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
};

}  // namespace emporia_vue
}  // namespace esphome

#endif  // ifdef USE_ESP32
