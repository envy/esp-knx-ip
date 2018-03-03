/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: MIT
 */

#include "esp-knx-ip.h"

/**
 * Send functions
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

void ESPKNXIP::send_1bit(address_t const &receiver, knx_command_type_t ct, uint8_t bit)
{
  uint8_t buf[] = {bit & 0b00000001};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_2bit(address_t const &receiver, knx_command_type_t ct, uint8_t twobit)
{
  uint8_t buf[] = {twobit & 0b00000011};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_4bit(address_t const &receiver, knx_command_type_t ct, uint8_t fourbit)
{
  uint8_t buf[] = {fourbit & 0b00001111};
  send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_1byte_int(address_t const &receiver, knx_command_type_t ct, int8_t val)
{
  uint8_t buf[] = {0x00, (uint8_t)val};
  send(receiver, ct, 2, buf);
}

int8_t ESPKNXIP::data_to_1byte_int(uint8_t *data)
{
  return (int8_t)data[1];
}

void ESPKNXIP::send_1byte_uint(address_t const &receiver, knx_command_type_t ct, uint8_t val)
{
  uint8_t buf[] = {0x00, val};
  send(receiver, ct, 2, buf);
}

uint8_t ESPKNXIP::data_to_1byte_uint(uint8_t *data)
{
  return data[1];
}

void ESPKNXIP::send_2byte_int(address_t const &receiver, knx_command_type_t ct, int16_t val)
{
  uint8_t buf[] = {0x00, (uint8_t)(val >> 8), (uint8_t)(val & 0x00FF)};
  send(receiver, ct, 3, buf);
}

int16_t ESPKNXIP::data_to_2byte_int(uint8_t *data)
{
  return (int16_t)((data[1] << 8) | data[2]);
}

void ESPKNXIP::send_2byte_float(address_t const &receiver, knx_command_type_t ct, float val)
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

void ESPKNXIP::send_3byte_time(address_t const &receiver, knx_command_type_t ct, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds)
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

void ESPKNXIP::send_3byte_date(address_t const &receiver, knx_command_type_t ct, uint8_t day, uint8_t month, uint8_t year)
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

void ESPKNXIP::send_3byte_color(address_t const &receiver, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue)
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

void ESPKNXIP::send_4byte_float(address_t const &receiver, knx_command_type_t ct, float val)
{
  uint8_t buf[] = {0x00, ((uint8_t *)&val)[3], ((uint8_t *)&val)[2], ((uint8_t *)&val)[1], ((uint8_t *)&val)[0]};
  send(receiver, ct, 5, buf);
}

float ESPKNXIP::data_to_4byte_float(uint8_t *data)
{
  return (float)((data[1] << 24) | (data[2] << 16) | (data[3] << 8) |data[4]);
}