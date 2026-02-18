#include "W25Q32JV.hpp"
#include "logs.hpp"
#include <Arduino.h>

unsigned long ultimo_log_detectado;
unsigned long primeiro_log_detectado = 0;

/********************************************************************************************************
 * Nome da função: logs_init
 * Descrição: verifica se a memória está saudável através da verificação do JEDEC
 * Parâmetros: void
 * Saída - unsigned long: -1 => Erro: JEDEC falhou
 *                        -2 => Erro: não foi possível localizar o último log registrado
 *                        -3 => Erro: não foi possível localizar o primeiro log
 *                         0 => Sucesso: sistema de logs inicializado
 *******************************************************************************************************/
int logs_init(void)
{
  byte jedec[3];
  memory_init();
  cmd_JEDEC_ID(jedec);

  Serial.println(jedec[0], HEX);
  Serial.println(jedec[1], HEX);
  Serial.println(jedec[2], HEX);
  
  if ((jedec[1] != 0xEF) && (jedec[2] != 0x40))
    return -1;
    
  ultimo_log_detectado = ultimo_log();
  
  if (ultimo_log_detectado == -1)
    return -2;

  primeiro_log_detectado = primeiro_log();

  if (primeiro_log_detectado == -1)
    return -3;

  Serial.print("Primeiro log: ");
  Serial.println(primeiro_log_detectado);

  Serial.print("Ultimo log: ");
  Serial.println(ultimo_log_detectado);

  return 0;
}

/********************************************************************************************************
 * Nome da função: ler_log
 * Descrição: realiza a leitura de um dado log
 * Parâmetros: unsigned long identificacao_log => qual log se deseja ler
 *             byte *saida => onde o log deve ser armazenado
 * Saída - byte:  ERRO_LOG_ID_INVALIDA => Erro: este log não existe na memória
 *                0 => Sucesso: o log foi lido
 *******************************************************************************************************/
byte ler_log(unsigned long identificacao_log, byte *saida)
{
  if (identificacao_log >= QUANTIDADE_LOGS)
    return ERRO_LOG_ID_INVALIDA;
  if (primeiro_log_detectado > ultimo_log_detectado)
  {
    if (identificacao_log+primeiro_log_detectado > QUANTIDADE_LOGS)
      cmd_read_data(nlog_para_endereco(identificacao_log+primeiro_log_detectado-QUANTIDADE_LOGS), saida, LOG_SIZE);
    else
      cmd_read_data(nlog_para_endereco(identificacao_log+primeiro_log_detectado), saida, LOG_SIZE);
  }
  else
    cmd_read_data(nlog_para_endereco(identificacao_log), saida, LOG_SIZE);
  return 0;
}

/********************************************************************************************************
 * Nome da função: adicionar_log
 * Descrição: adiciona um novo log a memória
 * Parâmetros: byte *dados => vetor contendo os dados a serem gravados
 * Saída - byte: LOG_GRAVADO_SUCESSO => log gravado com sucesso
 *******************************************************************************************************/
byte adicionar_log(byte *dados)
{
//  cmd_page_program(nlog_para_endereco(ultimo_log_detectado), dados, LOG_SIZE);
  
  Serial.println("*****");
  Serial.println(primeiro_log_detectado);
  Serial.println(ultimo_log_detectado);
  Serial.println("*****");
  if (nlog_para_endereco(ultimo_log_detectado) >= ENDERECO_FINAL) //Chegou ao final da memória?
  {
    ultimo_log_detectado = 0; //Volta o ponteiro para o topo
    primeiro_log_detectado = endereco_para_nlog(LOGS_POR_SETOR*LOG_SIZE+HEADER);
    cmd_sector_erase(nlog_para_endereco(0)); //Limpa o primeiro setor de logs da memória
    Serial.println("Novo ciclo");
  }
    
  if (is_ultimo_log_setor(nlog_para_endereco(ultimo_log_detectado))) //Detecta se este é o último log do setor
  {
    Serial.println("Novo setor. Limpando...");
    if (nlog_para_endereco(ultimo_log_detectado+1) >= ENDERECO_FINAL) //É o último setor da memória?
    {
      cmd_sector_erase(nlog_para_endereco(0)); //Limpa o primeiro setor de logs
      primeiro_log_detectado = endereco_para_nlog(LOGS_POR_SETOR*LOG_SIZE+HEADER);
      //ultimo_log_detectado = 0;
    }      
    else
    {
      cmd_sector_erase(nlog_para_endereco(ultimo_log_detectado+1)); //Limpa o próximo setor
      
      if (primeiro_log_detectado > ultimo_log_detectado)
        primeiro_log_detectado = (ultimo_log_detectado+1) + LOGS_POR_SETOR;
        
      if (primeiro_log_detectado >= QUANTIDADE_LOGS)
        primeiro_log_detectado = 0;
        
//      cmd_sector_erase(nlog_para_endereco(ultimo_log_detectado+1)); //Limpa o próximo setor
//      if (primeiro_log_detectado > ultimo_log_detectado)
//        primeiro_log_detectado = ultimo_log_detectado+LOGS_POR_SETOR;
    }
      
  }
  
  cmd_page_program(nlog_para_endereco(ultimo_log_detectado), dados, LOG_SIZE);
  ultimo_log_detectado ++;
  if (ultimo_log_detectado >= QUANTIDADE_LOGS)
    ultimo_log_detectado = 0;
  return LOG_GRAVADO_SUCESSO;
}

