#ifndef LightStateService_h
#define LightStateService_h

#include <LightMqttSettingsService.h>

#include <HttpEndpoint.h>
#include <MqttPubSub.h>
#include <WebSocketTxRx.h>
#include <FastLED.h>
#include <JsonUtil.h>
#include <LightEffect.h>
#include <map>

// fast led settings
#define LED_DATA_PIN 14  // was 21 for esp32
#define COLOR_ORDER GRB
#define LED_TYPE WS2812
#define NUM_LEDS 96
#define NUM_MODES 7

#define DEFAULT_EFFECT "Manual"
#define DEFAULT_LED_STATE false
#define OFF_STATE "OFF"
#define ON_STATE "ON"

#define LIGHT_SETTINGS_ENDPOINT_PATH "/rest/lightState"
#define LIGHT_SETTINGS_SOCKET_PATH "/ws/lightState"

typedef std::map<String, void*> LightEffectMap;
typedef std::pair<String, void*> LightEffectPair;

class LightState {
 public:
  bool ledOn;
  uint8_t brightness;
  CRGB color;
  String effect;

  static void read(LightState& lightState, JsonObject& root) {
    root["led_on"] = lightState.ledOn;
    root["color"] = colorToHexString(lightState.color);
    root["brightness"] = lightState.brightness;
    root["effect"] = lightState.effect;
  }

  static StateUpdateResult update(JsonObject& root, LightState& lightState) {
    lightState.ledOn = root["led_on"] | DEFAULT_LED_STATE;
    String color = root["color"];
    lightState.color = hexStringToColor(color, CRGB::White);
    lightState.brightness = root["brightness"] | 255;
    lightState.effect = root["effect"] | DEFAULT_EFFECT;

    return StateUpdateResult::CHANGED;
  }

  static void haRead(LightState& lightState, JsonObject& root) {
    root["state"] = lightState.ledOn ? ON_STATE : OFF_STATE;
    colorToRGBJson(lightState.color, root);
    root["brightness"] = lightState.brightness;
    root["effect"] = lightState.effect;
  }

  static StateUpdateResult haUpdate(JsonObject& root, LightState& lightState) {
    String state = root["state"];
    lightState.ledOn = state.equals(ON_STATE);
    rgbJsonToColor(root, lightState.color);
    lightState.brightness = root["brightness"] | lightState.brightness;
    lightState.effect = root["effect"] | lightState.effect;

    return StateUpdateResult::CHANGED;
  }
};

class LightStateService : public StatefulService<LightState> {
 public:
  LightStateService(AsyncWebServer* server,
                    SecurityManager* securityManager,
                    AsyncMqttClient* mqttClient,
                    LightMqttSettingsService* lightMqttSettingsService);
  void begin();
  void loop();

  // temp - LED controller should be suppliex externally
  CLEDController* getLedController();
  void addEffect(String key, void* lightEffect);

 private:
  HttpEndpoint<LightState> _httpEndpoint;
  MqttPubSub<LightState> _mqttPubSub;
  WebSocketTxRx<LightState> _webSocket;
  AsyncMqttClient* _mqttClient;
  LightMqttSettingsService* _lightMqttSettingsService;
  
  LightEffectMap _lightEffects;
  LightEffect* _currentEffect = nullptr;

  bool _refresh;
  CRGB _leds[NUM_LEDS];
  CLEDController* _ledController;

  void registerConfig();
  void onConfigUpdated();
};

#endif
