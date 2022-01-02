#pragma once
#if !defined(USE_ESP_IDF)
#include "mqtt_client_base.h"
#include <AsyncMqttClient.h>
#include "lwip/ip_addr.h"

struct MqttClientMessageProperties {
  uint8_t qos;
  bool dup;
  bool retain;
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

class MQTTArduinoClient : public MQTTClientBase {
 public:
  using OnConnectUserCallback = std::function<void(bool sessionPresent)>;
  using OnDisconnectUserCallback = std::function<void(MqttClientDisconnectReason reason)>;
  using OnSubscribeUserCallback = std::function<void(uint16_t packetId, uint8_t qos)>;
  using OnUnsubscribeUserCallback = std::function<void(uint16_t packetId)>;
  using OnMessageUserCallback = std::function<void(char *topic, char *payload, MqttClientMessageProperties properties,
                                                   size_t len, size_t index, size_t total)>;
  using OnPublishUserCallback = std::function<void(uint16_t packetId)>;

  MQTTClient &setKeepAlive(uint16_t keepAlive) {
    mqtt_client_.setKeepAlive(keepAlive);
    return *this;
  }
  MQTTClient &setClientId(const char *clientId) {
    mqtt_client_.setClientId(clientId);
    return *this;
  }
  MQTTClient &setCleanSession(bool cleanSession) {
    mqtt_client_.setCleanSession(cleanSession);
    return *this;
  }
  MQTTClient &setMaxTopicLength(uint16_t maxTopicLength) {
    mqtt_client_.setMaxTopicLength(maxTopicLength);
    return *this;
  };
  MQTTClient &setCredentials(const char *username, const char *password = nullptr) {
    mqtt_client_.setCredentials(username, password);
    return *this;
  }
  MQTTClient &setWill(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr, size_t length = 0) {
    mqtt_client_.setWill(topic, qos, retain, payload);
    return *this;
  }
  MQTTClient &setServer(IPAddress ip, uint16_t port) {
    mqtt_client_.setServer(ip, port);
    return *this;
  }
  MQTTClient &setServer(const char *host, uint16_t port) {
    mqtt_client_.setServer(host, port);
    return *this;
  }
#if ASYNC_TCP_SSL_ENABLED
  MQTTClientBase &setSecure(bool secure) {
    mqtt_client.setSecure(secure);
    return *this;
  }
  MQTTClientBase &addServerFingerprint(const uint8_t *fingerprint) {
    mqtt_client.addServerFingerprint(fingerprint);
    return *this;
  }
#endif

  MQTTClient &onConnect(OnConnectUserCallback callback) {
    this->mqtt_client_.onConnect(callback);
    return *this;
  }
  MQTTClient &onDisconnect(OnDisconnectUserCallback callback) {
    std::function<void(AsyncMqttClientDisconnectReason)> async_callback =
        [callback](AsyncMqttClientDisconnectReason reason) {
          // int based enum so casting isn't a problem
          callback(static_cast<MqttClientDisconnectReason>(reason));
        };
    this->mqtt_client_.onDisconnect(async_callback);
    return *this;
  }
  MQTTClient &onSubscribe(OnSubscribeUserCallback callback) {
    this->mqtt_client_.onSubscribe(callback);
    return *this;
  }
  MQTTClient &onUnsubscribe(OnUnsubscribeUserCallback callback) {
    this->mqtt_client_.onUnsubscribe(callback);
    return *this;
  }
  MQTTClient &onMessage(OnMessageUserCallback callback) {
    std::function<void(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len,
                       size_t index, size_t total)>
        async_callback = [callback](char *topic, char *payload, AsyncMqttClientMessageProperties async_properties,
                                    size_t len, size_t index, size_t total) {
          MqttClientMessageProperties properties;
          properties.dup = async_properties.dup;
          properties.qos = async_properties.qos;
          properties.retain = async_properties.retain;
          callback(topic, payload, properties, len, index, total);
        };
    mqtt_client_.onMessage(async_callback);
    return *this;
  }
  MQTTClient &onPublish(OnPublishUserCallback callback) {
    this->mqtt_client_.onPublish(callback);
    return *this;
  }

  bool connected() const { return mqtt_client_.connected(); }
  void connect() { mqtt_client_.connect(); }
  void disconnect(bool force = false) { mqtt_client_.disconnect(force); }
  uint16_t subscribe(const char *topic, uint8_t qos) { return mqtt_client_.subscribe(topic, qos); }
  uint16_t unsubscribe(const char *topic) { return mqtt_client_.unsubscribe(topic); }
  uint16_t publish(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr, size_t length = 0,
                   bool dup = false, uint16_t message_id = 0) {
    return mqtt_client_.publish(topic, qos, retain, payload, length, dup, message_id);
  }

 protected:
  AsyncMqttClient mqtt_client_;
};
#endif 