/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: Not yet decided on one...
 */

#ifndef ESP_KNX_IP_H
#define ESP_KNX_IP_H

#include "Arduino.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WifiUDP.h>
#include <ESP8266WebServer.h>

#include "DPT.h"

/**
 * CONFIG
 * All MAX_ values must not exceed 255 and must not be negative!
 */
#define MAX_GA_CALLBACKS 10 // Maximum number of group address callbacks that can be stored
#define MAX_CALLBACKS 10 // Maximum number of callbacks that can be stored
#define MAX_GAS 10 // Maximum number of configurable GAs that can be stored
#define MAX_CONFIGS 10
#define MAX_CONFIG_SPACE 100 // Maximum number of bytes that can be stored for custom config

#define MULTICAST_PORT 3671 // Default KNX/IP port is 3671
#define MULTICAST_IP IPAddress(224, 0, 23, 12) // Default KNX/IP ip is 224.0.23.12

// Uncomment to enable printing out debug messages.
//#define ESP_KNX_DEBUG
/**
 * END CONFIG
 */

#define EEPROM_MAGIC (0xDEADBEEF00000000 + (MAX_CONFIG_SPACE << 24) + (MAX_GA_CALLBACKS << 16) + (MAX_CALLBACKS << 8) + (MAX_GAS))

// Define where debug output will be printed.
#ifndef DEBUG_PRINTER
#define DEBUG_PRINTER Serial
#endif

// Setup debug printing macros.
#ifdef ESP_KNX_DEBUG
  #define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
  #define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif

/**
 * Different service types, we are mainly interested in KNX_ST_ROUTING_INDICATION
 */
typedef enum __knx_service_type
{
  KNX_ST_SEARCH_REQUEST           = 0x0201,
  KNX_ST_SEARCH_RESPONSE          = 0x0202,
  KNX_ST_DESCRIPTION_REQUEST      = 0x0203,
  KNX_ST_DESCRIPTION_RESPONSE     = 0x0204,
  KNX_ST_CONNECT_REQUEST          = 0x0205,
  KNX_ST_CONNECT_RESPONSE         = 0x0206,
  KNX_ST_CONNECTIONSTATE_REQUEST  = 0x0207,
  KNX_ST_CONNECTIONSTATE_RESPONSE = 0x0208,
  KNX_ST_DISCONNECT_REQUEST       = 0x0209,
  KNX_ST_DISCONNECT_RESPONSE      = 0x020A,

  KNX_ST_DEVICE_CONFIGURATION_REQUEST = 0x0310,
  KNX_ST_DEVICE_CONFIGURATION_ACK     = 0x0311,

  KNX_ST_TUNNELING_REQUEST = 0x0420,
  KNX_ST_TUNNELING_ACK     = 0x0421,

  KNX_ST_ROUTING_INDICATION   = 0x0530,
  KNX_ST_ROUTING_LOST_MESSAGE = 0x0531,
  KNX_ST_ROUTING_BUSY         = 0x0532,

//  KNX_ST_RLOG_START = 0x0600,
//  KNX_ST_RLOG_END   = 0x06FF,

  KNX_ST_REMOTE_DIAGNOSTIC_REQUEST          = 0x0740,
  KNX_ST_REMOTE_DIAGNOSTIC_RESPONSE         = 0x0741,
  KNX_ST_REMOTE_BASIC_CONFIGURATION_REQUEST = 0x0742,
  KNX_ST_REMOTE_RESET_REQUEST               = 0x0743,

//  KNX_ST_OBJSRV_START = 0x0800,
//  KNX_ST_OBJSRV_END   = 0x08FF,
} knx_service_type_t;

/**
 * Differnt command types, first three are of main interest
 */
