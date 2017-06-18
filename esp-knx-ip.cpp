/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: Not yet decided on one...
 */

#include "esp-knx-ip.h"


ESPKNXIP::ESPKNXIP()
{

}

void ESPKNXIP::setup(uint8_t area, uint8_t line, uint8_t member)
{
  this->registered_callbacks = 0;
  this->physaddr.bytes.high = (area << 4) | line;
  this->physaddr.bytes.low = member;
  udp.beginMulticast(WiFi.localIP(),  MULTICAST_IP, MULTICAST_PORT);
}

uint16_t ESPKNXIP::ntohs(uint16_t n)
{
  return (uint16_t)((((uint8_t*)&n)[0] << 8) | (((uint8_t*)&n)[1]));
}

int ESPKNXIP::register_GA_callback(uint8_t area, uint8_t line, uint8_t member, GACallback cb)
{
  if (this->registered_callbacks >= MAX_CALLBACKS)
    return -1;

  this->ga_callback_addrs[this->registered_callbacks].bytes.high = (area << 3) | line;
  this->ga_callback_addrs[this->registered_callbacks].bytes.low = member;
  this->ga_callbacks[this->registered_callbacks] = cb;
  this->registered_callbacks++;
  return 0;
}

void ESPKNXIP::send(uint8_t area, uint8_t line, uint8_t member, knx_command_type_t ct, uint8_t data_len, uint8_t *data)
{
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
  cemi_data->source = this->physaddr;
  cemi_data->destination.bytes.high = (area << 3) | line;
  cemi_data->destination.bytes.low = member;
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

void ESPKNXIP::sendColor(uint8_t area, uint8_t line, uint8_t member, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue)
{
  uint8_t buf[] = {0x00, red, green, blue};
  send(area, line, member, ct, 4, buf);
}

void ESPKNXIP::sendBit(uint8_t area, uint8_t line, uint8_t member, knx_command_type_t ct, uint8_t bit)
{
  uint8_t buf[] = {bit > 0x00 ? 0x01 : 0x00};
  send(area, line, member, ct, 1, buf);
}

void ESPKNXIP::loop()
{
  int read = udp.parsePacket();
  if (!read)
    return;
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINT(F("LEN: "));
  DEBUG_PRINTLN(read);

  uint8_t buf[read];

  udp.read(buf, read);

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
  for (int i = 0; i < this->registered_callbacks; ++i)
  {
    DEBUG_PRINT(F("Testing: 0x"));
    DEBUG_PRINT(this->ga_callback_addrs[i].bytes.high, 16);
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINTLN(this->ga_callback_addrs[i].bytes.low, 16);
    if (compareGA(cemi_data->destination.array, this->ga_callback_addrs[i].array))
    {
      DEBUG_PRINTLN(F("Found match"));
      uint8_t data[cemi_data->data_len];
      memcpy(data, cemi_data->data, cemi_data->data_len);
      data[0] = data[0] & 0x3F;
      this->ga_callbacks[i](ct, cemi_data->data_len, data);
      return;
    }
  }

  return;
}

// Global "singleton" object
ESPKNXIP knx;