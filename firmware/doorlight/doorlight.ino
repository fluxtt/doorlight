/*
    Door Status Light - ESP32 + MQTT
    Shows one of three states via LED:
        dnd -> red
        busy -> amber
        free -> green

    Publishes Home Assistant MQTT discovery on connect, so it will appear automatically as a "Door Status" select entity the moment Home Assistant is pointed at the same broker. No HA config needed.

    Libraries required (Arduino Library Manager):
      - PubSubClient by Nick O'Leary

    Board: DOIT ESP32 Devkit v1 (or any ESP32)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const int MQTT_PORT = 1883;

const char* DEVICE_ID       = "doorlight01";
const char* STATE_TOPIC     = "doorlight/status/state";
const char* COMMAND_TOPIC   = "doorlight/status/set";
const char* AVAIL_TOPIC     = "doorlight/status/availability";
const char* DISCOVERY_TOPIC = "homeassistant/select/doorlight_status/config";

// ---------------- LED pins ----------------
// Change these if you want to reuse different GPIOs.
// Avoid input-only pins (34-39) and strapping pins (0, 2, 15, 12) for outputs.
const int PIN_RED   = 25;
const int PIN_AMBER = 26;
const int PIN_GREEN = 27;

WiFiClient   espClient;
PubSubClient mqtt(espClient);

void setLeds(const String& state) {
  digitalWrite(PIN_RED,   state == "dnd"  ? HIGH : LOW);
  digitalWrite(PIN_AMBER, state == "busy" ? HIGH : LOW);
  digitalWrite(PIN_GREEN, state == "free" ? HIGH : LOW);
}

void publishState() {
  mqtt.publish(STATE_TOPIC, currentState.c_str(), true); // retained
}

void publishDiscovery() {
  // MQTT Discovery payload for a Home Assistant "select" entity.
  // Drop this device into an HA instance pointed at the same broker
  // and it appears automatically under MQTT integration -> devices.
  String payload = String("{") +
    "\"name\":\"Door Status\"," +
    "\"unique_id\":\"" + DEVICE_ID + "_status\"," +
    "\"command_topic\":\"" + COMMAND_TOPIC + "\"," +
    "\"state_topic\":\"" + STATE_TOPIC + "\"," +
    "\"options\":[\"dnd\",\"busy\",\"free\"]," +
    "\"availability_topic\":\"" + AVAIL_TOPIC + "\"," +
    "\"payload_available\":\"online\"," +
    "\"payload_not_available\":\"offline\"," +
    "\"device\":{" +
      "\"identifiers\":[\"" + DEVICE_ID + "\"]," +
      "\"name\":\"Door Light\"," +
      "\"manufacturer\":\"fluxt\"," +
      "\"model\":\"ESP32 Door Status Light\"" +
    "}" +
  "}";
  mqtt.publish(DISCOVERY_TOPIC, payload.c_str(), true);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  if (msg == "dnd" || msg == "busy" || msg == "free") {
    currentState = msg;
    setLeds(currentState);
    publishState();
  }
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT broker...");
    String clientId = String(DEVICE_ID);
    bool ok;
    if (strlen(MQTT_USER) > 0) {
      ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, AVAIL_TOPIC, 0, true, "offline");
    } else {
      ok = mqtt.connect(clientId.c_str(), AVAIL_TOPIC, 0, true, "offline");
    }

    if (ok) {
      Serial.println("connected");
      mqtt.publish(AVAIL_TOPIC, "online", true);
      mqtt.subscribe(COMMAND_TOPIC);
      publishDiscovery();
      publishState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" -- retrying in 2s");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_AMBER, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  setLeds(currentState);

  connectWifi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512); // discovery payload is bigger than PubSubClient's 256-byte default
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
  if (!mqtt.connected()) {
    connectMqtt();
  }
  mqtt.loop();
}
