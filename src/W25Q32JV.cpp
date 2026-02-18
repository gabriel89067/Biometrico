#include <SPI.h>
#include <Arduino.h>
#include "W25Q32JV.hpp"

void memory_init() {
  pinMode(CS_PIN, OUTPUT);
  //SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0));
  //SPI.begin();
  digitalWrite(CS_PIN, HIGH); //Mantém o chip desabilitado
  delay(1000);
}

byte enviar_comando_memoria(byte *buffer_dados, unsigned int tamanho)
{
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(buffer_dados, tamanho);
  digitalWrite(CS_PIN, HIGH);
  
  return CMD_SUCESSO;
}

void copiar_conteudo(byte *origem, byte *destino, unsigned int tamanho)
{
  if (tamanho <= 0)
    return;
  for (int i=0; i<tamanho; i++)
    destino[i] = origem[i];
}

void limpar_buffer(byte *ptr, unsigned int tamanho)
{
  for (unsigned int i=0; i<tamanho; i++)
    ptr[i] = 0;
}

/************** FUNÇÕES ESPECÍFICAS ***********************/
byte cmd_global_unlock(void)
{
  byte comando = CMD_GLOBAL_UNLOCK, retorno;
  retorno = enviar_comando_memoria(&comando, sizeof(comando));
  return retorno;
}

byte cmd_JEDEC_ID(byte *saida)
{
  byte comando[4], retorno;
  comando[0] = CMD_JEDEC_ID;
  retorno = enviar_comando_memoria(comando, 3);
  copiar_conteudo(comando, saida, 3);
  return retorno;
}

byte cmd_page_program(unsigned long endereco, byte *dados, unsigned int tamanho)
{
  if ((tamanho > 256) || (tamanho == 0))
    return FALHA_TAMANHO_INVALIDO;
    
  byte comando[4+tamanho], retorno;
  unsigned int i=0;
  
  cmd_write_enable(); //Ativa o modo de escrita
  comando[0] = CMD_PAGE_PROGRAM;
  comando[1] = (byte)((endereco & 0xFF0000) >> 16);
  comando[2] = (byte)((endereco & 0xFF00) >> 8);
  comando[3] = (byte)((endereco & 0xFF));
  
  for (unsigned int i = 0; i < tamanho; i++) {
  Serial.print("Dado[");
  Serial.print(i);
  Serial.print("]: 0x");
  Serial.println(dados[i], HEX);
  Serial.print(" | Char: ");
  Serial.println((char)dados[i]);
  }

  for (int j=4; j<tamanho+4; j++) //Acrescenta dados a comando
    comando[j] = dados[j-4];
  
  for (unsigned int i = 0; i < tamanho+4; i++) {
  Serial.print("Dado[");
  Serial.print(i);
  Serial.print("]: 0x");
  Serial.println(comando[i], HEX);
  Serial.print(" | Char: ");
  Serial.println((char)comando[i]);
  }
  
  enviar_comando_memoria(comando, 4 + tamanho);
  while (is_busy()) //O processo de gravacao nao terminou ainda
  {
    i++;
    if (i >= N_TENTATIVAS)
      return FALHA_PAGE_PROGRAM;
    delay(1);
  }
  return CMD_SUCESSO;
}

byte cmd_read_data(unsigned long endereco, byte *saida, unsigned long quantidade)
{
  if (quantidade == 0)
    return FALHA_TAMANHO_INVALIDO;
    
  if (endereco + (quantidade-1) > ENDERECO_FINAL)
    quantidade = ENDERECO_FINAL - endereco;

  if (quantidade == 0)
    return FALHA_FIM_MEMORIA;
    
  byte comando[4+quantidade], retorno;
  
  comando[0] = CMD_READ_DATA;
  comando[1] = (byte)((endereco & 0xFF0000) >> 16);
  comando[2] = (byte)((endereco & 0xFF00) >> 8);
  comando[3] = (byte)((endereco & 0xFF));
  
  retorno = enviar_comando_memoria(comando, sizeof(comando));

  for (int i=4; i<sizeof(comando); i++)
    saida[i-4] = comando[i];
//  saida[0] = comando[4];
  
  return retorno;
}

byte cmd_read_status(byte *saida)
{
  byte comando[2], retorno;

  comando[0] = CMD_READ_REGISTER1;
  enviar_comando_memoria(comando, sizeof(comando));
  saida[0] = comando[1];

  comando[0] = CMD_READ_REGISTER2;
  enviar_comando_memoria(comando, sizeof(comando));
  saida[1] = comando[1];

  comando[0] = CMD_READ_REGISTER3;
  retorno = enviar_comando_memoria(comando, sizeof(comando));
  saida[2] = comando[1];
  
  return retorno;
}

byte cmd_sector_erase(unsigned long endereco)
{
  byte comando[4], retorno;
  unsigned int i=0;
  
  cmd_write_enable();
  comando[0] = CMD_SECTOR_ERASE;
  comando[1] = (byte)((endereco & 0xFF0000) >> 16);
  comando[2] = (byte)((endereco & 0xFF00) >> 8);
  comando[3] = (byte)((endereco & 0xFF));
  
  enviar_comando_memoria(comando, sizeof(comando));
  while (is_busy()) //O processo de apagar o setor nao terminou ainda
  {
    i++;
    if (i >= N_TENTATIVAS)
    {
      Serial.println("-----");
      return FALHA_ERASE_SECTOR;
    }
    delay(1);
  }
  return CMD_SUCESSO;
}

byte cmd_write_disable(void)
{
  byte comando = CMD_WRITE_DISABLE;
  enviar_comando_memoria(&comando, sizeof(comando));    
  return CMD_SUCESSO;
}

byte cmd_write_enable(void)
{
  byte comando = CMD_WRITE_ENABLE;
  enviar_comando_memoria(&comando, sizeof(comando));    
  return CMD_SUCESSO;
}

byte cmd_write_status(byte dado)
{
  byte comando[2];
  comando[0] = CMD_WRITE_REGISTER1;
  comando[1] = dado;
  enviar_comando_memoria(comando, sizeof(comando));
  delay(50);
  while(is_busy());
  return 0;
}

//Funções complementares

bool is_busy(void)
{
  byte comando[3];
  cmd_read_status(comando);
  return ((comando[0] & BUSY_BIT) == BUSY_BIT);
}

bool is_new_sector(unsigned long endereco)
{
  return ((endereco % 0x1000) == 0);
}

bool is_write_enable(void)
{
  byte comando[3];
  cmd_read_status(comando);
  return ((comando[0] & WEL_BIT) == WEL_BIT);
}
