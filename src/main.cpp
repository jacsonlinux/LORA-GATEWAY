#include <WiFi.h>
#include <WiFiMulti.h>
#include <LoRa.h>
#include <U8g2lib.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define SSID "..."            //nome da sua rede WiFi
#define WIFI_PASSWORD "..."   //senha da sua rede WiFi

#define TOKEN "..."           //token ThingsBoard

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 23
#define LORA_IRQ 26

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16

#define GREEN_LED_PIN 25

#define OFF 0
#define ON 1

U8G2_SSD1306_128X64_NONAME_F_SW_I2C Display(U8G2_R0, OLED_SCL, OLED_SDA, OLED_RST);

const char DEGREE_SYMBOL[] = { 0xB0, '\0' };

int count = 0;

char thingsboardServer[] = "demo.thingsboard.io";

WiFiClient wifiClient;

WiFiMulti wifiMulti;

PubSubClient client(wifiClient);

String rssi = "";

String packet = "";

String temperature = "";

String humidity = "";

void setupDisplay(){
  Display.begin();
  Display.enableUTF8Print();
  Display.setFont(u8g2_font_5x8_tf);
}

void setupWiFi() {
  int i = 0;
  Display.clearBuffer();
  Display.setCursor(0,10);
  Display.print("Connecting Wifi...");
  Display.sendBuffer();
  wifiMulti.addAP(SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED && i <= 5) {
    Display.clearBuffer();
    Display.setCursor(0,10);
    Display.print("Connecting Wifi... " + String(i));
    Display.sendBuffer();
    i = i + 1;
    delay(500);
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Display.clearBuffer();
    Display.setCursor(0,10);
    Display.print("WiFi connected falied!");
    Display.sendBuffer();
  } else {
    Display.clearBuffer();
    Display.setCursor(0,10);
    Display.print("WiFi connected!");
    Display.sendBuffer();
  }
}

void setupLora() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_IRQ);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa falied!");
    while (1);
  }
  LoRa.setSpreadingFactor(12);
}

void reconnect() {
  while (!client.connected()) {
    if ( wifiMulti.run() != WL_CONNECTED) {
      WiFi.begin(SSID, WIFI_PASSWORD);
      while (wifiMulti.run() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if ( client.connect("TTGOLORA", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      delay( 5000 );
    }
  }
}

void displayInfo() {
  count = count + 1;
  Display.clearBuffer();
  Display.setCursor(0,10); Display.print("LoRa RECEIVE " + String(count));
  Display.setCursor(0,30); Display.print("TEMP: " + temperature + DEGREE_SYMBOL + "C");
  Display.setCursor(0,40); Display.print("HUMIDITY: " + humidity + "%");
  Display.setCursor(0,50); Display.print("RSSI " + rssi);
  Display.setCursor(64,62); Display.print("JacsonLinux ");
  Display.sendBuffer();
}

void publishData() {
  digitalWrite(GREEN_LED_PIN, ON);
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";

  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "v1/devices/me/telemetry", attributes );
  Serial.println(attributes);
  delay(500);
  digitalWrite(GREEN_LED_PIN, OFF);
}

void mqttConnect() {
  client.setServer( thingsboardServer, 1883 );
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  setupDisplay();
  setupWiFi();
  if (wifiMulti.run() == WL_CONNECTED) {
    mqttConnect();
  }
  setupLora();
  Display.clearBuffer();
  Display.setCursor(0,10); Display.print("Waiting for data...");
  Display.setCursor(64,62); Display.print("JacsonLinux ");
  Display.sendBuffer();
  pinMode(GREEN_LED_PIN, OUTPUT);
}

void loop() {

    int packetSize = LoRa.parsePacket();

    if (packetSize) {
      packet = "";
      while (LoRa.available()) {
        packet += (char)LoRa.read();
      }
      rssi = LoRa.packetRssi();
      humidity = packet.substring(0,5);
      temperature = packet.substring(5,10);
      publishData();
      displayInfo();
    }

  if (!client.loop()) {
    reconnect();
  }

}
