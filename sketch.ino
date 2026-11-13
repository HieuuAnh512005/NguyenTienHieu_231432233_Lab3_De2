
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <math.h>

// =========================================
// LAB 3 - DE 2
// ESP32 DEVKITC V4 + DHT22 + FREERTOS + MQTT
// =========================================

// 1. KHAI BAO CHAN
#define DHT_PIN 15
#define DHT_TYPE DHT22

#define PUMP_PIN 18
#define FAN_PIN 19
#define MIST_PIN 23

DHT dht(DHT_PIN, DHT_TYPE);

// 2. WIFI VA MQTT
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* TOPIC_SENSOR = "lab3/de2/sensor";
const char* TOPIC_PUMP = "lab3/de2/pump/set";
const char* TOPIC_FAN = "lab3/de2/fan/set";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// 3. DU LIEU CAM BIEN
struct SensorData {
  float temperature;
  float humidity;
};

QueueHandle_t dataQueue;
QueueHandle_t dataAlert;

// =========================================
// 4. XAC DINH MUC DO AM
// =========================================
int getHumidityLevel(float humidity) {
  if (humidity < 30.0) return 3;
  if (humidity < 35.0) return 2;
  if (humidity < 40.0) return 1;
  return 0;
}

// =========================================
// 5. DIEU KHIEN PWM PHUN SUONG
// =========================================
void setMistPower(int percent) {
  percent = constrain(percent, 0, 100);

  int pwm = map(percent, 0, 100, 0, 255);

  analogWrite(MIST_PIN, pwm);

  Serial.printf(
    "[PHUN SUONG] Cong suat: %d%% | PWM: %d\n",
    percent, pwm
  );
}

// =========================================
// 6. NHAN LENH MQTT MANUAL
// =========================================
void mqttCallback(char* topic, byte* payload,
                  unsigned int length) {

  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();
  message.toUpperCase();

  if (String(topic) == TOPIC_PUMP) {
    if (message == "ON" || message == "1") {
      digitalWrite(PUMP_PIN, HIGH);
      Serial.println("[MAY BOM] BAT");
    }
    else if (message == "OFF" || message == "0") {
      digitalWrite(PUMP_PIN, LOW);
      Serial.println("[MAY BOM] TAT");
    }
  }

  if (String(topic) == TOPIC_FAN) {
    if (message == "ON" || message == "1") {
      digitalWrite(FAN_PIN, HIGH);
      Serial.println("[QUAT] BAT");
    }
    else if (message == "OFF" || message == "0") {
      digitalWrite(FAN_PIN, LOW);
      Serial.println("[QUAT] TAT");
    }
  }
}

