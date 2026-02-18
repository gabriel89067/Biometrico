#ifndef W25Q32JV_H
#define W25Q32JV_H

#include <SPI.h>
#include <Arduino.h>
#include "logs.hpp"
//Inicializacao
#define CS_PIN               13
#define SPI_SPEED      20000000
#define N_TENTATIVAS      10000 //Numero de tentativas de apagar um setor

//Constantes
//#define ENDERECO_FINAL         0x3FFFFE //4 MB

//Códigos de erro/sucesso
#define TRANSMISSAO_OK                1
#define CMD_SUCESSO                   0
#define FALHA_ERASE_SECTOR            6
#define FALHA_PAGE_PROGRAM            2
#define FALHA_WRITE_PROBLEM           3
#define FALHA_TAMANHO_INVALIDO        4
#define FALHA_FIM_MEMORIA             5

//Comandos
#define CMD_GLOBAL_UNLOCK          0x98
#define CMD_JEDEC_ID               0x9F
#define CMD_PAGE_PROGRAM           0x02
#define CMD_READ_DATA              0x03
#define CMD_SECTOR_ERASE           0x20
#define CMD_READ_REGISTER1         0x05
#define CMD_READ_REGISTER2         0x35
#define CMD_READ_REGISTER3         0x15
#define CMD_WRITE_DISABLE          0x04
#define CMD_WRITE_ENABLE           0x06
#define CMD_WRITE_REGISTER1        0x01
#define CMD_WRITE_REGISTER2        0x31
#define CMD_WRITE_REGISTER3        0x11

//Bits
#define BUSY_BIT                   0x01
#define WEL_BIT                    0x02

void memory_init();
byte enviar_comando_memoria(byte *buffer_dados, unsigned int tamanho);
void copiar_conteudo(byte *origem, byte *destino, unsigned int tamanho);
void limpar_buffer(byte *ptr, unsigned int tamanho);
byte cmd_global_unlock(void);
byte cmd_JEDEC_ID(byte *saida);
byte cmd_page_program(unsigned long endereco, byte *dados, unsigned int tamanho);
byte cmd_read_data(unsigned long endereco, byte *saida, unsigned long quantidade);
byte cmd_read_status(byte *saida);
byte cmd_sector_erase(unsigned long endereco);
byte cmd_write_disable(void);
byte cmd_write_enable(void);
byte cmd_write_status(byte dado);
bool is_busy(void);
bool is_new_sector(unsigned long endereco);
bool is_write_enable(void);

#endif