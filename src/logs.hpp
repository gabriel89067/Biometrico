#ifndef LOGS_H
#define LOGS_H

#include "W25Q32JV.hpp"
#include <Arduino.h>


//Constantes
#define LOG_SIZE                                   8
#define QUANTIDADE_USUARIOS                      350
#define HEADER          (QUANTIDADE_USUARIOS*0x1000)
#define HEADER_SIZE                           0x1000 //Nome + validade + matricula
#define HEADER_DATA_SIZE                          48

//#define QUANTIDADE_LOGS (ENDERECO_FINAL+1-HEADER)/LOG_SIZE //Quantos logs a memoria permite
#define QUANTIDADE_LOGS       3000
#define ENDERECO_FINAL        (QUANTIDADE_LOGS*LOG_SIZE+HEADER)
#define ENDERECO_FINAL_MAX    0x3FFFFE
#define LOGS_POR_SETOR        (0x1000/LOG_SIZE)

//Códigos de erro e sucesso
#define ERRO_LOG_ID_INVALIDA     1
#define ERRO_HEADER_CHEIO       -2

#define LOG_GRAVADO_SUCESSO      1
#define LOG_GRAVADO_FALHA        2

#define HEADER_GRAVADO           3

int logs_init(void);
byte ler_log(unsigned long identificacao_log, byte *saida);
byte adicionar_log(byte *dados);
long ultimo_log(void);
long primeiro_log(void);
unsigned long endereco_para_nlog(unsigned long endereco);
unsigned long nlog_para_endereco(unsigned long nlog);
void apagar_todos_logs(void);
byte adicionar_item_header(byte *dados);
long procurar_header_vazio(void);
unsigned int obter_header(byte *saida);
unsigned int quantidade_header(void);
bool is_header_vazio(unsigned long endereco);
long procurar_matricula(unsigned long matricula);
unsigned long pegar_validade(unsigned long endereco);
bool is_ultimo_log_setor(unsigned long endereco);
unsigned int quantidade_logs(void);

#endif