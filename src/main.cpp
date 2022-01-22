#include <Arduino.h>
#include <radon_eye.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

// use the MAC address for your sensor
RadonEye radon_sensor("F6:34:XX:XX:XX:XX");

// Replace the next variables with your WLAN SSID/Password combination
const char* ssid = "unser-smartes-zuhause.de";
const char* password = "ru********";

// Add your MQTT Broker IP address, example:
const char* mqtt_server = "192.168.178.xxx";
const char* mqttUser = "loxxxxxx";
const char* mqttPassword = "Lwkxxxxxxxxxxxx";

const char* mqttTopic_Now = "radon1/now";
const char* mqttTopic_Day = "radon1/day";
const char* mqttTopic_Month = "radon1/month";

// Loxone Miniserver

const char * udpAddress = "192.168.178.xxx";
const int udpPort = 44444;








WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP udp;

long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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

void setup() {
  Serial.begin(9600);
  
  // returns false if it can't connect to the RadonEye
  while(!radon_sensor.setup()) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("RadonEye connected");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
 

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Radon_RD200",mqttUser,mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop() {
  // must call this to read current values from the RadonEye
  // returns false if it fails

  if (!client.connected()) {
    reconnect();
  }
  //client.loop();

  if(!radon_sensor.update()) {
    delay(1000);
    return;
  }

  float now_converted = radon_sensor.radon_now() * 37;
  float day_converted = radon_sensor.radon_day() * 37;
  float month_converted = radon_sensor.radon_month() * 37;

  Serial.printf("Jetzt %.2f Tag %.2f Monat %.2f\n",
		now_converted,
		day_converted,
		month_converted);

  // MQTT Send

    client.publish(mqttTopic_Now, String(now_converted,2).c_str());
    client.publish(mqttTopic_Day,String(day_converted,2).c_str());
    client.publish(mqttTopic_Month, String(month_converted,2).c_str());
    
  // Loxone Miniserver Send

  char buffer_now[20];
  sprintf(buffer_now, "Now=%f", now_converted);

  char buffer_day[20];
  sprintf(buffer_day, "Day=%f", day_converted);

  char buffer_month[20];
  sprintf(buffer_month, "Month=%f", month_converted);


  //This initializes udp and transfer buffer
  udp.beginPacket(udpAddress, udpPort);
  udp.print(buffer_now);
  udp.endPacket();

  udp.beginPacket(udpAddress, udpPort);
  udp.print(buffer_day);
  udp.endPacket();

  udp.beginPacket(udpAddress, udpPort);
  udp.print(buffer_month);
  udp.endPacket();

  // one reading every 10 minutes
  delay(10*60*1000);
}