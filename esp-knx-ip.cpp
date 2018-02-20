/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: MIT
 */

#include "esp-knx-ip.h"

void ESPKNXIP::__handle_root()
{
  String m = F("<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>");
#if USE_BOOTSTRAP
  m += F("<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>");
  m += F("<style>.input-group-insert > .input-group-text { border-radius: 0; }</style>");
#endif
  m += F("</head><body><div class='container-fluid'>");
  m += F("<h2>ESP KNX</h2>");

  // Feedback

  if (registered_feedbacks > 0)
  {
    m += F("<h4>Feedback</h4>");
    for (feedback_id_t i = 0; i < registered_feedbacks; ++i)
    {
      if (feedbacks[i].cond && !feedbacks[i].cond())
      {
        continue;
      }
      m += F("<div class='row' style='margin-bottom:1em'><div class='col-auto'><div class='input-group'>");
      m += F("<div class='input-group-prepend'><span class='input-group-text'>");
      m += feedbacks[i].name;
      m += F("</span></div>");
      switch (feedbacks[i].type)
      {
        case FEEDBACK_TYPE_INT:
          m += F("<span class='input-group-text'>");
          m += String(*(int32_t *)feedbacks[i].data);
          m += F("</span>");
          break;
        case FEEDBACK_TYPE_FLOAT:
          m += F("<span class='input-group-text'>");
          m += String(*(float *)feedbacks[i].data, feedbacks[i].options.float_options.precision);
          m += F("</span>");
          break;
      }
      m += F("</div></div></div>");
    }
  }

  if (registered_callbacks > 0)
    m += F("<h4>Callbacks</h4>");

  if (registered_callback_assignments > 0)
  {
    for (uint8_t i = 0; i < registered_callback_assignments; ++i)
    {
      if (callbacks[callback_assignments[i].callback_id].cond && !callbacks[callback_assignments[i].callback_id].cond())
      {
        continue;
      }
      m += F("<form action='" __DELETE_PATH "' method='POST'>");
      m += F("<div class='row'><div class='col-auto'><div class='input-group'>");
      m += F("<div class='input-group-prepend'><span class='input-group-text'>");
      m += String((callback_assignments[i].address.bytes.high & 0xF8) >> 3);
      m += F("/");
      m += String(callback_assignments[i].address.bytes.high & 0x07);
      m += F("/");
      m += String(callback_assignments[i].address.bytes.low);
      m += F("</span>");
      m += F("<span class='input-group-text'>");
      m += callbacks[callback_assignments[i].callback_id].name;
      m += F("</span></div>");
      m += F("<input class='form-control' type='hidden' name='id' value='");
      m += i;
      m += F("' /><div class='input-group-append'><button type='submit' class='btn btn-danger'>Delete</button></div>");
      m += F("</div></div></div>");
      m += F("</form>");
    }
  }

  if (registered_callbacks > 0)
  {
    m += F("<form action='" __REGISTER_PATH "' method='POST'>");
    m += F("<div class='row'><div class='col-auto'><div class='input-group'>");
    m += F("<input class='form-control' type='number' name='area' min='0' max='31'/>");
    m += F("<div class='input-group-insert'><span class='input-group-text'>/</span></div>");
    m += F("<input class='form-control' type='number' name='line' min='0' max='7'/>");
    m += F("<div class='input-group-insert'><span class='input-group-text'>/</span></div>");
    m += F("<input class='form-control' type='number' name='member' min='0' max='255'/>");
    m += F("<div class='input-group-insert'><span class='input-group-text'>-&gt;</span></div>");
    m += F("<select class='form-control' name='cb'>");
    for (callback_id_t i = 0; i < registered_callbacks; ++i)
    {
      if (callbacks[i].cond && !callbacks[i].cond())
      {
        continue;
      }
      m += F("<option value=\"");
      m += i;
      m += F("\">");
      m += callbacks[i].name;
      m += F("</option>");
    }
    m += F("</select>");
    m += F("<div class='input-group-append'><button type='submit' class='btn btn-primary'>Set</button></div>");
    m += F("</div></div></div>");
    m += F("</form>");
  }

  m += F("<h4>Configuration</h4>");

  // Physical address
  m += F("<form action='" __PHYS_PATH "' method='POST'>");
  m += F("<div class='row'><div class='col-auto'><div class='input-group'>");
  m += F("<div class='input-group-prepend'><span class='input-group-text'>Physical address</span></div>");
  m += F("<input class='form-control' type='number' name='area' min='0' max='15' value='");
  m += String((physaddr.bytes.high & 0xF0) >> 4);
  m += F("'/>");
  m += F("<div class='input-group-insert'><span class='input-group-text'>.</span></div>");
  m += F("<input class='form-control' type='number' name='line' min='0' max='15' value='");
  m += String(physaddr.bytes.high & 0x0F);
  m += F("'/>");
  m += F("<div class='input-group-insert'><span class='input-group-text'>.</span></div>");
  m += F("<input class='form-control' type='number' name='member' min='0' max='255' value='");
  m += String(physaddr.bytes.low);
  m += F("'/>");
  m += F("<div class='input-group-append'><button type='submit' class='btn btn-primary'>Set</button></div>");
  m += F("</div></div></div>");
  m += F("</form>");

  if (registered_configs > 0)
  {
    for (config_id_t i = 0; i < registered_configs; ++i)
    {
      // Check if this config option has a enable condition and if so check that condition
      if (custom_configs[i].cond && !custom_configs[i].cond())
        continue;

      m += F("<form action='" __CONFIG_PATH "' method='POST'>");
      m += F("<div class='row'><div class='col-auto'><div class='input-group'>");
      m += F("<div class='input-group-prepend'><span class='input-group-text'>");
      m += custom_configs[i].name;
      m += F("</span></div>");

      switch (custom_configs[i].type)
      {
        case CONFIG_TYPE_STRING:
          m += F("<input class='form-control' type='text' name='value' value='");
          m += config_get_string(i);
          m += F("' maxlength='");
          m += custom_configs[i].len - 1; // Subtract \0 byte
          m += F("'/>");
          break;
        case CONFIG_TYPE_INT:
          m += F("<input class='form-control' type='number' name='value' value='");
          m += config_get_int(i);
          m += F("'/>");
          break;
        case CONFIG_TYPE_BOOL:
          m += F("<div class='input-group-insert'><span class='input-group-text'>");
          m += F("<input type='checkbox' name='value' ");
          if (config_get_bool(i))
            m += F("checked ");
          m += F("/>");
          m += F("</span></div>");
          break;
        case CONFIG_TYPE_OPTIONS:
        {
          m += F("<select class='custom-select' name='value'>");
          option_entry_t *cur = custom_configs[i].data.options;
          while (cur->name != nullptr)
          {
            if (config_get_options(i) == cur->value)
            {
              m += F("<option selected value='");
            }
            else
            {
              m += F("<option value='");
            }
            m += cur->value;
            m += F("'>");
            m += String(cur->name);
            m += F("</option>");
            cur++;
          }
          m += F("");
          m += F("</select>");
          break;
        }
        case CONFIG_TYPE_GA:
          address_t a = config_get_ga(i);
          m += F("<input class='form-control' type='number' name='area' min='0' max='31' value='");
          m += String((a.bytes.high & 0xF8) >> 3);
          m += F("'/>");
          m += F("<div class='input-group-insert'><span class='input-group-text'>/</span></div>");
          m += F("<input class='form-control' type='number' name='line' min='0' max='7' value='");
          m += String(a.bytes.high & 0x07);
          m += F("'/>");
          m += F("<div class='input-group-insert'><span class='input-group-text'>/</span></div>");
          m += F("<input class='form-control' type='number' name='member' min='0' max='255' value='");
          m += String(a.bytes.low);
          m += F("'/>");
          break;
      }
      m += F("<input type='hidden' name='id' value='");
      m += i;
      m += F("'/>");
      m += F("<div class='input-group-append'><button type='submit' class='btn btn-primary'>Set</button></div>");
      m += F("</div></div></div>");
      m += F("</form>");
    }
  }

  // EEPROM save and restore
  m += F("<div class='row'>");
  // Save to EEPROM
  m += F("<div class='col-auto'>");
  m += F("<form action='" __EEPROM_PATH "' method='POST'>");
  m += F("<input type='hidden' name='mode' value='1'>");
  m += F("<button type='submit' class='btn btn-success'>Save to EEPROM</button>");
  m += F("</form>");
  m += F("</div>");
  // Restore from EEPROM
  m += F("<div class='col-auto'>");
  m += F("<form action='" __EEPROM_PATH "' method='POST'>");
  m += F("<input type='hidden' name='mode' value='2'>");
  m += F("<button type='submit' class='btn btn-info'>Restore from EEPROM</button>");
  m += F("</form>");
  m += F("</div>");
  // Load Defaults
  m += F("<div class='col-auto'>");
  m += F("<form action='" __RESTORE_PATH "' method='POST'>");
  m += F("<button type='submit' class='btn btn-warning'>Restore defaults</button>");
  m += F("</form>");
  m += F("</div>");
  // Reboot
  m += F("<div class='col-auto'>");
  m += F("<form action='" __REBOOT_PATH "' method='POST'>");
  m += F("<button type='submit' class='btn btn-danger'>Reboot</button>");
  m += F("</form>");
  m += F("</div>");

  m += F("</div>"); // row

  // End of page
  m += F("</div></body></html>");
  server->send(200, F("text/html"), m);
}

