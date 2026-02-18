#ifndef BIOMETRICO_H
#define BIOMETRICO_H

#include <Arduino.h>
/***********************************************************************************
 * Nome: obter_versao_firmware 
 * Descricao: obter a versao atual do firmware da placa do sensor.
 * Saida: versao (unsigned int) => MSB.LSB
 * 
 * Exemplo: versao igual a 221 (em hex) significa versao 2.21
 * 
 ***********************************************************************************/

/************************
 * Protótipos de funções
 ************************/
//Funções de conversão
void long_to_byte(long, byte *);
void long_to_char(long, byte *);
byte char_to_long(byte *, unsigned int, unsigned long *);

void construir_frame(byte *, long, long, long, long, byte *);
byte ler_resposta(byte *, unsigned int);
void enviar_comando(byte *, unsigned int);
void iniciar_serial_biometria(void);
byte cadastrar_biometria(long, byte);
byte excluir_biometria(unsigned int);

byte obter_lista_usuarios(void);
byte entrar_modo_mestre(void);
byte obter_versao_firmware(void);
byte verificar_status(void);
byte mudar_auto_identificacao(bool);
byte parar_execucao(void);

byte ler_resultado_auto_identificacao(unsigned long *);
void limpar_buffer(void);
byte excluir_todos_usuarios(bool);
void pre_comando(void);
void pos_comando(void);

void carregarConfiguracoes(void);



#define RX_PIN 32 //32
#define TX_PIN 33 //33

#define START_BYTE                  0x7E

#define TAMANHO_FRAME                 25
#define TAMANHO_FRAME_DADOS           56
#define TAMANHO_NOVA_DIGITAL          27
#define TAMANHO_FRAME_EXCLUSAO        40 //Exclusao de matricula
#define TAMANHO_FPID                  11

#define TAMANHO_MAX_MATRICULA         10
#define TAMANHO_MIN_MATRICULA          4

#define MAX_TENTATIVAS_CONEXAO       100

#define FALHA_FIRMWARE              0x02
#define FALHA_MODO_MESTRE           0x03
#define MATRICULA_INVALIDA          0x05
#define FALHA_CONEXAO               0x06
#define ERRO_DIGITAIS_DIFERENTES    0x08
#define RESULT_NADA_DISPONIVEL      0x09
#define RESULT_FP_NAO_ENCONTRADA    0x0A

//Modos de operacao
#define IDLE_MODE                   0x00
#define BUSY_MODE                   0x01
#define DB_UPLOADING_MODE           0x02
#define AUTO_IDENTIFY_MODE          0x03

#define RESULT_SUCCEEDED            0x01
#define RESULT_FAILED               0x02
#define RESULT_NOT_MASTER_MODE      0x03
#define RESULT_USED_ID              0x04
#define RESULT_NOT_IN_TIME          0x07
#define RESULT_ANOTHER_FINGER       0x0E
#define RESULT_INVALID_DATA         0x16

#define CMD_GET_FIRMWARE_VERSION2   0x04
#define CMD_CANCEL                  0x17
#define CMD_AUTO_IDENTIFY           0x1A
#define CMD_AUTO_IDENTIFY_RESULT    0x1B
#define CMD_DELETE_FP               0x22
#define CMD_DELETE_ALL_FP           0x23
#define CMD_ENTER_MASTER_MODE2      0x2F
#define CMD_GET_FP_LIST2            0x30
#define CMD_REGISTER_MULTI_FP       0x38
#define CMD_STATUS_CHECK            0x62

#endif