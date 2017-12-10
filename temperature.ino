#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// *** GPIO pin number for DS18B20
const int TEMP_PIN = 16;

// BUILT IN LED PIN
const int LED_PIN = 5;

// Wifi Configuration
const char* ssid     = "SSID";
const char* password = "PASSWORD";

DeviceAddress insideThermometer;

OneWire oneWire(TEMP_PIN);

DallasTemperature sensors(&oneWire);

WiFiServer server(80);

void printLine()
{
  Serial.println();
  for (int i = 0; i < 30; i++)
    Serial.print("-");
  Serial.println();
}

void blinkLed(int ledToBlink) {
  int ledState = 0;
  // Blink LED while we're connecting:
  digitalWrite(ledToBlink, ledState);
  ledState = (ledState + 1) % 2; // Flip ledState
  delay(250);
}

void connectToWiFi(const char * ssid, const char * pwd)
{
  printLine();
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED)
  {
    blinkLed(LED_PIN);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, HIGH);
  printLine();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void initOneWireDevices() {
  Serial.println("Locating Devices......");
  // Must be called before search()
  oneWire.reset_search();
  // assigns the first address found to insideThermometer
  if (!oneWire.search(insideThermometer)) {
    Serial.println("Unable to find address for insideThermometer");
  }
  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  // set the resolution to 9 bit
  sensors.setResolution(insideThermometer, 9);
  Serial.print("Device 0 Resolution: ");
  Serial.println(sensors.getResolution(insideThermometer), DEC);
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  printLine();
}

float getTemp() {
  Serial.println("Requesting temperature...");
  // Send the command to get temperatures
  sensors.requestTemperatures();
  float tempc = sensors.getTempCByIndex(0);
}

void setup()
{
  Serial.begin(115200);
  // set the LED pin mode
  pinMode(LED_PIN, OUTPUT);
  delay(10);

  connectToWiFi(ssid, password);
  sensors.begin();
  server.begin();
  initOneWireDevices();
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client");
    // make a String to hold incoming data from the client
    String currentLine = "";
    digitalWrite(LED_PIN, LOW);    

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            float tempc = getTemp();
            Serial.print("Temperature is: ");
            Serial.print(tempc);
            Serial.println("C");
            // the content of the HTTP response follows the header:
            client.println("<!doctype html><html><head><meta charset='utf-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='viewport' content='width=device-width, initial-scale=1'><!-- load MUI --><link href='//cdn.muicss.com/mui-0.9.31/css/mui.min.css' rel='stylesheet' type='text/css' /><script src='//cdn.muicss.com/mui-0.9.31/js/mui.min.js'></script></head>");
            client.println("<body><div class='mui-container'><div class='mui-panel'>");
            client.println("<h1>ESP32</h1>");
            client.print("<div class='mui-container-fluid'>");
            client.print("The current temperature is: ");
            client.print(tempc);
            client.print("C<br />");
            client.print("</div>");
            client.print("</div>");
            client.print("</div>");
            client.println("</body></html>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Client Disconnected.");
    printLine();
  }
}