typedef enum __knx_command_type
{
  KNX_CT_READ                     = 0x00,
  KNX_CT_ANSWER                   = 0x01,
  KNX_CT_WRITE                    = 0x02,
  KNX_CT_INDIVIDUAL_ADDR_WRITE    = 0x03,
  KNX_CT_INDIVIDUAL_ADDR_REQUEST  = 0x04,
  KNX_CT_INDIVIDUAL_ADDR_RESPONSE = 0x05,
  KNX_CT_ADC_READ                 = 0x06,
  KNX_CT_ADC_ANSWER               = 0x07,
  KNX_CT_MEM_READ                 = 0x08,
  KNX_CT_MEM_ANSWER               = 0x09,
  KNX_CT_MEM_WRITE                = 0x0A,
//KNX_CT_UNKNOWN                  = 0x0B,
  KNX_CT_MASK_VERSION_READ        = 0x0C,
  KNX_CT_MASK_VERSION_RESPONSE    = 0x0D,
  KNX_CT_RESTART                  = 0x0E,
  KNX_CT_ESCAPE                   = 0x0F,
} knx_command_type_t;

/**
 * cEMI message types, mainly KNX_MT_L_DATA_IND is interesting
 */
typedef enum __knx_cemi_msg_type
{
  KNX_MT_L_DATA_REQ = 0x11,
  KNX_MT_L_DATA_IND = 0x29,
  KNX_MT_L_DATA_CON = 0x2E,
} knx_cemi_msg_type_t;

/**
 * TCPI communication type
 */
typedef enum __knx_communication_type {
  KNX_COT_UDP = 0x00, // Unnumbered Data Packet
  KNX_COT_NDP = 0x01, // Numbered Data Packet
  KNX_COT_UCD = 0x02, // Unnumbered Control Data
  KNX_COT_NCD = 0x03, // Numbered Control Data
} knx_communication_type_t;

/**
 * KNX/IP header
 */
typedef struct __knx_ip_pkt
{
  uint8_t header_len; // Should always be 0x06
  uint8_t protocol_version; // Should be version 1.0, transmitted as 0x10
  uint16_t service_type; // See knx_service_type_t
  union
  {
    struct {
      uint8_t first_byte;
      uint8_t second_byte;
    } bytes;
    uint16_t len;
  } total_len; // header_len + rest of pkt. This is a bit weird as the spec says this:  If the total number of bytes transmitted is greater than 252 bytes, the first “Total Length” byte is set to FF (255). Only in this case the second byte includes additional length information
  uint8_t pkt_data[]; // This is of type cemi_msg_t
} knx_ip_pkt_t;

typedef struct __cemi_addi
{
  uint8_t type_id;
  uint8_t len;
  uint8_t data[];
} cemi_addi_t;

typedef union __address
{
  uint16_t value;
  struct
  {
    uint8_t high;
    uint8_t low;
  } bytes;
  uint8_t array[2];
} address_t;

typedef struct __cemi_service
{
  union
  {
    struct
    {
      // Struct is reversed due to bit order
      uint8_t confirm:1; // 0 = no error, 1 = error
      uint8_t ack:1; // 0 = no ack, 1 = ack
      uint8_t priority:2; // 0 = system, 1 = high, 2 = urgent/alarm, 3 = normal
      uint8_t system_broadcast:1; // 0 = system broadcast, 1 = broadcast
      uint8_t repeat:1; // 0 = repeat on error, 1 = do not repeat
      uint8_t reserved:1; // always zero
      uint8_t frame_type:1; // 0 = extended, 1 = standard
    } bits;
    uint8_t byte;
  } control_1;
  union
  {
    struct
    {
      // Struct is reversed due to bit order
      uint8_t extended_frame_format:4;
      uint8_t hop_count:3;
      uint8_t dest_addr_type:1; // 0 = individual, 1 = group
    } bits;
    uint8_t byte;
  } control_2;
  address_t source;
  address_t destination;
  uint8_t data_len; // length of data, excluding the tpci byte
  struct
  {
    uint8_t apci:2; // If tpci.comm_type == KNX_COT_UCD or KNX_COT_NCD, then this is apparently control data?
    uint8_t tpci_seq_number:4;
    uint8_t tpci_comm_type:2; // See knx_communication_type_t
  } pci;
  uint8_t data[];
} cemi_service_t;

