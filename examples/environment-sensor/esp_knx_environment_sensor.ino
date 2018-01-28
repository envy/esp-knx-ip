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
config_id_t enable_sending_id;

callback_id_t temp_cb_id, hum_cb_id, pres_cb_id;

Adafruit_BME280 bme;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

  // Register the group addresses where to send information to and the other config
  hostname_id = knx.config_register_string("Hostname", 20, String("env"));
  enable_sending_id = knx.config_register_bool("Enable periodic sending", true);
  temp_rate_id = knx.config_register_int("Send rate (ms)", UPDATE_INTERVAL, show_periodic_options);
  temp_ga = knx.config_register_ga("Temperature", show_periodic_options);
  hum_ga = knx.config_register_ga("Humidity", show_periodic_options);
  pres_ga = knx.config_register_ga("Pressure", show_periodic_options);

  temp_cb_id = knx.register_callback("Read Temperature", temp_cb);
  hum_cb_id = knx.register_callback("Read Humidity", temp_cb);
  pres_cb_id = knx.register_callback("Read Pressure", pres_cb);

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

  if (knx.config_get_bool(enable_sending_id) && next_change < now)
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

// This is a enable condition function that is checked by the webui to figure out if some config options should be hidden
bool show_periodic_options()
{
  return knx.config_get_bool(enable_sending_id);
}

void temp_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_READ:
    {
      knx.answer2ByteFloat(received_on, bme.readTemperature());
      break;
    }
  }
}

void hum_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_READ:
    {
      knx.answer2ByteFloat(received_on, bme.readHumidity());
      break;
    }
  }
}

void pres_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_READ:
    {
      knx.answer2ByteFloat(received_on, bme.readPressure()/100.0f);
      break;
    }
  }
}