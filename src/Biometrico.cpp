#include "Biometrico.hpp"

void long_to_byte(long dado, byte *ptr)
{
  ptr[0] = byte((dado & 0xFF000000) >> 24);
  ptr[1] = byte((dado & 0xFF0000) >> 16);
  ptr[2] = byte((dado & 0xFF00) >> 8);
  ptr[3] = byte(dado & 0xFF);
}

void long_to_char(long dado, byte *ptr)
{
  unsigned int tamanho = TAMANHO_MAX_MATRICULA, cont = 0;
  byte tamanho_dado = 0;
  unsigned long divisor;
  bool conversao_iniciada = false; //Somente para eliminar zeros a esquerda

  if (dado <= 9) //Preenche com tres zeros a esquerda
  {
    ptr[0] = 0x30; ptr[1] = 0x30; ptr[2] = 0x30; ptr[3] = 0x30 + dado; 
  }
  else if (dado <= 99) //Preenche com dois zeros a esquerda
  {
    ptr[0] = 0x30; ptr[1] = 0x30; ptr[2] = int(dado/10) + 0x30; ptr[3] = int(dado%10) + 0x30;
  }
  else if (dado <= 999) //Preenche com um zero a esquerda
  {
    ptr[0] = 0x30; ptr[1] = int(dado/100) + 0x30; ptr[2] = int((dado%100)/10) + 0x30; ptr[3] = int((dado%100)%10) + 0x30;
  }
  else //1000 ou maior => sem preenchimento
  {
    while(tamanho > 0)
    {
      divisor = pow(10, tamanho-1);
      if ((int(dado/divisor) == 0) && !conversao_iniciada) //O numero e' menor que esse fator
        tamanho --;
      else
      {
        ptr[cont] = byte(int(dado/divisor) + 0x30);
        cont++; //Avanca para a proxima posicao do vetor ptr
        tamanho--; //Avanca para o proximo digito
        dado = dado % divisor; //Atualiza o novo valor de entrada
        
        conversao_iniciada = true; //Nao ha mais zeros a esquerda
      }
    }
  }
}

byte char_to_long(byte *ptr, unsigned int tamanho, unsigned long *saida)
{
  unsigned int posicao = tamanho;
  unsigned long saida_temp = 0;
  int i;
  for (i=tamanho-1; i>=0; i--)
  {
    if (ptr[i] != 0)
    {
      posicao = i;
      break;
    }
  }
  if (i < 0)
    return RESULT_FAILED; //A ID é inválida (veio apenas 0x0)
  for (i=0; i<=posicao; i++)
    saida_temp += (ptr[i]-0x30)*pow(10, posicao-i); //Dá o peso para cada parte do número
  *saida = saida_temp;
  return RESULT_SUCCEEDED;
}

void construir_frame(byte *ptr, long comando, long param1, long param2, long data_size, byte *dados = NULL)
{
  for (int i=0; i<TAMANHO_FRAME; i++) //Limpa frame
    ptr[i] = 0;
    
  ptr[0] = START_BYTE; //Start byte
  long_to_byte(comando, &ptr[1]); //4 bytes de comando
  long_to_byte(param1, &ptr[5]); //4 bytes de param1
  long_to_byte(param2, &ptr[9]); //4 bytes de param2
  long_to_byte(data_size, &ptr[13]); //4 bytes de data_size
  long_to_byte(0L, &ptr[17]); //4 bytes de error_code (sempre 0 no envio)

  long checksum = 0L;
  for (int i=1; i<TAMANHO_FRAME - 4; i++) //Calcula checksum
    checksum += ptr[i];

  long_to_byte(checksum, &ptr[21]); //Registra checksum
  if (dados != NULL) //Um ponteiro foi passado
  {
    //Serial.println("Ponteiro valido");
    for (int i=TAMANHO_FRAME; i<TAMANHO_FRAME+data_size; i++)
      ptr[i] = dados[i-TAMANHO_FRAME]; //Carrega dados em ptr
    checksum = 0L;
    for (int i=0; i<data_size; i++) //Calcula checksum de dados
      checksum += dados[i];
    long_to_byte(checksum, &ptr[TAMANHO_FRAME + data_size]); //Registra checksum
  }
}