typedef struct __cemi_msg
{
  uint8_t message_code;
  uint8_t additional_info_len;
  union
  {
    cemi_addi_t additional_info[];
    cemi_service_t service_information;
  } data;
} cemi_msg_t;

typedef enum __config_type
{
  CONFIG_TYPE_UNKNOWN,
  CONFIG_TYPE_INT,
  CONFIG_TYPE_STRING,
  CONFIG_TYPE_GA,

} config_type_t;

typedef struct __config
{
  config_type_t type;
  uint8_t offset;
  uint8_t len;
} config_t;

typedef uint8_t callback_id_t;
typedef uint8_t config_id_t;

typedef void (*GACallback)(knx_command_type_t ct, address_t const &received_on, uint8_t data_len, uint8_t *data);

class ESPKNXIP {
  public:
    ESPKNXIP();
    void load();
    void start();
    void start(ESP8266WebServer *srv);
    void loop();

    void save_to_eeprom();
    void restore_from_eeprom();
    int first_free_eeprom_address();

    callback_id_t register_callback(String name, GACallback cb);
    callback_id_t register_callback(const char *name, GACallback cb);
    int register_GA(String name);
    address_t get_GA(int id);

    config_id_t register_config_string(String name, uint8_t len, String _default);
    config_id_t register_config_int(String name, int32_t _default);
    config_id_t register_config_ga(String name);

    String get_config_string(config_id_t id);
    int32_t get_config_int(config_id_t id);
    address_t get_config_ga(config_id_t id);

    void send(address_t const &receiver, knx_command_type_t ct, uint8_t data_len, uint8_t *data);

    void sendBit(address_t const &receiver, knx_command_type_t ct, uint8_t bit);
    void send1ByteInt(address_t const &receiver, knx_command_type_t ct, int8_t val);
    void send2ByteInt(address_t const &receiver, knx_command_type_t ct, int16_t val);
    void send2ByteFloat(address_t const &receiver, knx_command_type_t ct, float val);
    void send3ByteTime(address_t const &receiver, knx_command_type_t ct, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds);
    void send3ByteDate(address_t const &receiver, knx_command_type_t ct, uint8_t day, uint8_t month, uint8_t year);
    void send3ByteColor(address_t const &receiver, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue);
    void send4ByteFloat(address_t const &receiver, knx_command_type_t ct, float val);

    static address_t GA_to_address(uint8_t area, uint8_t line, uint8_t member)
    {
      address_t tmp;
      tmp.bytes.high = (area << 3) | line;
      tmp.bytes.low = member;
      return tmp;
    }
  private:
    void __start();
    void __loop_knx();

    // Webserver functions
    void __loop_webserver();
    void __handle_root();
    void __handle_register();
    void __handle_delete();
    void __handle_set(bool phys);
    void __handle_eeprom();
    void __handle_config();

    void __config_set_string(config_id_t id, String val);
    void __config_set_int(config_id_t id, int32_t val);
    void __config_set_ga(config_id_t id, address_t const &val);

    ESP8266WebServer *server;
    address_t physaddr;
    WiFiUDP udp;

    uint8_t registered_ga_callbacks;
    address_t ga_callback_addrs[MAX_GA_CALLBACKS];
    callback_id_t ga_callbacks[MAX_GA_CALLBACKS];

    callback_id_t registered_callbacks;
    GACallback callbacks[MAX_CALLBACKS];
    String callback_names[MAX_CALLBACKS];

    uint8_t registered_gas;
    address_t gas[MAX_GAS];
    String ga_names[MAX_GAS];

    config_id_t registered_configs;
    uint8_t custom_config_data[MAX_CONFIG_SPACE];
    String custom_config_names[MAX_CONFIGS];
    config_t custom_configs[MAX_CONFIGS];

    uint16_t ntohs(uint16_t);
    int register_GA_callback(uint8_t area, uint8_t line, uint8_t member, callback_id_t cb);
    int delete_GA_callback(int id);
};

// Global "singleton" object
extern ESPKNXIP knx;

#endif
