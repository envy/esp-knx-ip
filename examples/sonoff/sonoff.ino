/*
 * This is an example showing a simple environment sensor based on a DHT11 attached to a Sonoff
 * and KNX Configuration set by code.
 */
#include <esp-knx-ip.h> // Include KNX library
#include <DHT.h>

// WiFi config here
const char* ssid = "ssid";
const char* pass = "pass";

#define ROOT_PREFIX               ""  // [Default ""] This gets prepended to all webserver paths, default is empty string "". Set this to "/knx" if you want the config to be available on http://<ip>/knx

// Common
#define LED_PIN 2 // 13 on Sonoff, 2 on NodeMCU
#define UPDATE_INTERVAL 10000
#define DHTTYPE DHT11
#define DHTPIN  14 // Check connection Pin (14 is for Sonoff Basic)
unsigned long next_change = 0;

float last_temp = 0.0;
float last_hum = 0.0;

// For Basic and S20
#define BTN1_PIN 0
#define CH1_PIN 12

// For 4CH
#define BTN2_PIN 9
#define CH2_PIN 5
#define BTN3_PIN 10
#define CH3_PIN 4
#define BTN4_PIN 14
#define CH4_PIN 15

typedef enum __type_option
{
  SONOFF_TYPE_NONE = 0,
  SONOFF_TYPE_BASIC = 1,
  SONOFF_TYPE_S20 = 2,
  SONOFF_TYPE_4CH = 3,
  SONOFF_TYPE_4CH_PRO = 4,
} type_option_t;

option_entry_t type_options[] = {
  {"Sonoff Basic", SONOFF_TYPE_BASIC},
  {"Sonoff S20", SONOFF_TYPE_S20},
  {"Sonoff 4CH", SONOFF_TYPE_4CH},
  {"Sonoff 4CH Pro", SONOFF_TYPE_4CH_PRO},
  {nullptr, 0}
};

config_id_t hostname_id;
config_id_t type_id;
callback_id_t callback_id;
config_id_t temp_ga, hum_ga;
config_id_t update_rate_id;
config_id_t enable_sending_id;

DHT dht(DHTPIN, DHTTYPE);

typedef struct __sonoff_channel
{
  int pin;
  int btn_pin;
  config_id_t status_ga_id;
  bool state;
  bool last_btn_state;
} sonoff_channel_t;

sonoff_channel_t channels[] = {
  {CH1_PIN, BTN1_PIN, 0, false, false},
  {CH2_PIN, BTN2_PIN, 0, false, false},
  {CH3_PIN, BTN3_PIN, 0, false, false},
  {CH4_PIN, BTN4_PIN, 0, false, false},
};

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);
  pinMode(BTN4_PIN, INPUT_PULLUP);
  pinMode(CH1_PIN, OUTPUT);
  pinMode(CH2_PIN, OUTPUT);
  pinMode(CH3_PIN, OUTPUT);
  pinMode(CH4_PIN, OUTPUT);
  Serial.begin(115200);

  // Register the config options
  hostname_id = knx.config_register_string("Hostname", 20, String("sonoff_DHT11"));
  type_id = knx.config_register_options("Type", type_options, SONOFF_TYPE_BASIC);
  enable_sending_id = knx.config_register_bool("Send on update", true);
  update_rate_id = knx.config_register_int("Update rate (ms)", UPDATE_INTERVAL);

  // Set Physical KNX Address of ESP KNX
  knx.config_set_physical_addr(1,1,1);

  // Register and Set Group Addresses to Write to
  channels[0].status_ga_id = knx.config_register_ga("Channel 1 Status GA");
  knx.config_set_ga(channels[0].status_ga_id,2,2,1);
  channels[1].status_ga_id = knx.config_register_ga("Channel 2 Status GA", is_4ch_or_4ch_pro);
  channels[2].status_ga_id = knx.config_register_ga("Channel 3 Status GA", is_4ch_or_4ch_pro);
  channels[3].status_ga_id = knx.config_register_ga("Channel 4 Status GA", is_4ch_or_4ch_pro);
  temp_ga = knx.config_register_ga("Temperature", show_periodic_options);
  knx.config_set_ga(temp_ga,4,1,1);
  hum_ga = knx.config_register_ga("Humidity", show_periodic_options);
  knx.config_set_ga(hum_ga,4,1,2);

  // Register and set Group Addresses to Receive data from and execute callbacks
  callback_id = knx.callback_register("Channel 1", channel_cb, &channels[0]);
  knx.callback_assign(callback_id,2,2,1);
  knx.callback_register("Channel 2", channel_cb, &channels[1], is_4ch_or_4ch_pro);
  knx.callback_register("Channel 3", channel_cb, &channels[2], is_4ch_or_4ch_pro);
  knx.callback_register("Channel 4", channel_cb, &channels[3], is_4ch_or_4ch_pro);
  knx.callback_register("Read Temperature", temp_cb);
  knx.callback_register("Read Humidity", hum_cb);

  // Register data to be shown on the webserver
  knx.feedback_register_bool("Channel 1 is on", &(channels[0].state));
  knx.feedback_register_action("Toogle channel 1", toggle_chan, &channels[0]);
  knx.feedback_register_bool("Channel 2 is on", &(channels[1].state), is_4ch_or_4ch_pro);
  knx.feedback_register_action("Toogle channel 2", toggle_chan, &channels[1], is_4ch_or_4ch_pro);
  knx.feedback_register_bool("Channel 3 is on", &(channels[2].state), is_4ch_or_4ch_pro);
  knx.feedback_register_action("Toogle channel 3", toggle_chan, &channels[2], is_4ch_or_4ch_pro);
  knx.feedback_register_bool("Channel 4 is on", &(channels[3].state), is_4ch_or_4ch_pro);
  knx.feedback_register_action("Toogle channel 4", toggle_chan, &channels[3], is_4ch_or_4ch_pro);
  knx.feedback_register_float("Temperature (°C)", &last_temp);
  knx.feedback_register_float("Humidity (%)", &last_hum);

  // Load previous values from EEPROM
  knx.load(); // Comment if you don't want to use EEPROM

  // Init sensor
  dht.begin();

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
  //knx.start(nullptr); // Start KNX WITHOUT webserver
  knx.start(); // Start KNX with webserver
  //knx.start(myserver); // Start KNX with a webserver already running on 'myserver'
                     // On this case you might want to change ROOT_PREFIX to
                     // #define ROOT_PREFIX   "/knx"

  Serial.println();
  Serial.println("Connected to wifi");
  Serial.println(WiFi.localIP());
}

