#pragma once

///
/// Mirror the public interface of MqttClientBase using esp-idf
///

#ifdef USE_ESP_IDF
#include <string>
#include <map>
#include <mqtt_client.h>
#include "esphome/components/network/ip_address.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace mqtt {

enum class MqttEvent {
  MQTT_EVENT_ANY = -1,
  MQTT_EVENT_ERROR = 0,
  MQTT_EVENT_CONNECTED,
  MQTT_EVENT_DISCONNECTED,
  MQTT_EVENT_SUBSCRIBED,
  MQTT_EVENT_UNSUBSCRIBED,
  MQTT_EVENT_PUBLISHED,
  MQTT_EVENT_DATA,
  MQTT_EVENT_BEFORE_CONNECT,
  MQTT_EVENT_DELETED,
};

struct MqttEvent_t {
  char *data;              /*!< Data associated with this event */
  int data_len;            /*!< Length of the data for this event */
  int total_data_len;      /*!< Total length of the data (longer data are supplied with multiple events) */
  int current_data_offset; /*!< Actual offset for the data associated with this event */
  char *topic;             /*!< Topic associated with this event */
  int topic_len;           /*!< Length of the topic for this event associated with this event */
  int msg_id;              /*!< MQTT messaged id of message */
  int session_present;     /*!< MQTT session_present flag for connection event */
  bool retain;             /*!< Retained flag of the message associated with this event */
  int qos;                 /*!< qos of the messages associated with this event */
  bool dup;                /*!< dup flag of the message associated with this event */
};

enum class MqttClientDisconnectReason : int8_t {
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  ESP8266_NOT_ENOUGH_SPACE = 6,
  TLS_BAD_FINGERPRINT = 7
};

class MqttClientBase {
 public:
  using on_connect_callback_t = std::function<void(bool session_present)>;
  using on_disconnect_callback_t = std::function<void(MqttClientDisconnectReason reason)>;
  using on_subscribe_callback_t = std::function<void(uint16_t packet_id, uint8_t qos)>;
  using on_unsubscribe_callback_t = std::function<void(uint16_t packet_id)>;
  using on_message_callback_t = std::function<void(const char *topic, char *payload, uint8_t qos, bool dup, bool retain,
                                                   size_t len, size_t index, size_t total)>;
  using on_publish_uer_callback_t = std::function<void(uint16_t packet_id)>;

  void set_keep_alive(uint16_t keep_alive) { this->keep_alive_ = keep_alive; }
  void set_client_id(const char *client_id) { this->client_id_ = client_id; }
  void set_clean_session(bool clean_session) { this->clean_session_ = clean_session; }

  void set_credentials(const char *username, const char *password = nullptr) {
    if (username)
      this->username_ = username;
    if (password)
      this->password_ = password;
  }
  void set_will(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr, size_t length = 0) {
    if (topic)
      this->lwt_topic_ = topic;
    this->lwt_qos_ = qos;
    if (payload)
      this->lwt_message_ = payload;
    this->lwt_retain_ = retain;
  }
  void set_server(network::IPAddress ip, uint16_t port) {
    this->host_ = ip.str();
    this->port_ = port;
  }
  void set_server(const char *host, uint16_t port) {
    this->host_ = host;
    this->port_ = port;
  }
  void set_on_connect(const on_connect_callback_t &callback) { this->on_connect_.push_back(callback); }
  void set_on_disconnect(const on_disconnect_callback_t &callback) { this->on_disconnect_.push_back(callback); }
  void set_on_subscribe(const on_subscribe_callback_t &callback) { this->on_subscribe_.push_back(callback); }
  void set_on_unsubscribe(const on_unsubscribe_callback_t &callback) { this->on_unsubscribe_.push_back(callback); }
  void set_on_message(const on_message_callback_t &callback) { this->on_message_.push_back(callback); }
  void set_on_publish(const on_publish_uer_callback_t &callback) { this->on_publish_.push_back(callback); }
  bool connected() const { return this->is_connected_; }
  virtual void connect() = 0;
  virtual void disconnect(bool) = 0;
  virtual uint16_t subscribe(const char *topic, uint8_t qos) = 0;
  virtual uint16_t unsubscribe(const char *topic) = 0;
  virtual uint16_t publish(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr,
                           size_t length = 0, bool unused_dup = false, uint16_t unused_message_id = 0) = 0;
  virtual uint16_t publish(const std::string &topic, uint8_t qos, bool retain, const std::string &payload = 0,
                           size_t length = 0, bool unused_dup = false, uint16_t unused_message_id = 0) {
    return publish(topic.c_str(), qos, retain, payload.c_str(), length, unused_dup, unused_message_id);
  }

 protected:
  virtual bool initialize_() = 0;

  bool is_connected_{false};
  bool is_initalized_{false};

  std::string host_;
  uint16_t port_;
  std::string username_;
  std::string password_;
  std::string lwt_topic_;
  std::string lwt_message_;
  uint8_t lwt_qos_;
  bool lwt_retain_;
  std::string client_id_;
  uint16_t keep_alive_;
  bool clean_session_;
  // callbacks
  std::vector<on_connect_callback_t> on_connect_;
  std::vector<on_disconnect_callback_t> on_disconnect_;
  std::vector<on_subscribe_callback_t> on_subscribe_;
  std::vector<on_unsubscribe_callback_t> on_unsubscribe_;
  std::vector<on_message_callback_t> on_message_;
  std::vector<on_publish_uer_callback_t> on_publish_;
};

}  // namespace mqtt
}  // namespace esphome
#endif
