/*
 * This is an example showing a simple environment sensor based on a BME280 attached via I2C.
 * This sketch was tested on a WeMos D1 mini
 */

#include <Adafruit_BME280.h>
#include <esp-knx-ip.h>

// WiFi config here
const char* ssid = "myssid";
const char* pass = "mypassword";

#define LED_PIN D4
#define UPDATE_INTERVAL 10000 // This is the default send interval

unsigned long next_change = 0;

config_id_t temp_ga, hum_ga, pres_ga;
config_id_t hostname_id;
config_id_t temp_rate_id;

Adafruit_BME280 bme;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

  // Register the group addresses where to send information to and the other config
  temp_ga = knx.config_register_ga("Temperature");
  hum_ga = knx.config_register_ga("Humidity");
  pres_ga = knx.config_register_ga("Pressure");
  hostname_id = knx.config_register_string("Hostname", 20, String("env"));
  temp_rate_id = knx.config_register_int("Send rate (ms)", UPDATE_INTERVAL);

  // Load previous values from EEPROM
  knx.load();

  // Init sensor
  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }

  // Init WiFi
  WiFi.hostname(knx.config_get_string(hostname_id));
  WiFi.begin(ssid, pass);

  Serial.println("");
  Serial.print("[Connecting]");
  Serial.print(ssid);

  digitalWrite(LED_PIN, LOW);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, LOW);
  }
  digitalWrite(LED_PIN, HIGH);

  // Start knx
  knx.start();

  Serial.println();
  Serial.println("Connected to wifi");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Process knx events, e.g. webserver
  knx.loop();
  
  unsigned long now = millis();

  if (next_change < now)
  {
    next_change = now + knx.config_get_int(temp_rate_id);
    Serial.print("T: ");
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure()/100.0f;
    Serial.print(temp);
    Serial.print("°C H: ");
    Serial.print(hum);
    Serial.print("% P: ");
    Serial.print(pres);
    Serial.println("hPa");
    knx.write2ByteFloat(knx.config_get_ga(temp_ga), temp);
    knx.write2ByteFloat(knx.config_get_ga(hum_ga), hum);
    knx.write2ByteFloat(knx.config_get_ga(pres_ga), pres);
  }

  delay(50);
}
