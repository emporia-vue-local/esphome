#pragma once

///
/// Mirror the public interface of AsyncMqttClient using esp-idf
///

#ifdef USE_ESP_IDF
#include <string>
#include <map>
#include <mqtt_client.h>
#include "esphome/components/network/ip_address.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace mqtt {

/// Start Required for compability with AsyncMqttClient inteface as used by esphome
struct AsyncMqttClientMessageProperties {
  uint8_t qos;
  bool dup;
  bool retain;
};

enum class AsyncMqttClientDisconnectReason : int8_t {
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  ESP8266_NOT_ENOUGH_SPACE = 6,
  TLS_BAD_FINGERPRINT = 7
};

class AsyncMqttClient {
 public:
  using OnConnectUserCallback = std::function<void(bool session_present)>;
  using OnDisconnectUserCallback = std::function<void(AsyncMqttClientDisconnectReason reason)>;
  using OnSubscribeUserCallback = std::function<void(uint16_t packet_id, uint8_t qos)>;
  using OnUnsubscribeUserCallback = std::function<void(uint16_t packet_id)>;
  using OnMessageUserCallback = std::function<void(
      char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)>;
  using OnPublishUserCallback = std::function<void(uint16_t packet_id)>;
  static const size_t MQTT_BUFFER_SIZE = 4096;

  AsyncMqttClient &setKeepAlive(uint16_t keep_alive) {  // NOLINT(readability-identifier-naming)
    mqtt_cfg_.keepalive = keep_alive;
    return *this;
  }
  AsyncMqttClient &setClientId(const char *client_id) {  // NOLINT(readability-identifier-naming)
    this->client_id_ = client_id;
    return *this;
  }
  AsyncMqttClient &setCleanSession(bool clean_session) {  // NOLINT(readability-identifier-naming)
    mqtt_cfg_.disable_clean_session = !clean_session;
    return *this;
  }
  AsyncMqttClient &setMaxTopicLength(uint16_t max_topic_length) {  // NOLINT(readability-identifier-naming)
    // not required - ignored
    return *this;
  }
  AsyncMqttClient &setCredentials(const char *username,  // NOLINT(readability-identifier-naming)
                                  const char *password = nullptr) {
    if (username)
      this->username_ = username;
    if (password)
      this->password_ = password;
    return *this;
  }
  AsyncMqttClient &setWill(const char *topic, uint8_t qos, bool retain,  // NOLINT(readability-identifier-naming)
                           const char *payload = nullptr, size_t length = 0) {
    if (topic)
      this->lwt_topic_ = topic;
    this->mqtt_cfg_.lwt_qos = qos;
    if (payload)
      this->lwt_message_ = payload;
    this->mqtt_cfg_.lwt_retain = retain;
    return *this;
  }
  AsyncMqttClient &setServer(network::IPAddress ip, uint16_t port) {  // NOLINT(readability-identifier-naming)
    host_ = ip.str();
    this->mqtt_cfg_.host = host_.c_str();
    this->mqtt_cfg_.port = port;
    return *this;
  }
  AsyncMqttClient &setServer(const char *host, uint16_t port) {  // NOLINT(readability-identifier-naming)
    host_ = host;
    this->mqtt_cfg_.host = host_.c_str();
    this->mqtt_cfg_.port = port;
    return *this;
  }
  AsyncMqttClient &onConnect(const OnConnectUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_connect_.push_back(callback);
    return *this;
  }
  AsyncMqttClient &onDisconnect(const OnDisconnectUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_disconnect_.push_back(callback);
    return *this;
  }
  AsyncMqttClient &onSubscribe(const OnSubscribeUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_subscribe_.push_back(callback);
    return *this;
  }
  AsyncMqttClient &onUnsubscribe(const OnUnsubscribeUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_unsubscribe_.push_back(callback);
    return *this;
  }
  AsyncMqttClient &onMessage(const OnMessageUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_message_.push_back(callback);
    return *this;
  }
  AsyncMqttClient &onPublish(const OnPublishUserCallback &callback) {  // NOLINT(readability-identifier-naming)
    this->on_publish_.push_back(callback);
    return *this;
  }

  bool connected() const { return this->is_connected_; }
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
  std::string host_;
  std::string username_;
  std::string password_;
  std::string lwt_topic_;
  std::string lwt_message_;
  std::string client_id_;
  optional<std::string> ca_certificate_;
  bool skip_cert_cn_check_{false};
  // callbacks
  std::vector<OnConnectUserCallback> on_connect_;
  std::vector<OnDisconnectUserCallback> on_disconnect_;
  std::vector<OnSubscribeUserCallback> on_subscribe_;
  std::vector<OnUnsubscribeUserCallback> on_unsubscribe_;
  std::vector<OnMessageUserCallback> on_message_;
  std::vector<OnPublishUserCallback> on_publish_;
};

}  // namespace mqtt
}  // namespace esphome
#endif
