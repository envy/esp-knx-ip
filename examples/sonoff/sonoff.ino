#include <esp-knx-ip.h>

// WiFi config here
const char* ssid = "ssid";
const char* pass = "pass";

// Common
#define LED_PIN 13

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
config_id_t ch1_status_ga_id, ch2_status_ga_id, ch3_status_ga_id, ch4_status_ga_id;
bool ch1_state = false, ch2_state = false, ch3_state = false, ch4_state = false;

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
  hostname_id = knx.config_register_string("Hostname", 20, String("sonoff"));
  type_id = knx.config_register_options("Type", type_options, SONOFF_TYPE_BASIC);
  
  ch1_status_ga_id = knx.config_register_ga("Channel 1 Status GA");
  ch2_status_ga_id = knx.config_register_ga("Channel 2 Status GA", is_4ch_or_4ch_pro);
  ch3_status_ga_id = knx.config_register_ga("Channel 3 Status GA", is_4ch_or_4ch_pro);
  ch4_status_ga_id = knx.config_register_ga("Channel 4 Status GA", is_4ch_or_4ch_pro);

  knx.register_callback("Channel 1", channel1_cb);
  knx.register_callback("Channel 2", channel2_cb, is_4ch_or_4ch_pro);
  knx.register_callback("Channel 3", channel3_cb, is_4ch_or_4ch_pro);
  knx.register_callback("Channel 4", channel4_cb, is_4ch_or_4ch_pro);

  knx.load();

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

void loop()
{
  knx.loop();
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

void channel1_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_WRITE:
      ch1_state = data[0];
      Serial.println(ch1_state ? "Toggle on" : "Toggle off");
      digitalWrite(CH1_PIN, ch1_state ? HIGH : LOW);
      knx.write1Bit(knx.config_get_ga(ch1_status_ga_id), ch1_state);
      break;
     case KNX_CT_READ:
      knx.answer1Bit(received_on, ch1_state);
  }
}

void channel2_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_WRITE:
      ch2_state = data[0];
      Serial.println(ch2_state ? "Toggle on" : "Toggle off");
      digitalWrite(CH2_PIN, ch2_state ? HIGH : LOW);
      knx.write1Bit(knx.config_get_ga(ch2_status_ga_id), ch2_state);
      break;
     case KNX_CT_READ:
      knx.answer1Bit(received_on, ch2_state);
  }
}

void channel3_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_WRITE:
      ch3_state = data[0];
      Serial.println(ch3_state ? "Toggle on" : "Toggle off");
      digitalWrite(CH3_PIN, ch3_state ? HIGH : LOW);
      knx.write1Bit(knx.config_get_ga(ch3_status_ga_id), ch3_state);
      break;
     case KNX_CT_READ:
      knx.answer1Bit(received_on, ch3_state);
  }
}

void channel4_cb(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data)
{
  switch (ct)
  {
    case KNX_CT_WRITE:
      ch4_state = data[0];
      Serial.println(ch4_state ? "Toggle on" : "Toggle off");
      digitalWrite(CH4_PIN, ch4_state ? HIGH : LOW);
      knx.write1Bit(knx.config_get_ga(ch4_status_ga_id), ch4_state);
      break;
     case KNX_CT_READ:
      knx.answer1Bit(received_on, ch4_state);
  }
}

