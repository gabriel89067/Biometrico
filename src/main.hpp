#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>
#include "Servidor_web.hpp"
#include "Biometrico.hpp"
#include "logs.hpp"
#include "DS3231.hpp"
#include "display.hpp"
#include "radio.hpp"
#include <EEPROM.h>
//O correto
#define ID_unid_sen      0x04 // Alterar aqui o ID da unidade sensora/// tAMBEM TEM QUE ALTERAR EM RADIO.HPP
#define ID_prod_pur      10 // ID produto pur
#define ID_prod_bio      32 // ID biometrico
#define SS               12  // GPIO12
#define RST              27  // GPIO27
#define DIO0             14  // GPIO14
#define RX_ESP           6   // GPIO06
#define TX_ESP           5   // GPIO05

#define LED_BLOQ         26
#define LED_LIB          25

void setup();
void loop();
void atualizar_display();
void lora_envia_abertura(void);
void lora_envia_fechamento(void);
void lora_obter_status(void);
void pisca_led(unsigned int quantidade);
void lora_reset();






#endif