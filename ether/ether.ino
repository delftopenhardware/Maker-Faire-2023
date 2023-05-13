#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Servo.h>

// Replace with your SSID and password details
char ssid[] = "OpenHardware";
char pass[] = "makerfaire2023";

WiFiClientSecure client;
Servo myservo;  // create servo object to control a servo

// Just the base of the URL you want to connect to
#define TEST_HOST "api.coingecko.com"

float ethereum_eur_old = 0;
float ethereum_eur = 0;
unsigned long lastConnectionTime = 3 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval =  * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)

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
  client.setInsecure();
}

void loop() {
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
  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(TEST_HOST, 443)) {
    Serial.println(F("Connection failed"));
    return;
  }

  // give the esp a breather
  yield();

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  client.print("/api/v3/simple/price?ids=ethereum%2Cbitcoin&vs_currencies=usd%2Ceur");  // %2C == ,
  client.println(F(" HTTP/1.1"));

  //Headers
  client.print(F("Host: "));
  client.println(TEST_HOST);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }
  //delay(100);
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

  // This is probably not needed for most, but I had issues
  // with the Tindie api where sometimes there were random
  // characters coming back before the body of the response.
  // This will cause no hard to leave it in
  // peek() will look at the character, but not take it off the queue
  while (client.available() && client.peek() != '{') {
    char c = 0;
    client.readBytes(&c, 1);
    Serial.print(c);
    Serial.println("BAD");
  }

  //  // While the client is still availble read each
  //  // byte and print to the serial monitor
  //  while (client.available()) {
  //    char c = 0;
  //    client.readBytes(&c, 1);
  //    Serial.print(c);
  //  }

  //Use the ArduinoJson Assistant to calculate this:

  //StaticJsonDocument<192> doc;
  DynamicJsonDocument doc(192);  //For ESP32/ESP8266 you'll mainly use dynamic.

  DeserializationError error = deserializeJson(doc, client);

  if (!error) {
    float ethereum_usd = doc["ethereum"]["usd"];  // 3961.66
    float ethereum_eur = doc["ethereum"]["eur"];  // 3261.73

    long bitcoin_usd = doc["bitcoin"]["usd"];  // 48924
    long bitcoin_eur = doc["bitcoin"]["eur"];  // 40281

    Serial.print("ethereum_usd: ");
    Serial.println(ethereum_usd);
    Serial.print("ethereum_eur: ");
    Serial.println(ethereum_eur);

    Serial.print("bitcoin_usd: ");
    Serial.println(bitcoin_usd);
    Serial.print("bitcoin_eur: ");
    Serial.println(bitcoin_eur);

  } else {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  if  (ethereum_eur_old < ethereum_eur){
    diffServoAction(180);
  }  else {
    diffServoAction(0);
  }
  ethereum_eur_old = ethereum_eur;
}

void diffServoAction(float tempNow) {
  int pos;
  Serial.println(int(tempNow));
  myservo.write(int(tempNow));
}
