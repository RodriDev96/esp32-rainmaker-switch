// ================= INCLUDES =================
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "AppInsights.h"

// ================= CONFIG =================
#define DEFAULT_POWER_MODE false   // 🔴 SEMPRE COMEÇA DESLIGADO

const char *service_name = "PROV_1234";
const char *pop = "abcd1234";

// ================= LEDS =================
#define LED_WIFI 2     // Pisca quando WiFi OFF
#define LED_MSG  4     // Pisca quando recebe comando

volatile bool wifiOnline = false;
volatile bool msgReceived = false;

// ================= GPIO =================
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
static int gpio_0 = 9;
static int gpio_switch = 7;
#else
static int gpio_0 = 0;
static int gpio_switch = 16;
#endif

bool switch_state = DEFAULT_POWER_MODE;
static Switch *my_switch = NULL;

// ================= TASK LED WIFI =================
void ledWifiTask(void *pvParameters) {
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, LOW);

  while (true) {
    if (!wifiOnline) {
      digitalWrite(LED_WIFI, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(LED_WIFI, LOW);
      vTaskDelay(pdMS_TO_TICKS(500));
    } else {
      digitalWrite(LED_WIFI, LOW);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// ================= TASK LED MSG =================
void ledMsgTask(void *pvParameters) {
  pinMode(LED_MSG, OUTPUT);
  digitalWrite(LED_MSG, LOW);

  while (true) {
    if (msgReceived) {
      msgReceived = false;

      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_MSG, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(LED_MSG, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= EVENTOS WIFI / PROV =================
void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {

    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32S2
      Serial.printf("Provisioning SoftAP: %s | PoP: %s\n", service_name, pop);
      WiFiProv.printQR(service_name, pop, "softap");
#else
      Serial.printf("Provisioning BLE: %s | PoP: %s\n", service_name, pop);
      WiFiProv.printQR(service_name, pop, "ble");
#endif
      wifiOnline = false;
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi ONLINE");
      wifiOnline = true;
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi OFFLINE");
      wifiOnline = false;
      break;

    case ARDUINO_EVENT_PROV_INIT:
      WiFiProv.disableAutoStop(10000);
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      WiFiProv.endProvision();
      break;

    default:
      break;
  }
}

// ================= CALLBACK RAINMAKER =================
void write_callback(Device *device, Param *param, const param_val_t val,
                    void *priv_data, write_ctx_t *ctx) {

  if (strcmp(param->getParamName(), ESP_RMAKER_DEF_POWER_NAME) == 0) {

    msgReceived = true;

    switch_state = val.val.b;
    digitalWrite(gpio_switch, switch_state ? HIGH : LOW);

    param->updateAndReport(val);
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(gpio_0, INPUT_PULLUP);
  pinMode(gpio_switch, OUTPUT);

  // 🔒 Força relé desligado no boot
  digitalWrite(gpio_switch, DEFAULT_POWER_MODE);

  // ===== RainMaker =====
  Node my_node = RMaker.initNode("ESP RainMaker Node");

  my_switch = new Switch("Switch", &gpio_switch);
  my_switch->addCb(write_callback);
  my_node.addDevice(*my_switch);

  // 🔁 Sincroniza estado inicial OFF no app
  my_switch->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER_MODE);

  RMaker.enableOTA(OTA_USING_TOPICS);
  RMaker.enableTZService();
  RMaker.enableSchedule();
  RMaker.enableScenes();
  initAppInsights();
  RMaker.enableSystemService(SYSTEM_SERV_FLAGS_ALL, 2, 2, 2);

#if CONFIG_IDF_TARGET_ESP32S2
  WiFiProv.initProvision(NETWORK_PROV_SCHEME_SOFTAP,
                         NETWORK_PROV_SCHEME_HANDLER_NONE);
#else
  WiFiProv.initProvision(NETWORK_PROV_SCHEME_BLE,
                         NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM);
#endif

  RMaker.start();
  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32S2
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_SOFTAP,
                          NETWORK_PROV_SCHEME_HANDLER_NONE,
                          NETWORK_PROV_SECURITY_1,
                          pop, service_name);
#else
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE,
                          NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
                          NETWORK_PROV_SECURITY_1,
                          pop, service_name);
#endif

  // ===== TASKS =====
  xTaskCreatePinnedToCore(ledWifiTask, "LED_WIFI", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(ledMsgTask,  "LED_MSG",  2048, NULL, 1, NULL, 1);
}

// ================= LOOP =================
void loop() {
  if (digitalRead(gpio_0) == LOW) {

    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_0) == LOW) delay(50);
    int pressTime = millis() - startTime;

    if (pressTime > 10000) {
      Serial.println("Factory Reset");
      RMakerFactoryReset(2);

    } else if (pressTime > 3000) {
      Serial.println("WiFi Reset");
      RMakerWiFiReset(2);

    } else {
      // 🔘 Botão físico
      switch_state = !switch_state;
      msgReceived = true;

      my_switch->updateAndReportParam(
        ESP_RMAKER_DEF_POWER_NAME,
        switch_state
      );

      digitalWrite(gpio_switch, switch_state ? HIGH : LOW);
    }
  }
  delay(100);
}