//TODO adequar o tamanho do quadro com base no datasize da resposta
byte ler_resposta(byte *ptr, unsigned int tamanho)
{
  int resposta = 0, cont = 0;
  
  for (int i=0; i<tamanho; i++)
  {
    resposta = Serial2.read();
    while ((resposta == -1) && (cont < MAX_TENTATIVAS_CONEXAO))
    {
      resposta = Serial2.read();
      cont++;
      delay(100);
    }
    if ((resposta != START_BYTE) && (i == 0))
    {
      limpar_buffer();
      parar_execucao();
      return RESULT_FAILED; //Veio lixo
    }
      
    if (cont == MAX_TENTATIVAS_CONEXAO)
      return FALHA_CONEXAO;
    ptr[i] = resposta;
  }
  return RESULT_SUCCEEDED;
}

void enviar_comando(byte *ptr, unsigned int tamanho)
{
  for (int i=0; i<tamanho; i++)
    Serial2.write(ptr[i]);
}

void iniciar_serial_biometria(void)
{
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  delay(100);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

byte cadastrar_biometria(long matricula, byte qual_dedo=0)
{
  byte dados[TAMANHO_FRAME_DADOS];
  byte informacao_usuario[TAMANHO_NOVA_DIGITAL];
  byte resposta;
  
  for (int i=0; i<TAMANHO_NOVA_DIGITAL; i++) //Limpa o vetor
    informacao_usuario[i] = 0;
  long_to_char(matricula, informacao_usuario);
  
  construir_frame(dados, long(CMD_REGISTER_MULTI_FP), 0L, long(qual_dedo<<4), long(TAMANHO_NOVA_DIGITAL), informacao_usuario);
  enviar_comando(dados, TAMANHO_FRAME_DADOS);

  if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO) //O sensor esta desconectado ou com problemas de comunicacao
    return FALHA_CONEXAO;
  if (dados[8] == RESULT_NOT_MASTER_MODE) //O sistema nao esta no modo mestre
    return RESULT_NOT_MASTER_MODE;
  if (dados[8] == RESULT_INVALID_DATA) //Os dados nao foram compreendidos
    return RESULT_INVALID_DATA;
  if (dados[8] == RESULT_SUCCEEDED)
  {
//    Serial.println("Aceitou o primeiro cadastro");
    delay(3000); //Aguarda um tempo para a pessoa remover o dedo do leitor
    construir_frame(dados, long(CMD_REGISTER_MULTI_FP), 0L, 3L, 0L);
    enviar_comando(dados, TAMANHO_FRAME);
    if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO)
      return FALHA_CONEXAO;
//    Serial.println(dados[8]);
    switch(dados[8])
    {
      case RESULT_SUCCEEDED:
        return RESULT_SUCCEEDED;
      case RESULT_ANOTHER_FINGER:
        return RESULT_ANOTHER_FINGER;
      default:
        Serial.println(dados[8]);
        return RESULT_FAILED;
    }
  }
  return dados[8];  
}

byte excluir_biometria(unsigned int matricula)
{
  byte dados[TAMANHO_FRAME_EXCLUSAO];
  byte informacao[TAMANHO_MAX_MATRICULA+1]; //+1 por causa do NULL byte
  byte resposta = entrar_modo_mestre();
  if (resposta == RESULT_SUCCEEDED)
  {
    for (int i=0; i<sizeof(dados); i++)
      dados[i] = 0;
    for (int i=0; i<sizeof(informacao); i++)
      informacao[i] = 0;
    long_to_char(matricula, informacao);
    construir_frame(dados, long(CMD_DELETE_FP), 0L, 0L, long(TAMANHO_MAX_MATRICULA)+1, informacao);
    enviar_comando(dados, sizeof(dados));
    for (int i=0; i<sizeof(dados); i++)
      Serial.println(dados[i]);
    obter_lista_usuarios();
    if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO)
        return FALHA_CONEXAO; 
    return dados[8]; 
    obter_lista_usuarios();
    
  }
  return RESULT_FAILED;   
}

//TODO precisa de melhorias => ler_resposta precisa ser adaptada
byte obter_lista_usuarios(void)
{
  byte dados[TAMANHO_FRAME];
  for (int i=0; i<sizeof(dados); i++)
    dados[i] = 0;
  construir_frame(dados, long(CMD_GET_FP_LIST2), 0L, 0L, 0L);
  enviar_comando(dados, sizeof(dados));
  for (int i=0; i<sizeof(dados); i++)
      Serial.println(dados[i]);
  if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO)
      return FALHA_CONEXAO;
  return RESULT_SUCCEEDED;
}