/********************************************************************************************************
 * Nome da função: ultimo_log
 * Descrição: verifica qual foi o último log registrado na memória
 * Parâmetros: void
 * Saída - long: ultimo log vazio => Sucesso: a detecção funcionou
 *                        -1 => Erro: não foi possível ler a memória
 *******************************************************************************************************/
long ultimo_log(void)
{
  unsigned long endereco;
  byte temp[LOG_SIZE];
  bool log_vazio;
  for (endereco=HEADER; endereco<ENDERECO_FINAL; endereco+=LOG_SIZE)
  {
//    Serial.println("*****");
//    Serial.println(endereco, HEX);
//    Serial.println("*****");
    log_vazio = true;
    if (cmd_read_data(endereco, temp, LOG_SIZE) == CMD_SUCESSO)
    {
      for (int i=0; i<LOG_SIZE; i++)
      {
        //Serial.println(temp[i], HEX);
        if (temp[i] != 0xFF) //Indica que essa posição de memória está limpa
          log_vazio = false; //Este log nao esta vazio
      }
    }
    else
      return -1;
    if (log_vazio)
      break; //Nao ha necessidade de continuar: o ultimo log vazio foi encontrado
  }
  return endereco_para_nlog(endereco);
}

/********************************************************************************************************
 * Nome da função: primeiro_log
 * Descrição: verifica qual foi o primeiro log registrado na memória
 * Parâmetros: void
 * Saída - long: primeiro log registrado => Sucesso: a detecção funcionou
 *                        -1 => Erro: não foi possível ler a memória
 *******************************************************************************************************/
long primeiro_log(void)
{
  unsigned long endereco;
  byte saida[LOG_SIZE];
  bool log_vazio;
  byte status_mem = 1; // 1 => não iniciado ainda

  for (unsigned long endereco=HEADER; endereco<ENDERECO_FINAL; endereco+=LOG_SIZE)
  {
    log_vazio = true; //Reinicia o flag
    if (cmd_read_data(endereco, saida, LOG_SIZE) == CMD_SUCESSO)
    {
      for (int i=0; i<LOG_SIZE; i++)
      {
        if (saida[i] != 0xFF)
        {
          log_vazio = false; //Um log preenchido foi detectado
          if (status_mem == 2) //Existe um log preenchido, depois um vazio, depois um preenchido
          {
            Serial.println("A memoria esta em loop");
            return endereco_para_nlog(endereco);
          }
//          else
//            status_mem = 1; //1 => um log preenchido está presente
        }
        if (i == LOG_SIZE-1) //Está na última posição do log
        {
          if ((saida[i] == 0xFF) && log_vazio && (status_mem == 1))
            status_mem = 2; // 2 => existe um log preenchido e um vazio após
        }
//        if ((log_vazio && (saida[i] != 0xFF))
//        {
//          Serial.println("A memoria esta em loop");
//          return endereco_para_nlog(endereco);
//        }
      }
    }
    else
      return -1; //Erro na leitura da memoria
  }
  return 0; //O primeiro log está na primeira posição da memória
}

/********************************************************************************************************
 * Nome da função: endereco_para_nlog
 * Descrição: converte um valor de endereço para um valor de log (entre 0 e QUANTIDADE_LOGS)
 * Parâmetros: unsigned long endereco => endereço a ser convertido
 * Saída - unsigned long: nlog => valor do log 
 *******************************************************************************************************/
