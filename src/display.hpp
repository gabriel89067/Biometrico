#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>

#define STRING_TAMANHO_MAX 32
#define N_COLUNAS          16
#define N_LINHAS            2

#define I2C_ENDERECO     0x27

void display_init(void);
void display_escrever_completo(char *ptr);
void display_definir_backlight(bool ligado);
void display_limpar(void);

#endif