void ESPKNXIP::__handle_register()
{
  DEBUG_PRINTLN(F("Register called"));
  if (server->hasArg(F("area")) && server->hasArg(F("line")) && server->hasArg(F("member")) && server->hasArg(F("cb")))
  {
    uint8_t area = server->arg(F("area")).toInt();
    uint8_t line = server->arg(F("line")).toInt();
    uint8_t member = server->arg(F("member")).toInt();
    callback_id_t cb = (callback_id_t)server->arg(F("cb")).toInt();

    DEBUG_PRINT(F("Got args: "));
    DEBUG_PRINT(area);
    DEBUG_PRINT(F("/"));
    DEBUG_PRINT(line);
    DEBUG_PRINT(F("/"));
    DEBUG_PRINT(member);
    DEBUG_PRINT(F("/"));
    DEBUG_PRINT(cb);
    DEBUG_PRINTLN(F(""));

    if (area > 31 || line > 7)
    {
      DEBUG_PRINTLN(F("Area or Line wrong"));
      goto end;
    }

    if (cb >= registered_callbacks)
    {
      DEBUG_PRINTLN(F("Invalid callback id"));
      goto end;
    }

    __callback_register_assignment(area, line, member, cb);
  }
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

void ESPKNXIP::__handle_delete()
{
  DEBUG_PRINTLN(F("Delete called"));
  if (server->hasArg(F("id")))
  {
    callback_assignment_id_t id = (callback_assignment_id_t)server->arg(F("id")).toInt();

    DEBUG_PRINT(F("Got args: "));
    DEBUG_PRINT(id);
    DEBUG_PRINTLN(F(""));

    if (id >= registered_callback_assignments)
    {
      DEBUG_PRINTLN(F("ID wrong"));
      goto end;
    }

    __callback_delete_assignment(id);
  }
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

void ESPKNXIP::__handle_set()
{
  DEBUG_PRINTLN(F("Set called"));
  if (server->hasArg(F("area")) && server->hasArg(F("line")) && server->hasArg(F("member")))
  {
    uint8_t area = server->arg(F("area")).toInt();
    uint8_t line = server->arg(F("line")).toInt();
    uint8_t member = server->arg(F("member")).toInt();

    DEBUG_PRINT(F("Got args: "));
    DEBUG_PRINT(area);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(line);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(member);
    DEBUG_PRINTLN(F(""));

    if (area > 31 || line > 7)
    {
      DEBUG_PRINTLN(F("Area or Line wrong"));
      goto end;
    }

    physaddr.bytes.high = (area << 4) | line;
    physaddr.bytes.low = member;
  }
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

void ESPKNXIP::__handle_config()
{
  DEBUG_PRINTLN(F("Config called"));
  if (server->hasArg(F("id")))
  {
    config_id_t id = server->arg(F("id")).toInt();

    DEBUG_PRINT(F("Got args: "));
    DEBUG_PRINT(id);
    DEBUG_PRINTLN(F(""));

    if (id < 0 || id >= registered_configs)
    {
      DEBUG_PRINTLN(F("ID wrong"));
      goto end;
    }

    switch (custom_configs[id].type)
    {
      case CONFIG_TYPE_STRING:
      {
        String v = server->arg(F("value"));
        if (v.length() >= custom_configs[id].len)
          goto end;
        __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
        __config_set_string(id, v);
        break;
      }
      case CONFIG_TYPE_INT:
      {
        __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
        __config_set_int(id, server->arg(F("value")).toInt());
        break;
      }
      case CONFIG_TYPE_BOOL:
      {
        __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
        __config_set_bool(id, server->arg(F("value")).compareTo(F("on")) == 0);
        break;
      }
      case CONFIG_TYPE_OPTIONS:
      {
        uint8_t val = (uint8_t)server->arg(F("value")).toInt();
        DEBUG_PRINT(F("Value: "));
        DEBUG_PRINTLN(val);
        config_set_options(id, val);
        break;
      }
      case CONFIG_TYPE_GA:
      {
        uint8_t area = server->arg(F("area")).toInt();
        uint8_t line = server->arg(F("line")).toInt();
        uint8_t member = server->arg(F("member")).toInt();
        if (area > 31 || line > 7)
        {
          DEBUG_PRINTLN(F("Area or Line wrong"));
          goto end;
        }
        address_t tmp;
        tmp.bytes.high = (area << 3) | line;
        tmp.bytes.low = member;
        __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
        __config_set_ga(id, tmp);
        break;
      }
    }
  }
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

void ESPKNXIP::__handle_restore()
{
  DEBUG_PRINTLN(F("Restore called"));
  memcpy(custom_config_data, custom_config_default_data, MAX_CONFIG_SPACE);
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

void ESPKNXIP::__handle_reboot()
{
  DEBUG_PRINTLN(F("Rebooting!"));
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
  delay(1000);
  ESP.restart();
  //while(1);
}

void ESPKNXIP::__handle_eeprom()
{
  DEBUG_PRINTLN(F("EEPROM called"));
  if (server->hasArg(F("mode")))
  {
    uint8_t mode = server->arg(F("mode")).toInt();

    DEBUG_PRINT(F("Got args: "));
    DEBUG_PRINT(mode);
    DEBUG_PRINTLN(F(""));

    if (mode == 1)
    {
      // save
      save_to_eeprom();
    }
    else if (mode == 2)
    {
      // restore
      restore_from_eeprom();
    }
  }
end:
  server->sendHeader(F("Location"),F(__ROOT_PATH));
  server->send(302);
}

ESPKNXIP::ESPKNXIP() : registered_callback_assignments(0), registered_callbacks(0), registered_configs(0), registered_feedbacks(0)
{
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("ESPKNXIP starting up");
  // Default physical address is 1.1.0
  physaddr.bytes.high = (/*area*/1 << 4) | /*line*/1;
  physaddr.bytes.low = /*member*/0;
  memset(callback_assignments, 0, MAX_CALLBACK_ASSIGNMENTS * sizeof(callback_assignment_t));
  memset(callbacks, 0, MAX_CALLBACKS * sizeof(callback_fptr_t));
  memset(custom_config_data, 0, MAX_CONFIG_SPACE * sizeof(uint8_t));
  memset(custom_config_default_data, 0, MAX_CONFIG_SPACE * sizeof(uint8_t));
  memset(custom_configs, 0, MAX_CONFIGS * sizeof(config_t));
}

void ESPKNXIP::load()
{
  memcpy(custom_config_default_data, custom_config_data, MAX_CONFIG_SPACE);
  EEPROM.begin(EEPROM_SIZE);
  restore_from_eeprom();
}

void ESPKNXIP::start(ESP8266WebServer *srv)
{
  if (srv == nullptr)
  {
    return;
  }
  server = srv;
  __start();
}

void ESPKNXIP::start()
{
  server = new ESP8266WebServer(80);
  __start();
}

void ESPKNXIP::__start()
{
  server->on(ROOT_PREFIX, [this](){
    __handle_root();
  });
  server->on(__ROOT_PATH, [this](){
    __handle_root();
  });
  server->on(__REGISTER_PATH, [this](){
    __handle_register();
  });
  server->on(__DELETE_PATH, [this](){
    __handle_delete();
  });
  server->on(__PHYS_PATH, [this](){
    __handle_set();
  });
  server->on(__EEPROM_PATH, [this](){
    __handle_eeprom();
  });
  server->on(__CONFIG_PATH, [this](){
    __handle_config();
  });
  server->on(__RESTORE_PATH, [this](){
    __handle_restore();
  });
  server->on(__REBOOT_PATH, [this](){
    __handle_reboot();
  });
  server->begin();

  udp.beginMulticast(WiFi.localIP(),  MULTICAST_IP, MULTICAST_PORT);
}

void ESPKNXIP::save_to_eeprom()
{
  uint32_t address = 0;
  uint64_t magic = EEPROM_MAGIC;
  EEPROM.put(address, magic);
  address += sizeof(uint64_t);
  EEPROM.put(address++, registered_callback_assignments);
  for (uint8_t i = 0; i < MAX_CALLBACK_ASSIGNMENTS; ++i)
  {
    EEPROM.put(address, callback_assignments[i].address);
    address += sizeof(address_t);
  }
  for (uint8_t i = 0; i < MAX_CALLBACK_ASSIGNMENTS; ++i)
  {
    EEPROM.put(address, callback_assignments[i].callback_id);
    address += sizeof(callback_id_t);
  }
  EEPROM.put(address, physaddr);
  address += sizeof(address_t);

  EEPROM.put(address, custom_config_data);
  address += sizeof(custom_config_data);

  EEPROM.commit();
  DEBUG_PRINT("Wrote to EEPROM: 0x");
  DEBUG_PRINTLN(address, HEX);
}

void ESPKNXIP::restore_from_eeprom()
{
  uint32_t address = 0;
  uint64_t magic = 0;
  EEPROM.get(address, magic);
  if (magic != EEPROM_MAGIC)
  {
    DEBUG_PRINTLN("No valid magic in EEPROM, aborting restore.");
    DEBUG_PRINT("Expected 0x");
    DEBUG_PRINT((unsigned long)(EEPROM_MAGIC >> 32), HEX);
    DEBUG_PRINT(" 0x");
    DEBUG_PRINT((unsigned long)(EEPROM_MAGIC), HEX);
    DEBUG_PRINT(" got 0x");
    DEBUG_PRINT((unsigned long)(magic >> 32), HEX);
    DEBUG_PRINT(" 0x");
    DEBUG_PRINTLN((unsigned long)magic, HEX);
    return;
  }
  address += sizeof(uint64_t);
  EEPROM.get(address++, registered_callback_assignments);
  for (uint8_t i = 0; i < MAX_CALLBACK_ASSIGNMENTS; ++i)
  {
    EEPROM.get(address, callback_assignments[i].address);
    address += sizeof(address_t);
  }
  for (uint8_t i = 0; i < MAX_CALLBACK_ASSIGNMENTS; ++i)
  {
    EEPROM.get(address, callback_assignments[i].callback_id);
    address += sizeof(callback_id_t);
  }
  EEPROM.get(address, physaddr);
  address += sizeof(address_t); 

  //EEPROM.get(address, custom_config_data);
  //address += sizeof(custom_config_data);
  uint32_t conf_offset = address;
  for (uint8_t i = 0; i < registered_configs; ++i)
  {
    // First byte is flags.
    config_flags_t flags = CONFIG_FLAGS_NO_FLAGS;
    flags = (config_flags_t)EEPROM.read(address);
    DEBUG_PRINT("Flag in EEPROM @ ");
    DEBUG_PRINT(address - conf_offset);
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(flags, BIN);
    custom_config_data[custom_configs[i].offset] = flags;
    if (flags & CONFIG_FLAGS_VALUE_SET)
    {
      DEBUG_PRINTLN("Non-default value");
      for (int j = 0; j < custom_configs[i].len - sizeof(uint8_t); ++j)
      {
        custom_config_data[custom_configs[i].offset + sizeof(uint8_t) + j] = EEPROM.read(address + sizeof(uint8_t) + j);
      }
    }

    address += custom_configs[i].len;
  }

  DEBUG_PRINT("Restored from EEPROM: 0x");
  DEBUG_PRINTLN(address, HEX);
}

uint16_t ESPKNXIP::ntohs(uint16_t n)
{
  return (uint16_t)((((uint8_t*)&n)[0] << 8) | (((uint8_t*)&n)[1]));
}

callback_assignment_id_t ESPKNXIP::__callback_register_assignment(uint8_t area, uint8_t line, uint8_t member, callback_id_t cb)
{
  if (registered_callback_assignments >= MAX_CALLBACK_ASSIGNMENTS)
    return -1;

  callback_assignment_id_t id = registered_callback_assignments;

  callback_assignments[id].address.bytes.high = (area << 3) | line;
  callback_assignments[id].address.bytes.low = member;
  callback_assignments[id].callback_id = cb;
  registered_callback_assignments++;
  return id;
}

void ESPKNXIP::__callback_delete_assignment(callback_assignment_id_t id)
{
  if (id >= registered_callback_assignments)
    return;

  uint32_t dest_offset = 0;
  uint32_t src_offset = 0;
  uint32_t len = 0;
  if (id == 0)
  {
    // start of array, so delete first entry
    src_offset = 1;
    // registered_ga_callbacks will be 1 in case of only one entry
    // registered_ga_callbacks will be 2 in case of two entries, etc..
    // so only copy anything, if there is it at least more then one element
    len = (registered_callback_assignments - 1);
  }
  else if (id == registered_callback_assignments - 1)
  {
    // last element, don't do anything, simply decrement counter
  }
  else
  {
    // somewhere in the middle
    // need to calc offsets

    // skip all prev elements
    dest_offset = id; // id is equal to how many element are in front of it
    src_offset = dest_offset + 1; // start after the current element
    len = (registered_callback_assignments - 1 - id);
  }

  if (len > 0)
  {
    memmove(callback_assignments + dest_offset, callback_assignments + src_offset, len * sizeof(callback_assignment_t));
  }

  registered_callback_assignments--;
}

callback_id_t ESPKNXIP::register_callback(String name, callback_fptr_t cb, void *arg, enable_condition_t cond)
{
  if (registered_callbacks >= MAX_CALLBACKS)
    return -1;

  callback_id_t id = registered_callbacks;

  callbacks[id].name = name;
  callbacks[id].fkt = cb;
  callbacks[id].cond = cond;
  callbacks[id].arg = arg;
  registered_callbacks++;
  return id;
}

/**
 * Configuration functions start here
 */
config_id_t ESPKNXIP::config_register_string(String name, uint8_t len, String _default, enable_condition_t cond)
{
  if (registered_configs >= MAX_CONFIGS)
    return -1;

  if (_default.length() >= len)
    return -1;

  config_id_t id = registered_configs;

  custom_configs[id].name = name;
  custom_configs[id].type = CONFIG_TYPE_STRING;
  custom_configs[id].len = sizeof(uint8_t) + len;
  custom_configs[id].cond = cond;
  if (id == 0)
    custom_configs[id].offset = 0;
  else
    custom_configs[id].offset = custom_configs[id - 1].offset + custom_configs[id - 1].len;

  __config_set_string(id, _default);

  registered_configs++;

  DEBUG_PRINT("Registered config >");
  DEBUG_PRINT(name);
  DEBUG_PRINT("< @ ");
  DEBUG_PRINT(id);
  DEBUG_PRINT("/string[");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT("+");
  DEBUG_PRINT(custom_configs[id].len);
  DEBUG_PRINTLN("]");

  return id;
}

config_id_t ESPKNXIP::config_register_int(String name, int32_t _default, enable_condition_t cond)
{
  if (registered_configs >= MAX_CONFIGS)
    return -1;

  config_id_t id = registered_configs;

  custom_configs[id].name = name;
  custom_configs[id].type = CONFIG_TYPE_INT;
  custom_configs[id].len = sizeof(uint8_t) + sizeof(int32_t);
  custom_configs[id].cond = cond;
  if (id == 0)
    custom_configs[id].offset = 0;
  else
    custom_configs[id].offset = custom_configs[id - 1].offset + custom_configs[id - 1].len;

  __config_set_int(id, _default);

  registered_configs++;

  DEBUG_PRINT("Registered config >");
  DEBUG_PRINT(name);
  DEBUG_PRINT("< @ ");
  DEBUG_PRINT(id);
  DEBUG_PRINT("/int[");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT("+");
  DEBUG_PRINT(custom_configs[id].len);
  DEBUG_PRINTLN("]");

  return id;
}

config_id_t ESPKNXIP::config_register_bool(String name, bool _default, enable_condition_t cond)
{
  if (registered_configs >= MAX_CONFIGS)
    return -1;

  config_id_t id = registered_configs;

  custom_configs[id].name = name;
  custom_configs[id].type = CONFIG_TYPE_BOOL;
  custom_configs[id].len = sizeof(uint8_t) + sizeof(uint8_t);
  custom_configs[id].cond = cond;
  if (id == 0)
    custom_configs[id].offset = 0;
  else
    custom_configs[id].offset = custom_configs[id - 1].offset + custom_configs[id - 1].len;

  __config_set_bool(id, _default);

  registered_configs++;

  DEBUG_PRINT("Registered config >");
  DEBUG_PRINT(name);
  DEBUG_PRINT("< @ ");
  DEBUG_PRINT(id);
  DEBUG_PRINT("/bool[");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT("+");
  DEBUG_PRINT(custom_configs[id].len);
  DEBUG_PRINTLN("]");

  return id;
}

config_id_t ESPKNXIP::config_register_options(String name, option_entry_t *options, uint8_t _default, enable_condition_t cond)
{
  if (registered_configs >= MAX_CONFIGS)
    return -1;

  if (options == nullptr || options->name == nullptr)
    return -1;

  config_id_t id = registered_configs;

  custom_configs[id].name = name;
  custom_configs[id].type = CONFIG_TYPE_OPTIONS;
  custom_configs[id].len = sizeof(uint8_t) + sizeof(uint8_t);
  custom_configs[id].cond = cond;
  if (id == 0)
    custom_configs[id].offset = 0;
  else
    custom_configs[id].offset = custom_configs[id - 1].offset + custom_configs[id - 1].len;

  custom_configs[id].data.options = options;

  __config_set_options(id, _default);

  registered_configs++;

  DEBUG_PRINT("Registered config >");
  DEBUG_PRINT(name);
  DEBUG_PRINT("< @ ");
  DEBUG_PRINT(id);
  DEBUG_PRINT("/opt[");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT("+");
  DEBUG_PRINT(custom_configs[id].len);
  DEBUG_PRINTLN("]");

  return id;
}

config_id_t ESPKNXIP::config_register_ga(String name, enable_condition_t cond)
{
  if (registered_configs >= MAX_CONFIGS)
    return -1;

  config_id_t id = registered_configs;

  custom_configs[id].name = name;
  custom_configs[id].type = CONFIG_TYPE_GA;
  custom_configs[id].len = sizeof(uint8_t) + sizeof(address_t);
  custom_configs[id].cond = cond;
  if (id == 0)
    custom_configs[id].offset = 0;
  else
    custom_configs[id].offset = custom_configs[id - 1].offset + custom_configs[id - 1].len;

  address_t t;
  t.value = 0;
  __config_set_ga(id, t);

  registered_configs++;

  DEBUG_PRINT("Registered config >");
  DEBUG_PRINT(name);
  DEBUG_PRINT("< @ ");
  DEBUG_PRINT(id);
  DEBUG_PRINT("/ga[");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT("+");
  DEBUG_PRINT(custom_configs[id].len);
  DEBUG_PRINTLN("]");

  return id;
}

void ESPKNXIP::__config_set_flags(config_id_t id, config_flags_t flags)
{
  DEBUG_PRINT("Setting flag @ ");
  DEBUG_PRINT(custom_configs[id].offset);
  DEBUG_PRINT(" to ");
  DEBUG_PRINT(custom_config_data[custom_configs[id].offset], BIN);
  DEBUG_PRINT(" | ");
  DEBUG_PRINT(flags, BIN);
  custom_config_data[custom_configs[id].offset] |= (uint8_t)flags;
  DEBUG_PRINT(" = ");
  DEBUG_PRINTLN(custom_config_data[custom_configs[id].offset], BIN);
}

void ESPKNXIP::config_set_string(config_id_t id, String val)
{
  if (id >= registered_configs)
    return;
  if (custom_configs[id].type != CONFIG_TYPE_STRING)
    return;
  if (val.length() >= custom_configs[id].len)
    return;
  __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
  __config_set_string(id, val);
}

void ESPKNXIP::__config_set_string(config_id_t id, String &val)
{
  memcpy(&custom_config_data[custom_configs[id].offset + sizeof(uint8_t)], val.c_str(), val.length()+1);
}

void ESPKNXIP::config_set_int(config_id_t id, int32_t val)
{
  if (id >= registered_configs)
    return;
  if (custom_configs[id].type != CONFIG_TYPE_INT)
    return;
  __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
  __config_set_int(id, val);
}

void ESPKNXIP::__config_set_int(config_id_t id, int32_t val)
{
  // This does not work for some reason:
  // Could be due to pointer alignment
  //int32_t *v = (int32_t *)(custom_config_data + custom_configs[id].offset);
  //*v = val;
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 0] = (uint8_t)((val & 0xFF000000) >> 24);
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 1] = (uint8_t)((val & 0x00FF0000) >> 16);
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 2] = (uint8_t)((val & 0x0000FF00) >>  8);
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 3] = (uint8_t)((val & 0x000000FF) >>  0);
}

void ESPKNXIP::config_set_bool(config_id_t id, bool val)
{
  if (id >= registered_configs)
    return;
  if (custom_configs[id].type != CONFIG_TYPE_BOOL)
    return;
  __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
  __config_set_bool(id, val);
}

void ESPKNXIP::__config_set_bool(config_id_t id, bool val)
{
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t)] = val ? 1 : 0;
}