unsigned long endereco_para_nlog(unsigned long endereco)
{
  return (unsigned long)((endereco-HEADER)/LOG_SIZE);
}

/********************************************************************************************************
 * Nome da função: nlog_para_endereco
 * Descrição: converte um valor de log (entre 0 e QUANTIDADE_LOGS) para um valor de endereço
 * Parâmetros: unsigned long nlog => valor de log a ser convertido
 * Saída - unsigned long: endereco => endereço convertido 
 *******************************************************************************************************/
unsigned long nlog_para_endereco(unsigned long nlog)
{
  return nlog*LOG_SIZE+HEADER;
}

/********************************************************************************************************
 * Nome da função: apagar_todos_logs
 * Descrição: apaga todos os logs da memória
 * Parâmetros: void
 * Saída: void
 *******************************************************************************************************/
void apagar_todos_logs(void)
{
  unsigned long add;
  byte resposta;
//  for (add=HEADER; add<ENDERECO_FINAL; add+=0x1000)
  for (add=HEADER; add<HEADER+0x1000; add+=0x1000)
  {
    if (cmd_sector_erase(add) == FALHA_ERASE_SECTOR)
      Serial.println("Falha");
  }
  ultimo_log_detectado = 0;
  primeiro_log_detectado = 0;
}

/*********************** HEADER ****************************/
/********************************************************************************************************
 * Nome da função: adicionar_item_header
 * Descrição: adiciona novo item no header
 * Parâmetros: byte *dados => dados a serem inseridos no header
 * Saída - byte: ERRO_HEADER_CHEIO => Erro: o header está cheio
 *               HEADER_GRAVADO => Sucesso: o header foi gravado
 *******************************************************************************************************/
byte adicionar_item_header(byte *dados)
{
  long endereco = procurar_header_vazio();
  if (endereco == ERRO_HEADER_CHEIO)
    return ERRO_HEADER_CHEIO;
  cmd_page_program(endereco, dados, HEADER_DATA_SIZE);
  return HEADER_GRAVADO;
}

/********************************************************************************************************
 * Nome da função: procurar_header_vazio
 * Descrição: busca uma posição no header que esteja vazia
 * Parâmetros: void
 * Saída - long: -1 => Erro: não foi possível ler a memória 
 *                ERRO_HEADER_CHEIO => Erro: não há mais espaço disponível no header
 *******************************************************************************************************/
long procurar_header_vazio(void)
{
  unsigned long endereco;
  byte saida[HEADER_DATA_SIZE];
  bool header_vazio;
  for (endereco=0; endereco<HEADER; endereco+=HEADER_SIZE)
  {
    header_vazio = true;
    if (cmd_read_data(endereco, saida, HEADER_DATA_SIZE) == CMD_SUCESSO)
    {
      for (int i=0; i<HEADER_DATA_SIZE; i++)
      {
        if (saida[i] != 0xFF)
          header_vazio = false; //Este header nao esta vazio
      }
    }
    else
      return -1;
    if (header_vazio)
      break; //Nao ha necessidade de continuar: a ultima posicao do header foi encontrada
  }
  if (endereco >= HEADER)
    return ERRO_HEADER_CHEIO; //Nao tem mais header disponivel
  return endereco;
}

/********************************************************************************************************
 * Nome da função: obter_header
 * Descrição: obtém todos os dados do header
 * Parâmetros: byte *saida => onde os dados serão salvos
 * Saída - unsigned int: quantidade => quantidade de registros no header
 *******************************************************************************************************/
unsigned int obter_header(byte *saida)
{
  unsigned int quantidade = 0;
  unsigned long posicao;
  for (unsigned long i=0; i<HEADER; i+=0x1000)
  {
    if (!is_header_vazio(i))
    {
      quantidade++;
      posicao = i*(unsigned long)(HEADER_DATA_SIZE)/0x1000UL;
//      Serial.println("obter-header");
//      Serial.println(posicao);
//      Serial.println(i*HEADER_DATA_SIZE/0x1000);
//      Serial.println(i*(int)(HEADER_DATA_SIZE));
      cmd_read_data(i, &saida[posicao], HEADER_DATA_SIZE);
//      for (int i=0; i<50; i++)
//      {
//        Serial.print(saida[i], HEX);
//      }
    }
  }
  return quantidade;
}

