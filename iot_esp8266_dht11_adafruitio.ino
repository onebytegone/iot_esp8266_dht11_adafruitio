/**
 * ESP8266 temp/humidity IOT device for io.adafruit.com
 * Repo: https://github.com/onebytegone/iot_esp8266_dht11_adafruitio
 * Author: Ethan Smith <ethan@onebytegone.com>
 *
 * NOTE: Please see `config.h` for io.adafruit.com and WiFi config
 */

#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

void setup() {
   Serial.begin(115200);
   Serial.println();

   Serial.print("connecting to ");
   Serial.println(NETWORK_SSID);
   WiFi.mode(WIFI_STA);
   WiFi.begin(NETWORK_SSID, NETWORK_PSK);
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());

   dht.begin();
   delay(2000); // Wait for DHT sensor to initialize
}

void loop() {
   float humidity = dht.readHumidity();
   float temperature = dht.readTemperature(true); // (isFahrenheit = true)

   if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor, will try again in 2 seconds");
      delay(2000);
      return;
   }

   transmitValue(IO_HUMIDITY_FEED, String(humidity));
   transmitValue(IO_TEMPERATURE_FEED, String(temperature));

   delay((60 * 1000) / REPORTS_PER_MIN);
}

void transmitValue(String feed, String value) {
   String path = "";
   BearSSL::WiFiClientSecure secureClient;
   HTTPClient https;
   int didBeginSuccessfully, httpStatusCode;

   path += "/api/v2/";
   path += IO_USERNAME;
   path += "/feeds/";
   path += feed;
   path += "/data";

   secureClient.setInsecure();
   didBeginSuccessfully = https.begin(secureClient, "io.adafruit.com", 443, path, true);

   if (!didBeginSuccessfully) {
      Serial.print("Failed to begin connection to ");
      Serial.print(path);
      return;
   }

   https.addHeader("Content-Type", "application/x-www-form-urlencoded");
   https.addHeader("X-AIO-Key", IO_KEY);

   httpStatusCode = https.POST("value=" + value);
   Serial.print("POST status: ");
   Serial.print(httpStatusCode);
   Serial.print(" (");
   Serial.print(feed);
   Serial.print(", ");
   Serial.print(value);
   Serial.println(")");

   if (httpStatusCode != 200) {
      Serial.print("Did not get 200 response from ");
      Serial.println(path);
      Serial.println(https.getString());
   }

   https.end();
   secureClient.stop();
}