void ESPKNXIP::config_set_options(config_id_t id, uint8_t val)
{
  if (id >= registered_configs)
    return;
  if (custom_configs[id].type != CONFIG_TYPE_OPTIONS)
    return;

  option_entry_t *cur = custom_configs[id].data.options;
  while (cur->name != nullptr)
  {
    if (cur->value == val)
    {
      __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
      __config_set_options(id, val);
      break;
    }
    cur++;
  }
}

void ESPKNXIP::__config_set_options(config_id_t id, uint8_t val)
{
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t)] = val;
}

void ESPKNXIP::config_set_ga(config_id_t id, address_t val)
{
  if (id >= registered_configs)
    return;
  if (custom_configs[id].type != CONFIG_TYPE_GA)
    return;
  __config_set_flags(id, CONFIG_FLAGS_VALUE_SET);
  __config_set_ga(id, val);
}

void ESPKNXIP::__config_set_ga(config_id_t id, address_t const &val)
{
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 0] = val.bytes.high;
  custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 1] = val.bytes.low;
}

String ESPKNXIP::config_get_string(config_id_t id)
{
  if (id >= registered_configs)
    return String("");

  return String((char *)&custom_config_data[custom_configs[id].offset + sizeof(uint8_t)]);
}

int32_t ESPKNXIP::config_get_int(config_id_t id)
{
  if (id >= registered_configs)
    return 0;

  int32_t v = (custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 0] << 24) +
              (custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 1] << 16) +
              (custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 2] <<  8) +
              (custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 3] <<  0);
  return v;
}

