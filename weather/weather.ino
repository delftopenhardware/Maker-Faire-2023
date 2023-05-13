#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>

// Replace with your SSID and password details
char ssid[] = "OpenHardware";
char pass[] = "makerfaire2023";

WiFiClient client;
Servo myservo;  // create servo object to control a servo

// Open Weather Map API server name
const char server[] = "api.openweathermap.org";

// Replace the next line to match your city and 2 letter country code
String nameOfCity = "Delft,NL";
// How your nameOfCity variable would look like for Lagos on Nigeria
//String nameOfCity = "Lagos,NG";

// Replace the next line with your API Key
String apiKey = "6aaa6b19c67b4f3318e471fe687d8ccc";

unsigned long lastConnectionTime = 3 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval = 3 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)

void setup() {
  myservo.attach(2, 500, 2500);  // attaches the servo on GIO2 to the servo object
  Serial.begin(9600);

  WiFi.begin(ssid, pass);
  Serial.println("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  printWiFiStatus();
}

void loop() {
  //OWM requires 10mins between request intervals
  //check if 10mins has passed then conect again and pull
  //Serial.println("loop");
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    makehttpRequest();
  }
}


// print Wifi status
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void makehttpRequest() {
  if (client.connect(server, 80)) {
    // Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    if (client.println() == 0) {
      Serial.println(F("Failed to send request"));
      return;
    }

    // Check HTTP status
    char status[32] = { 0 };
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }


    StaticJsonDocument<112> filter;

    JsonObject filter_list_0 = filter["list"].createNestedObject();
    filter_list_0["main"]["temp"] = true;
    filter_list_0["weather"][0]["description"] = true;

    DynamicJsonDocument doc(6144);

    DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    float list_item_main_temp;
    for (JsonObject list_item : doc["list"].as<JsonArray>()) {

      JsonObject filter_list_0 = filter["list"].createNestedObject();
      filter_list_0["main"]["temp"] = true;
      list_item_main_temp = list_item["main"]["temp"];                                       // 3.95, 3.2, 3.25, 0.66, -0.84, -0.33, 2.36, ...
      const char* list_item_weather_0_description = list_item["weather"][0]["description"];  // "light rain", ...
      // Serial.println(list_item_main_temp);
      Serial.println(list_item_weather_0_description);
    }
    diffServoAction(list_item_main_temp);

    // Disconnect
    client.stop();
  }
}

void diffServoAction(float tempNow) {
  int pos;
  Serial.println(int(tempNow));
  myservo.write(180);
  delay(2000);
  myservo.write(0);
  delay(2000);
  // for (pos = 0; pos <= 180; pos += 1) {  // goes from 0 degrees to 180 degrees
  //   // in steps of 1 degree
  //   myservo.write(pos);  // tell servo to go to position in variable 'pos'
  //   delay(1);           // waits 15ms for the servo to reach the position
  // }
  // for (pos = 180; pos >= 0; pos -= 1) {  // goes from 180 degrees to 0 degrees
  //   myservo.write(pos);                  // tell servo to go to position in variable 'pos'
  //   delay(1);                           // waits 15ms for the servo to reach the position
  // }
  pos = map(int(tempNow), -10, 45, 180, 0);
  Serial.println(pos);
  myservo.write(pos);
}
