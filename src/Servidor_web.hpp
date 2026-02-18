#ifndef SERVIDOR_WEB_H
#define SERVIDOR_WEB_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "Biometrico.hpp"
#include "DS3231.hpp"
#include <Arduino.h>
#include "logs.hpp"
//#include "ESPAsyncWebServer.h"

#define SERVIDOR_INICIADO_SUCESSO 0
#define SERVIDOR_ERRO_SPIFFS 1

#define MAX_USUARIOS 350

void verifica_login();
void usuarios_conectados();
byte setup_servidor();
void check_client();
void obter_usuario();
void criar_usuario();
void atualizar_usuario();
void deletar_usuario();
void obter_quantidade_logs(void);
void obter_logs(void);
void obter_rtc(void);
void alterar_rtc(void);
void handle_NotFound();
void formatar_dados(String nome, unsigned long matricula, unsigned long nivel,unsigned long validade, byte *saida);
void apagar_logs();
void carregarConfiguracoes();
void configurar_senha();
#endif