bool ESPKNXIP::config_get_bool(config_id_t id)
{
  if (id >= registered_configs)
    return false;

  return custom_config_data[custom_configs[id].offset + sizeof(uint8_t)] != 0;
}

uint8_t ESPKNXIP::config_get_options(config_id_t id)
{
  if (id >= registered_configs)
    return false;

  return custom_config_data[custom_configs[id].offset + sizeof(uint8_t)];
}

address_t ESPKNXIP::config_get_ga(config_id_t id)
{
  address_t t;
  if (id >= registered_configs)
  {
    t.value = 0;
    return t;
  }

  t.bytes.high = custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 0];
  t.bytes.low = custom_config_data[custom_configs[id].offset + sizeof(uint8_t) + 1];

  return t;
}

/**
 * Feedback functions start here
 */

feedback_id_t ESPKNXIP::feedback_register_int(String name, int32_t *value, enable_condition_t cond)
{
  if (registered_feedbacks >= MAX_FEEDBACKS)
    return -1;

  feedback_id_t id = registered_feedbacks;

  feedbacks[id].type = FEEDBACK_TYPE_INT;
  feedbacks[id].name = name;
  feedbacks[id].cond = cond;
  feedbacks[id].data = (void *)value;

  registered_feedbacks++;

  return id;
}