void loop()
{
  knx.loop();  // Process knx events

  unsigned long now = millis();

  if (next_change < now)
  {
    next_change = now + knx.config_get_int(update_rate_id);

    float last_temp = dht.readTemperature();
    float last_hum = dht.readHumidity();
    if ( isnan (last_temp) )
    {
        last_temp = -99.0;
    }
    if ( isnan(last_hum) )
    {
        last_hum = -99.0;
    }
    Serial.print("T: ");
    Serial.print(last_temp);
    Serial.print("°C  H: ");
    Serial.print(last_hum);
    Serial.println("%");

    if (knx.config_get_bool(enable_sending_id))
    {
      knx.write2ByteFloat(knx.config_get_ga(temp_ga), last_temp);
      knx.write2ByteFloat(knx.config_get_ga(hum_ga), last_hum);
    }
  }

  // Check local buttons
  check_button(&channels[0]);
  if (is_4ch_or_4ch_pro())
  {
    check_button(&channels[1]);
    check_button(&channels[2]);
    check_button(&channels[3]);
  }

  delay(50);
}

bool is_basic_or_s20()
{
  uint8_t type = knx.config_get_options(type_id);
  return type == SONOFF_TYPE_BASIC || type == SONOFF_TYPE_S20;
}

bool is_4ch_or_4ch_pro()
{
  uint8_t type = knx.config_get_options(type_id);
  return type == SONOFF_TYPE_4CH ||type == SONOFF_TYPE_4CH_PRO;
}

void check_button(sonoff_channel_t *chan)
{
  bool state_now = digitalRead(chan->btn_pin) == HIGH ? true : false;
  if (state_now != chan->last_btn_state && state_now == LOW)
  {
    chan->state = !chan->state;
    digitalWrite(chan->pin, chan->state ? HIGH : LOW);
    digitalWrite(LED_PIN, chan->state ? LOW : HIGH);
    knx.write1Bit(knx.config_get_ga(chan->status_ga_id), chan->state);
  }
  chan->last_btn_state = state_now;
}

void toggle_chan(void *arg)
{
  sonoff_channel_t *chan = (sonoff_channel_t *)arg;
  chan->state = !chan->state;
  digitalWrite(chan->pin, chan->state ? HIGH : LOW);
  digitalWrite(LED_PIN, chan->state ? LOW : HIGH);
  knx.write1Bit(knx.config_get_ga(chan->status_ga_id), chan->state);
}

void channel_cb(message_t const &msg, void *arg)
{
  sonoff_channel_t *chan = (sonoff_channel_t *)arg;
  switch (msg.ct)
  {
    case KNX_CT_WRITE:
      chan->state = msg.data[0];
      Serial.println(chan->state ? "Toggle on" : "Toggle off");
      digitalWrite(chan->pin, chan->state ? HIGH : LOW);
      digitalWrite(LED_PIN, chan->state ? LOW : HIGH);
      knx.write1Bit(knx.config_get_ga(chan->status_ga_id), chan->state);
      break;
     case KNX_CT_READ:
      knx.answer1Bit(msg.received_on, chan->state);
  }
}

bool show_periodic_options()
{
  return knx.config_get_bool(enable_sending_id);
}

void temp_cb(message_t const &msg, void *arg)
{
  switch (msg.ct)
  {
    case KNX_CT_READ:
    {
      knx.answer2ByteFloat(msg.received_on, last_temp);
      break;
    }
  }
}

void hum_cb(message_t const &msg, void *arg)
{
  switch (msg.ct)
  {
    case KNX_CT_READ:
    {
      knx.answer2ByteFloat(msg.received_on, last_hum);
      break;
    }
  }
}