byte entrar_modo_mestre(void)
{
  byte dados[TAMANHO_FRAME];
  unsigned int cont;
  construir_frame(dados, long(CMD_ENTER_MASTER_MODE2), 3L, 0L, 0L);
  enviar_comando(dados, TAMANHO_FRAME);
  if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO)
    return FALHA_CONEXAO;
    
  if (dados[8] == RESULT_SUCCEEDED)
    return RESULT_SUCCEEDED;
  
  return FALHA_MODO_MESTRE;
  
}

byte obter_versao_firmware(void)
{
  byte dados[TAMANHO_FRAME];
  construir_frame(dados, CMD_GET_FIRMWARE_VERSION2, 0L, 0L, 0L);
  enviar_comando(dados, TAMANHO_FRAME);
  if (ler_resposta(dados, TAMANHO_FRAME) == FALHA_CONEXAO)
    return FALHA_CONEXAO;

  if (dados[8] == RESULT_SUCCEEDED)
    return (dados[11] << 8) + dados[12];

  return FALHA_FIRMWARE;
}

byte verificar_status(void)
{
  byte dados[TAMANHO_FRAME];
  construir_frame(dados, CMD_STATUS_CHECK, 0L, 0L, 0L);
  enviar_comando(dados, sizeof(dados));
  ler_resposta(dados, sizeof(dados));
  return dados[12]; //Byte de resultado. Verifique Biometrico.h para os valores de status
}

byte mudar_auto_identificacao(bool ativado)
{
  byte dados[TAMANHO_FRAME];
  construir_frame(dados, CMD_AUTO_IDENTIFY, long(ativado), 0L, 0L);
  enviar_comando(dados, sizeof(dados));
  ler_resposta(dados, sizeof(dados));
  return dados[8];
}

byte parar_execucao(void)
{
  byte dados[TAMANHO_FRAME];
  construir_frame(dados, CMD_CANCEL, 0L, 0L, 0L);
  enviar_comando(dados, sizeof(dados));
  ler_resposta(dados, sizeof(dados));
  return RESULT_SUCCEEDED;
}

byte ler_resultado_auto_identificacao(unsigned long *saida)
{
  byte dado[TAMANHO_FRAME-1]; //-1 porque começa em 0
  byte resposta;
  int leitura;
  
  leitura = Serial2.read();
  if (leitura == -1)
    return RESULT_NADA_DISPONIVEL;
  else if (leitura == START_BYTE)
  {
    for (int i=0; i<sizeof(dado); i++)
    {
      dado[i] = byte(Serial2.read());
    }
    if (dado[7] == RESULT_SUCCEEDED)
    {
      for (int i=0; i<TAMANHO_FPID; i++)
        dado[i] = byte(Serial2.read());
      for (int i=0; i<4; i++) //Ignora o CRC de dados
        Serial2.read();
      return char_to_long(dado, TAMANHO_FPID, saida);
    }
    return RESULT_FP_NAO_ENCONTRADA; //A digital foi lida mas nao existe nenhum usuario associado
  }
  else //Houve um problema de sincronismo e veio lixo
  {
    Serial.print("\nAlgo estranho: ");
    Serial.println(leitura);
  }
  return RESULT_NADA_DISPONIVEL;
}

void limpar_buffer(void)
{
  Serial.println("Limpando buffer");
  while(Serial2.read() != -1);
}

byte excluir_todos_usuarios(bool mestres_tambem=false)
{
  byte dados[TAMANHO_FRAME];
  byte resposta = entrar_modo_mestre();
  if (resposta == RESULT_SUCCEEDED)
  {
    if (mestres_tambem)
      construir_frame(dados, CMD_DELETE_ALL_FP, 0L, 0L, 0L);
    else
      construir_frame(dados, CMD_DELETE_ALL_FP, 1L, 0L, 0L);
    enviar_comando(dados, sizeof(dados));
    ler_resposta(dados, sizeof(dados));
  //  Serial.println(dados[8]);
    return dados[8];
  }
  return RESULT_FAILED;
}

void pre_comando(void)
{
  Serial.println("Pre-comando");
  parar_execucao();
  limpar_buffer();
  mudar_auto_identificacao(false);
}

void pos_comando(void)
{
  Serial.println("Pos-comando");
  parar_execucao();
  limpar_buffer();
  mudar_auto_identificacao(true);
}
