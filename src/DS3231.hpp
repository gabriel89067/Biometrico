#ifndef DS3231_H
#define DS3231_H

#include <Wire.h>
#include <TimeLib.h>
#include <Arduino.h>

#define DS3231_END      0x68
#define DS3231_SEG      0x00
#define DS3231_MIN      0x01
#define DS3231_HOR      0x02
#define DS3231_DIA      0x04
#define DS3231_MES      0x05
#define DS3231_ANO      0x06
void DS3231_init();
byte ler_registrador(byte registrador);
byte escrever_registrador(byte registrador, byte dado);
void definir_data_horario(unsigned long epoch);
unsigned long obter_data_horario(void);
void obter_data_horario_string(char *saida);
void converter_int_char(unsigned int dado, char *saida);
byte bcd_para_decimal(byte bcd);
byte decimal_para_bcd(byte decimal);
byte converter_hora(byte hora);

#endif