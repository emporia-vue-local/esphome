#pragma once

///
/// Mirror the public interface of MqttIdfClient using esp-idf
///

#ifdef USE_ESP_IDF
#include <string>
#include <map>
#include <mqtt_client.h>
#include "esphome/components/network/ip_address.h"
#include "esphome/core/helpers.h"
#include "mqtt_client_base.h"

namespace esphome {
namespace mqtt {

/// Start Required for compability with MqttIdfClient inteface as used by esphome
struct MqttIdfClientMessageProperties {
  uint8_t qos;
  bool dup;
  bool retain;
};

enum class MqttIdfClientDisconnectReason : int8_t {
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  ESP8266_NOT_ENOUGH_SPACE = 6,
  TLS_BAD_FINGERPRINT = 7
};

class MqttIdfClient : public MqttClientBase {
 public:
  static const size_t MQTT_BUFFER_SIZE = 4096;

  void connect() {
    if (!is_initalized_)
      if (initialize_())
        esp_mqtt_client_start(handler_.get());
  }
  void disconnect(bool) {
    if (is_initalized_)
      esp_mqtt_client_disconnect(handler_.get());
  }

  // AsyncMqtt uses 0 for an error and 1 for QoS 0 message ids
  // idf ues -1 for error and 0 for QoS 0
  inline uint16_t to_async_status(int status) {
    if (status == -1)
      status = 0;
    if (status == 0)
      status = 1;
    return status;
  }

  uint16_t subscribe(const char *topic, uint8_t qos) {
    return to_async_status(esp_mqtt_client_subscribe(handler_.get(), topic, qos));
  }
  uint16_t unsubscribe(const char *topic) {
    return to_async_status(esp_mqtt_client_unsubscribe(handler_.get(), topic));
  }
  uint16_t publish(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr, size_t length = 0,
                   bool unused_dup = false, uint16_t unused_message_id = 0) {
#if defined IDF_MQTT_USE_ENQUEUE
    // use the non-blocking version
    // it can delay sending a couple of seconds but won't block
    auto status = to_async_status(esp_mqtt_client_enqueue(handler_.get(), topic, payload, length, qos, retain, true));
#else
    // might block for several seconds, either due to network timeout (10s)
    // or if publishing payloads longer than internal buffer (due to message fragmentation)
    auto status = to_async_status(esp_mqtt_client_publish(handler_.get(), topic, payload, length, qos, retain));
#endif
    return status;
  }
  /// End - Required for compability
  void set_ca_certificate(const std::string &cert) { ca_certificate_ = cert; }
  void set_skip_cert_cn_check(bool skip_check) { skip_cert_cn_check_ = skip_check; }

 protected:
  bool initialize_();
  void mqtt_event_handler_(esp_event_base_t base, int32_t event_id, void *event_data);
  static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

  struct MqttClientDeleter {
    void operator()(esp_mqtt_client *client_handler) { esp_mqtt_client_destroy(client_handler); }
  };
  using ClientHandler_ = std::unique_ptr<esp_mqtt_client, MqttClientDeleter>;
  ClientHandler_ handler_;

  bool is_connected_{false};
  bool is_initalized_{false};
  esp_mqtt_client_config_t mqtt_cfg_{};
  optional<std::string> ca_certificate_;
  bool skip_cert_cn_check_{false};
};

}  // namespace mqtt
}  // namespace esphome
#endif
