#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <esp_log.h>
#include <PubSubClient.h>

#define TAG "main"

// Initialize Variables
const char* ssid = "EZEKIEL";
const char* password = "late1978";
const char* mqtt_server = "broker.emqx.io";
const char* topic = "ton/server/m5";
const int mqtt_port = 1883;

// Function Declaration
void setup_wifi(void);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect(void);
void setup_ble(void);

// Initialize WiFi
WiFiClient espClient;
PubSubClient client(espClient);

// Initialize Sensor
BMI270 m5IMU;

uint8_t i2cAddress = BMI2_I2C_SEC_ADDR; // 0x69

// Create Data
struct IMUData{
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
};

void setup(void) {
  // Begin Serial
  Serial.begin(115200);
  
  // Start IMU
  Wire.begin(8, 10, 400000);  // Initialize I2C communication with SDA pin 8 and SCL pin 10 at 400kHz
  m5IMU.beginI2C(i2cAddress);

  // Setup BLE
  setup_ble();

  // Setup WiFi and MQTT
  setup_wifi();
  String name = "M5Capsule-Client-" + String(random(0xffff), HEX);
  client.setServer(mqtt_server, mqtt_port);
  client.connect(name.c_str());
  client.setCallback(callback);
}
 
void loop(void) {
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  // Create Data
  static IMUData data_buf[5];
  static int data_count = 0;

  // Store Data in Buffer
  m5IMU.getSensorData();
  data_buf[data_count].accelX = m5IMU.data.accelX;
  data_buf[data_count].accelY = m5IMU.data.accelY;
  data_buf[data_count].accelZ = m5IMU.data.accelZ;
  data_buf[data_count].gyroX = m5IMU.data.gyroX;
  data_buf[data_count].gyroY = m5IMU.data.gyroY;
  data_buf[data_count].gyroZ = m5IMU.data.gyroZ;

  data_count++;

  if (data_count >= 5) {
    char buf[256]; // Make sure the buffer is large enough
    snprintf(buf, sizeof(buf),
      "{\"axm\":[%.2f,%.2f,%.2f,%.2f,%.2f],"
      "\"aym\":[%.2f,%.2f,%.2f,%.2f,%.2f],"
      "\"azm\":[%.2f,%.2f,%.2f,%.2f,%.2f],"
      "\"gxm\":[%.2f,%.2f,%.2f,%.2f,%.2f],"
      "\"gym\":[%.2f,%.2f,%.2f,%.2f,%.2f],"
      "\"gzm\":[%.2f,%.2f,%.2f,%.2f,%.2f]}",
      data_buf[0].accelX, data_buf[1].accelX, data_buf[2].accelX, data_buf[3].accelX, data_buf[4].accelX,
      data_buf[0].accelY, data_buf[1].accelY, data_buf[2].accelY, data_buf[3].accelY, data_buf[4].accelY,
      data_buf[0].accelZ, data_buf[1].accelZ, data_buf[2].accelZ, data_buf[3].accelZ, data_buf[4].accelZ,
      data_buf[0].gyroX, data_buf[1].gyroX, data_buf[2].gyroX, data_buf[3].gyroX, data_buf[4].gyroX,
      data_buf[0].gyroY, data_buf[1].gyroY, data_buf[2].gyroY, data_buf[3].gyroY, data_buf[4].gyroY,
      data_buf[0].gyroZ, data_buf[1].gyroZ, data_buf[2].gyroZ, data_buf[3].gyroZ, data_buf[4].gyroZ
    );
    Serial.println(buf);

    if (client.publish(topic, buf)) {
      Serial.println("Publish successful!");
      
    } else {
      Serial.println("Publish failed...");
    }
    memset(data_buf, 0, sizeof(data_buf));
    data_count = 0;
  }
  
  delay(100);
}

// Function to Setup BLE
void setup_ble(void) {
  BLEDevice::init("M5Capsule");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

// Function to Setup WiFi
void setup_wifi(void) {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Callback Function for MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Function to Reconnect MQTT
void reconnect(void) {
  String name = "M5Capsule-Client-" + String(random(0xffff), HEX);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(name.c_str())) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}