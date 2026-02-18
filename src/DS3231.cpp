#include <Wire.h>
#include "DS3231.hpp"
#include <TimeLib.h>
#include <Arduino.h>
void DS3231_init() {
   Wire.begin(); //is already called by LiquidCrystal_I2C library
  // Do not reinitialize to avoid lock contention errors
  delay(100);
}

byte ler_registrador(byte registrador)
{
  Wire.beginTransmission(DS3231_END);
  Wire.write(registrador);
  Wire.endTransmission(true);
  delay(2);
  Wire.requestFrom(DS3231_END, (uint8_t)1);
  delay(2);
  byte result = Wire.read();
  return result;
}

byte escrever_registrador(byte registrador, byte dado)
{
  Wire.beginTransmission(DS3231_END);
  Wire.write(registrador);
  Wire.write(dado);
  Wire.endTransmission(true);
  delay(2);
  return 0;
}

void definir_data_horario(unsigned long epoch)
{
  setTime(epoch - 60*60*3);
  escrever_registrador(DS3231_SEG, decimal_para_bcd(second()));
  escrever_registrador(DS3231_MIN, decimal_para_bcd(minute()));
  escrever_registrador(DS3231_HOR, decimal_para_bcd(hour()));

  escrever_registrador(DS3231_DIA, decimal_para_bcd(day()));
  escrever_registrador(DS3231_MES, decimal_para_bcd(month()));
  escrever_registrador(DS3231_ANO, decimal_para_bcd(year()-2000));
}

unsigned long obter_data_horario(void)
{
  setTime(converter_hora(ler_registrador(DS3231_HOR)), bcd_para_decimal(ler_registrador(DS3231_MIN)), bcd_para_decimal(ler_registrador(DS3231_SEG)),
          bcd_para_decimal(ler_registrador(DS3231_DIA)), bcd_para_decimal(ler_registrador(DS3231_MES)&0x7F), bcd_para_decimal(ler_registrador(DS3231_ANO)));
  return now() + 60*60*3;
}

void obter_data_horario_string(char *saida)
{
  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_DIA)), saida);
  saida[2] = '/';
  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_MES)&0x7F), &saida[3]);
  saida[5] = '/';
  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_ANO)), &saida[6]);
  saida[8] = 0x0A;
  saida[9] = ' ';
  saida[10] = ' ';
  saida[11] = ' ';
  saida[12] = ' ';

  converter_int_char(converter_hora(ler_registrador(DS3231_HOR)), &saida[13]);
  saida[15] = ':';
  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_MIN)), &saida[16]);
  saida[18] = ':';
  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_SEG)), &saida[19]);
  saida[21] = 0;
//  converter_int_char(bcd_para_decimal(ler_registrador(DS3231_SEG)), &saida[19]);
//  saida[21] = 0;
//  Serial.println("******");
//  for (int i=0; i<32; i++)
//  {
//    Serial.print(saida[i]);
//    Serial.print(" ");
//  }
//  Serial.println("\n******");
}

void converter_int_char(unsigned int dado, char *saida)
{
  saida[0] = (dado/10) + 0x30;
  saida[1] = (dado%10) + 0x30;
}

byte bcd_para_decimal(byte bcd)
{
  return ((bcd & 0xF0) >> 4)*10 + (bcd & 0x0F);
}

byte decimal_para_bcd(byte decimal)
{
  return (int)((decimal/10)<< 4) + decimal%10;
}

byte converter_hora(byte hora)
{
  if ((hora & 0x20) == 1)//Conforme datasheet: AM/PM indicador
    return bcd_para_decimal(hora & 0x1F)+12;
  return bcd_para_decimal(hora & 0x1F);
}