feedback_id_t ESPKNXIP::feedback_register_float(String name, float *value, uint8_t precision, enable_condition_t cond)
{
  if (registered_feedbacks >= MAX_FEEDBACKS)
    return -1;

  feedback_id_t id = registered_feedbacks;

  feedbacks[id].type = FEEDBACK_TYPE_FLOAT;
  feedbacks[id].name = name;
  feedbacks[id].cond = cond;
  feedbacks[id].data = (void *)value;
  feedbacks[id].options.float_options.precision = precision;

  registered_feedbacks++;

  return id;
}

/**
 * Send functions start here
 */

void ESPKNXIP::send(address_t const &receiver, knx_command_type_t ct, uint8_t data_len, uint8_t *data)
{
  if (receiver.value == 0)
    return;

  uint32_t len = 6 + 2 + 8 + data_len + 1; // knx_pkt + cemi_msg + cemi_service + data + checksum
  DEBUG_PRINT(F("Creating packet with len "));
  DEBUG_PRINTLN(len)
  uint8_t buf[len];
  knx_ip_pkt_t *knx_pkt = (knx_ip_pkt_t *)buf;
  knx_pkt->header_len = 0x06;
  knx_pkt->protocol_version = 0x10;
  knx_pkt->service_type = ntohs(KNX_ST_ROUTING_INDICATION);
  knx_pkt->total_len.len = ntohs(len);
  cemi_msg_t *cemi_msg = (cemi_msg_t *)knx_pkt->pkt_data;
  cemi_msg->message_code = KNX_MT_L_DATA_IND;
  cemi_msg->additional_info_len = 0;
  cemi_service_t *cemi_data = &cemi_msg->data.service_information;
  cemi_data->control_1.bits.confirm = 0;
  cemi_data->control_1.bits.ack = 0;
  cemi_data->control_1.bits.priority = B11;
  cemi_data->control_1.bits.system_broadcast = 0x01;
  cemi_data->control_1.bits.repeat = 0x01;
  cemi_data->control_1.bits.reserved = 0;
  cemi_data->control_1.bits.frame_type = 0x01;
  cemi_data->control_2.bits.extended_frame_format = 0x00;
  cemi_data->control_2.bits.hop_count = 0x06;
  cemi_data->control_2.bits.dest_addr_type = 0x01;
  cemi_data->source = physaddr;
  cemi_data->destination = receiver;
  //cemi_data->destination.bytes.high = (area << 3) | line;
  //cemi_data->destination.bytes.low = member;
  cemi_data->data_len = data_len;
  cemi_data->pci.apci = (ct & 0x0C) >> 2;
  cemi_data->pci.tpci_seq_number = 0x00; // ???
  cemi_data->pci.tpci_comm_type = KNX_COT_UDP; // ???
  memcpy(cemi_data->data, data, data_len);
  cemi_data->data[0] = (cemi_data->data[0] & 0x3F) | ((ct & 0x03) << 6);

  // Calculate checksum, which is just XOR of all bytes
  uint8_t cs = buf[0] ^ buf[1];
  for (uint32_t i = 2; i < len - 1; ++i)
  {
    cs ^= buf[i];
  }
  buf[len - 1] = cs;

  DEBUG_PRINT(F("Sending packet:"));
  for (int i = 0; i < len; ++i)
  {
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINT(buf[i], 16);
  }
  DEBUG_PRINTLN(F(""));

  udp.beginPacketMulticast(MULTICAST_IP, MULTICAST_PORT, WiFi.localIP());
  udp.write(buf, len);
  udp.endPacket();
}