// =========================================
// 7. TASK DOC DHT22 - CORE 1
// =========================================
void TaskReadData(void* pvParameters) {

  SensorData data;

  Serial.printf(
    "[TASK DOC] Core %d\n",
    xPortGetCoreID()
  );

  while (true) {

    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();

    if (isnan(data.temperature) ||
        isnan(data.humidity)) {

      Serial.println("[DHT22] Loi doc cam bien!");

    } else {

      Serial.println();
      Serial.println("========== DU LIEU DHT22 ==========");

      Serial.printf(
        "Nhiet do : %.1f C\n",
        data.temperature
      );

      Serial.printf(
        "Do am    : %.1f %%\n",
        data.humidity
      );

      Serial.printf(
        "Task doc : Core %d\n",
        xPortGetCoreID()
      );

      // Gui du lieu den Task MQTT
      xQueueOverwrite(dataQueue, &data);

      // Gui do am den Task canh bao
      xQueueOverwrite(
        dataAlert,
        &data.humidity
      );

      Serial.println("[dataQueue] Gui thanh cong");
      Serial.println("[dataAlert] Gui thanh cong");
      Serial.println("===================================");

    }

    // Doc DHT22 moi 2 giay
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// =========================================
// 8. TASK CANH BAO - CORE 1
// DEM NGUOC THOI GIAN PHUN SUONG
// =========================================
void TaskAlert(void* pvParameters) {

  float humidity = 0;

  int lastLevel = -1;
  int currentPower = 0;

  bool mistActive = false;

  unsigned long mistStart = 0;
  unsigned long mistDuration = 0;

  int lastRemaining = -1;

  Serial.printf(
    "[TASK CANH BAO] Core %d\n",
    xPortGetCoreID()
  );

  while (true) {

    // NHAN DO AM TU QUEUE
    if (xQueueReceive(
          dataAlert,
          &humidity,
          pdMS_TO_TICKS(50)
        ) == pdPASS) {

      int level = getHumidityLevel(humidity);

      // NEU DO AM CHUYEN SANG MUC KHAC
      if (level != lastLevel) {

        // Huy chu ky phun cu
        if (mistActive) {
          Serial.println();
          Serial.println(
            "[CANH BAO] Huy chu ky phun cu!"
          );
        }

        setMistPower(0);
        mistActive = false;

        Serial.println();
        Serial.println("========== CANH BAO ==========");

        Serial.printf(
          "Do am hien tai: %.1f %%\n",
          humidity
        );

        int power = 0;
        unsigned long duration = 0;

        switch (level) {

          case 1:
            power = 25;
            duration = 10000;
            break;

          case 2:
            power = 50;
            duration = 20000;
            break;

          case 3:
            power = 100;
            duration = 30000;
            break;

          default:
            power = 0;
            duration = 0;
            break;
        }

        if (level == 0) {

          Serial.println(
            "[TRANG THAI] Do am khong can phun"
          );

          Serial.println(
            "[PHUN SUONG] TAT"
          );

          currentPower = 0;

        } else {

          currentPower = power;

          mistDuration = duration;
          mistStart = millis();

          mistActive = true;

          setMistPower(power);

          Serial.printf(
            "[THOI GIAN] Bat dau %lu giay\n",
            duration / 1000UL
          );

          // Buoc dem ngược moi
          lastRemaining = -1;
        }

        lastLevel = level;

        Serial.println("==============================");
      }
    }

    // =====================================
    // BO DEM NGUOC KHONG BLOCKING
    // =====================================
    if (mistActive) {

      unsigned long elapsed =
        millis() - mistStart;

      // NEU DA HET THOI GIAN
      if (elapsed >= mistDuration) {

        setMistPower(0);

        mistActive = false;
        currentPower = 0;

        Serial.println();
        Serial.println(
          "[COUNTDOWN] Con lai: 0 giay"
        );

        Serial.println(
          "[PHUN SUONG] HOAN THANH!"
        );

        Serial.println(
          "[PHUN SUONG] DA TAT"
        );

      } else {

        // Lam tron len so giay con lai
        unsigned long remainingMs =
          mistDuration - elapsed;

        int remainingSeconds =
          (remainingMs + 999) / 1000;

        // Chi in khi so giay thay doi
        if (remainingSeconds != lastRemaining) {

          lastRemaining = remainingSeconds;

          Serial.printf(
            "[COUNTDOWN] Phun %d%% | Con %d giay\n",
            currentPower,
            remainingSeconds
          );
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// =========================================
// 9. TASK MQTT - CORE 0
// =========================================
void TaskMQTT(void* pvParameters) {

  SensorData data;

  unsigned long lastWiFiAttempt = 0;
  unsigned long lastMQTTAttempt = 0;

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  Serial.printf(
    "[TASK MQTT] Core %d\n",
    xPortGetCoreID()
  );

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("[WiFi] Dang ket noi...");

  while (true) {

    // KIEM TRA WIFI
    if (WiFi.status() != WL_CONNECTED) {

      if (millis() - lastWiFiAttempt >= 5000) {

        lastWiFiAttempt = millis();

        Serial.println(
          "[WiFi] Thu ket noi lai..."
        );

        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
      }

      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // KIEM TRA MQTT
    if (!mqtt.connected()) {

      if (lastMQTTAttempt == 0 ||
          millis() - lastMQTTAttempt >= 5000) {

        lastMQTTAttempt = millis();

        Serial.println(
          "[MQTT] Dang ket noi..."
        );

        String clientId = "Lab3_De2_";

        clientId += String(
          (uint32_t)ESP.getEfuseMac(),
          HEX
        );

        if (mqtt.connect(clientId.c_str())) {

          Serial.println(
            "[MQTT] Ket noi thanh cong!"
          );

          mqtt.subscribe(TOPIC_PUMP);
          mqtt.subscribe(TOPIC_FAN);

          Serial.println(
            "[MQTT] Da dang ky topic dieu khien"
          );

        } else {

          Serial.printf(
            "[MQTT] Ket noi that bai: %d\n",
            mqtt.state()
          );
        }
      }

      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    mqtt.loop();

    // NHAN DU LIEU TU QUEUE
    if (xQueueReceive(
          dataQueue,
          &data,
          0
        ) == pdPASS) {

      char payload[160];

      snprintf(
        payload,
        sizeof(payload),
        "{\"temperature\":%.1f,\"humidity\":%.1f}",
        data.temperature,
        data.humidity
      );

      bool result = mqtt.publish(
        TOPIC_SENSOR,
        payload
      );

      if (result) {

        Serial.println();
        Serial.println("========== MQTT ==========");

        Serial.printf(
          "Core: %d\n",
          xPortGetCoreID()
        );

        Serial.printf(
          "Data: %s\n",
          payload
        );

        Serial.println(
          "[MQTT] Gui thanh cong!"
        );

        Serial.println("==========================");

      } else {

        Serial.println(
          "[MQTT] Gui that bai!"
        );
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// =========================================
// 10. SETUP
// =========================================
void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("          LAB 3 - DE 2           ");
  Serial.println("=================================");

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(MIST_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  analogWrite(MIST_PIN, 0);

  dht.begin();

  // Queue chi luu mau moi nhat
  dataQueue = xQueueCreate(
    1,
    sizeof(SensorData)
  );

  dataAlert = xQueueCreate(
    1,
    sizeof(float)
  );

  if (dataQueue == NULL ||
      dataAlert == NULL) {

    Serial.println("[ERROR] Khong tao duoc Queue!");

    while (true) {
      delay(1000);
    }
  }

  Serial.println(
    "[FreeRTOS] Tao Queue thanh cong"
  );

  // TASK MQTT - CORE 0
  BaseType_t mqttOK = xTaskCreatePinnedToCore(
    TaskMQTT,
    "Task MQTT",
    6144,
    NULL,
    1,
    NULL,
    0
  );

  // TASK DOC DHT22 - CORE 1
  BaseType_t readOK = xTaskCreatePinnedToCore(
    TaskReadData,
    "Task Read",
    4096,
    NULL,
    2,
    NULL,
    1
  );

  // TASK CANH BAO - CORE 1
  BaseType_t alertOK = xTaskCreatePinnedToCore(
    TaskAlert,
    "Task Alert",
    4096,
    NULL,
    2,
    NULL,
    1
  );

  if (mqttOK != pdPASS ||
      readOK != pdPASS ||
      alertOK != pdPASS) {

    Serial.println(
      "[ERROR] Loi tao Task!"
    );

  } else {

    Serial.println(
      "[FreeRTOS] Tao 3 Task thanh cong!"
    );
  }

  Serial.println(
    "[SYSTEM] Khoi tao hoan tat!"
  );
}

// =========================================
// 11. LOOP
// =========================================
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
