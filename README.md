# Door Status Light - Setup

## 1. Wiring

| LED   | ESP32 GPIO |
| ----- | ---------- |
| Red   | 25         |
| Amber | 26         |
| Green | 27         |

^ CHECK THIS

Each LED: GPIO -> 220 resistor -> LED anode -> LED cathode -> GND.
Change the `PIN_RED` / `PIN_AMBER` / `PIN_GREEN` constants in the firmware if you want different pins.

## 2. Broker (on Docker host)

```bash
cd broker
docker compose up -d
```

This starts Mosquito with:

- port `1883` - plain MQTT (used by the ESP32 and later Home Assistant)
- port `9001` - MQTT over websockets (used by the browser control page)

Security note: `mosquitto.conf` currently has `allow_anonymouse true` for simplicity. Fine on your home LAN, but if you ever expose this broke outside your network (eveon over Tailscale), add a username/password (`mosquitto_passwd`) and set `allow_anonymouse false` first.

## 3. Firmware

1. In Arduino IDE, install the **PubSubClient** library (Nick O'Leary) via Library Manager.
2. Open `firmware/doorlight.ino`.
3. Fill in `WIFI_SSID` / `WIFI_PASSWORD` and confirm `MQTT_HOST` matches your broker.
4. Flash to the ESP32. Open Serial Monitor at 115200 baud to confirm it connects to WiFi and MQTT.

The device boots to `free`, then waits for messages on `doorlight/status/set` (`dnd`,`busy`, or `free`).

## 4. Control page

Open `webpage/control.html` in any browser (phone or PC), on the same network as the broker (or over Tailscale once that's set up). Tap a button o publish the new state - the ESP32 updates the LEDs immediately, and the page reflects the actual retained state on load/reconnect.

This file is a static file with no backend, so can be hosted anywhere with ease.

## 5. Moving to Home Assistant later

Nothing extra to do. The firmware publishes an MQTT Discovery message on every connect, so as soon as Home Assistant's MQTT integration points at the same broker, a "Door Status" select entity appears automatically under that device, with `Do Not Disturb` / `Busy` / `Free` as its options. From there you get:

- A dashboard tile/dropdown for free
- Automations (e.g. auto-set "busy" during calendar events, or tie it to your PC's lock state via an HA companion integration)
- History/logbook of your status changes

## Topics reference

| Topic                         | Direction            | Payload                          |
| ----------------------------- | -------------------- | -------------------------------- |
| doorlight/status/set          | phone/PC -> ESP32    | dnd / busy / free                |
| doorlight/status/state        | ESP32 -> subscribers | dnd / busy / free (retained)     |
| doorlight/status/availability | ESP32 -> subscribers | online / offline (LWT, retained) |