void ESPKNXIP::send1Bit(address_t const &receiver, knx_command_type_t ct, uint8_t bit)
{
  uint8_t buf[] = {bit & 0b00000001};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send2Bit(address_t const &receiver, knx_command_type_t ct, uint8_t twobit)
{
  uint8_t buf[] = {twobit & 0b00000011};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send4Bit(address_t const &receiver, knx_command_type_t ct, uint8_t fourbit)
{
  uint8_t buf[] = {fourbit & 0b00001111};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send1ByteInt(address_t const &receiver, knx_command_type_t ct, int8_t val)
{
  uint8_t buf[] = {0x00, (uint8_t)val};
  send(receiver, ct, 2, buf);
}

int8_t ESPKNXIP::data_to_1byte_int(uint8_t *data)
{
  return (int8_t)data[1];
}

void ESPKNXIP::send2ByteInt(address_t const &receiver, knx_command_type_t ct, int16_t val)
{
  uint8_t buf[] = {0x00, (uint8_t)(val >> 8), (uint8_t)(val & 0x00FF)};
  send(receiver, ct, 3, buf);
}

int16_t ESPKNXIP::data_to_2byte_int(uint8_t *data)
{
  return (int16_t)((data[1] << 8) | data[2]);
}

void ESPKNXIP::send2ByteFloat(address_t const &receiver, knx_command_type_t ct, float val)
{
  float v = val * 100.0f;
  int e = 0;
  for (; v < -2048.0f; v /= 2)
    ++e;
  for (; v > 2047.0f; v /= 2)
    ++e;
  long m = round(v) & 0x7FF;
  short msb = (short) (e << 3 | m >> 8);
  if (val < 0.0f)
    msb |= 0x80;
  uint8_t buf[] = {0x00, (uint8_t)msb, (uint8_t)m};
  send(receiver, ct, 3, buf);
}

float ESPKNXIP::data_to_2byte_float(uint8_t *data)
{
  //uint8_t  sign = (data[1] & 0b10000000) >> 7;
  uint8_t  expo = (data[1] & 0b01111000) >> 3;
  int16_t mant = ((data[1] & 0b10000111) << 8) | data[2];
  return 0.01f * mant * pow(2, expo);
}

void ESPKNXIP::send3ByteTime(address_t const &receiver, knx_command_type_t ct, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  weekday <<= 5;
  uint8_t buf[] = {0x00, (((weekday << 5) & 0xE0) + (hours & 0x1F)), minutes & 0x3F, seconds & 0x3F};
  send(receiver, ct, 4, buf);
}

time_of_day_t ESPKNXIP::data_to_3byte_time(uint8_t *data)
{
  time_of_day_t time;
  time.weekday = (weekday_t)((data[1] & 0b11100000) >> 5);
  time.hours = (data[1] & 0b00011111);
  time.minutes = (data[2] & 0b00111111);
  time.seconds = (data[3] & 0b00111111);
  return time;
}

void ESPKNXIP::send3ByteDate(address_t const &receiver, knx_command_type_t ct, uint8_t day, uint8_t month, uint8_t year)
{
  uint8_t buf[] = {0x00, day & 0x1F, month & 0x0F, year};
  send(receiver, ct, 4, buf);
}

date_t ESPKNXIP::data_to_3byte_data(uint8_t *data)
{
  date_t date;
  date.day = (data[1] & 0b00011111);
  date.month = (data[2] & 0b00001111);
  date.year = (data[3] & 0b01111111);
  return date;
}

void ESPKNXIP::send3ByteColor(address_t const &receiver, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue)
{
  uint8_t buf[] = {0x00, red, green, blue};
  send(receiver, ct, 4, buf);
}

color_t ESPKNXIP::data_to_3byte_color(uint8_t *data)
{
  color_t color;
  color.red = data[1];
  color.green = data[2];
  color.blue = data[3];
  return color;
}

void ESPKNXIP::send4ByteFloat(address_t const &receiver, knx_command_type_t ct, float val)
{
  uint8_t buf[] = {0x00, ((uint8_t *)&val)[3], ((uint8_t *)&val)[2], ((uint8_t *)&val)[1], ((uint8_t *)&val)[0]};
  send(receiver, ct, 5, buf);
}

float ESPKNXIP::data_to_4byte_float(uint8_t *data)
{
  return (float)((data[1] << 24) | (data[2] << 16) | (data[3] << 8) |data[4]);
}

void ESPKNXIP::loop()
{
  __loop_knx();
  __loop_webserver();
}

void ESPKNXIP::__loop_webserver()
{
  server->handleClient();
}

void ESPKNXIP::__loop_knx()
{
  int read = udp.parsePacket();
  if (!read)
  {
    return;
  }
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINT(F("LEN: "));
  DEBUG_PRINTLN(read);

  uint8_t buf[read];

  udp.read(buf, read);
  udp.flush();

  DEBUG_PRINT(F("Got packet:"));
  for (int i = 0; i < read; ++i)
  {
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINT(buf[i], 16);
  }
  DEBUG_PRINTLN(F(""));

  knx_ip_pkt_t *knx_pkt = (knx_ip_pkt_t *)buf;

  DEBUG_PRINT(F("ST: 0x"));
  DEBUG_PRINTLN(ntohs(knx_pkt->service_type), 16);

  if (knx_pkt->header_len != 0x06 && knx_pkt->protocol_version != 0x10 && knx_pkt->service_type != KNX_ST_ROUTING_INDICATION)
    return;

  cemi_msg_t *cemi_msg = (cemi_msg_t *)knx_pkt->pkt_data;

  DEBUG_PRINT(F("MT: 0x"));
  DEBUG_PRINTLN(cemi_msg->message_code, 16);

  if (cemi_msg->message_code != KNX_MT_L_DATA_IND)
    return;

  DEBUG_PRINT(F("ADDI: 0x"));
  DEBUG_PRINTLN(cemi_msg->additional_info_len, 16);

  cemi_service_t *cemi_data = &cemi_msg->data.service_information;

  if (cemi_msg->additional_info_len > 0)
    cemi_data = (cemi_service_t *)(((uint8_t *)cemi_data) + cemi_msg->additional_info_len);

  DEBUG_PRINT(F("C1: 0x"));
  DEBUG_PRINTLN(cemi_data->control_1.byte, 16);

  DEBUG_PRINT(F("C2: 0x"));
  DEBUG_PRINTLN(cemi_data->control_2.byte, 16);

  DEBUG_PRINT(F("DT: 0x"));
  DEBUG_PRINTLN(cemi_data->control_2.bits.dest_addr_type, 16);

  if (cemi_data->control_2.bits.dest_addr_type != 0x01)
    return;

  DEBUG_PRINT(F("HC: 0x"));
  DEBUG_PRINTLN(cemi_data->control_2.bits.hop_count, 16);

  DEBUG_PRINT(F("EFF: 0x"));
  DEBUG_PRINTLN(cemi_data->control_2.bits.extended_frame_format, 16);

  DEBUG_PRINT(F("Source: 0x"));
  DEBUG_PRINT(cemi_data->source.bytes.high, 16);
  DEBUG_PRINT(F(" 0x"));
  DEBUG_PRINTLN(cemi_data->source.bytes.low, 16);

  DEBUG_PRINT(F("Dest: 0x"));
  DEBUG_PRINT(cemi_data->destination.bytes.high, 16);
  DEBUG_PRINT(F(" 0x"));
  DEBUG_PRINTLN(cemi_data->destination.bytes.low, 16);

  knx_command_type_t ct = (knx_command_type_t)(((cemi_data->data[0] & 0xC0) >> 6) | ((cemi_data->pci.apci & 0x03) << 2));

  DEBUG_PRINT(F("CT: 0x"));
  DEBUG_PRINTLN(ct, 16);

  for (int i = 0; i < cemi_data->data_len; ++i)
  {
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINT(cemi_data->data[i], 16);
  }

  DEBUG_PRINTLN(F("=="));

  // Call callbacks
  for (int i = 0; i < registered_callback_assignments; ++i)
  {
    DEBUG_PRINT(F("Testing: 0x"));
    DEBUG_PRINT(callback_assignments[i].address.bytes.high, 16);
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINTLN(callback_assignments[i].address.bytes.low, 16);
    if (cemi_data->destination.value == callback_assignments[i].address.value)
    {
      DEBUG_PRINTLN(F("Found match"));
      if (callbacks[callback_assignments[i].callback_id].cond && !callbacks[callback_assignments[i].callback_id].cond())
      {
        DEBUG_PRINTLN(F("But it's disabled"));
#if ALLOW_MULTIPLE_CALLBACKS_PER_ADDRESS
        continue;
#else
        return;
#endif
      }
      uint8_t data[cemi_data->data_len];
      memcpy(data, cemi_data->data, cemi_data->data_len);
      data[0] = data[0] & 0x3F;
      message_t msg = {};
      msg.ct = ct;
      msg.received_on = cemi_data->destination;
      msg.data_len = cemi_data->data_len;
      msg.data = data;
      callbacks[callback_assignments[i].callback_id].fkt(msg, callbacks[callback_assignments[i].callback_id].arg);
#if ALLOW_MULTIPLE_CALLBACKS_PER_ADDRESS
      continue;
#else
      return;
#endif
    }
  }

  return;
}

// Global "singleton" object
ESPKNXIP knx;