/********************************************************************************************************
 * Nome da função: quantidade_header
 * Descrição: verifica quantos registros existem no header da memória
 * Parâmetros: void
 * Saída - unsigned int: quantidade => quantidade de registros no header 
 *******************************************************************************************************/
unsigned int quantidade_header(void)
{
  unsigned int quantidade = 0;
  for (int i=0; i<HEADER; i+=0x1000)
  {
    if (!is_header_vazio(i))
      quantidade++;
  }
  return quantidade;
}

/********************************************************************************************************
 * Nome da função: is_header_vazio
 * Descrição: verifica se um determinado registro do header está vazio
 * Parâmetros: unsigned long endereco => endereço a ser avaliado
 * Saída - bool: false => esse registro não está vazio 
 *               true => esse registro está vazio
 *******************************************************************************************************/
bool is_header_vazio(unsigned long endereco)
{
  byte ptr[HEADER_DATA_SIZE];
  cmd_read_data(endereco, ptr, sizeof(ptr));
//  Serial.println("\n\n");
//  for (int j=0; j<sizeof(ptr); j++)
//  {
//    Serial.print(ptr[j], HEX);
//    Serial.print(" ");
//  }
//  Serial.println("\n\n");
  for (int i=0; i<HEADER_DATA_SIZE; i++)
  {
    if (ptr[i] != 0xFF)
      return false;
  }
  return true;
}

/********************************************************************************************************
 * Nome da função: procurar_matricula
 * Descrição: busca a posição de memória onde esta matrícula se encontra
 * Parâmetros: unsigned long matricula
 * Saída: endereço => caso a matrícula tenha sido localizada dentro da memória
 *        -1 => caso a matrícula não tenha sido localizada
 *******************************************************************************************************/
long procurar_matricula(unsigned long matricula)
{
  unsigned int quantidade = 0;
  unsigned long posicao;
  byte matricula_b[HEADER_DATA_SIZE];
  unsigned long matricula_ul;
  for (unsigned long i=0; i<HEADER; i += 0x1000)
  {
    if (!is_header_vazio(i))
    {
//      quantidade++;
//      posicao = i*HEADER_DATA_SIZE/0x1000UL;
      cmd_read_data(i, matricula_b, HEADER_DATA_SIZE);
      matricula_ul = (matricula_b[4]<<16) + (matricula_b[5]<<8) + (matricula_b[6]);
      if (matricula_ul == matricula)
        return i;
    }
  }
  return -1;
}

/********************************************************************************************************
 * Nome da função: pegar_validade
 * Descrição: pega a validade de um registro no header
 * Parâmetros: unsigned long endereco => endereço de onde se deseja retirar a validade
 * Saída - unsigned long: validade em segundos (epoch) 
 *******************************************************************************************************/

unsigned long pegar_validade(unsigned long endereco)
{
  byte buffer_saida[HEADER_DATA_SIZE];
  cmd_read_data(endereco, buffer_saida, HEADER_DATA_SIZE);
//  Serial.println(buffer_saida[0]<<24) + (buffer_saida[1]<<16) + (buffer_saida[2]<<8) + (buffer_saida[3]);
  return (buffer_saida[0]<<24) + (buffer_saida[1]<<16) + (buffer_saida[2]<<8) + (buffer_saida[3]);
}
//void deletar_log()

/********************************************************************************************************
 * Nome da função: is_ultimo_log_setor
 * Descrição: verifica se o determinado endereço é referente ao último log do setor
 * Parâmetros: unsigned long endereco => endereço que se deseja analisar
 * Saída - bool: false => não é o último log do setor
 *               true => é o último log do setor
 *******************************************************************************************************/
bool is_ultimo_log_setor(unsigned long endereco)
{
  return (((endereco - HEADER + LOG_SIZE) % 0x1000) == 0);
    //return true;
  //return false;
}

/********************************************************************************************************
 * Nome da função: quantidade_logs
 * Descrição: calcula quantos logs existem na memória
 * Parâmetros: void
 * Saída - unsigned int: quantidade => quantidade de logs
 *******************************************************************************************************/
unsigned int quantidade_logs(void)
{
  if (primeiro_log_detectado > ultimo_log_detectado)
    return QUANTIDADE_LOGS - (primeiro_log_detectado - ultimo_log_detectado);
  return ultimo_log_detectado;
}
