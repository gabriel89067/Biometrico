
#include "main.hpp"



char Packet[3];
char msg_log[30];
unsigned int ID_SISTEMA = 1;
byte ip[10];
uint8_t bufferTx[9];

uint32_t time_now = millis();

unsigned int resposta;
unsigned long matricula, matricula_end, validade;
bool status_sistema = true;
byte buffer_saida[100];
unsigned int LED_PIN = 2;
char data_hora[32];
unsigned long duracao;
//unsigned int quantidade_reinicios = 0;
//bool apagar_quantidade_reinicios = false;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(2); //Duas posicoes de memoria serao ocupadas
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_BLOQ, OUTPUT);
  pinMode(LED_LIB, OUTPUT);
  pisca_led(10);

//  Serial.println("Verificando o contador");
//  if (apagar_quantidade_reinicios)
//  {
//    EEPROM.write(0, 0);
//    EEPROM.write(1, 0);
//    EEPROM.commit();
//    Serial.println("Apagando o contador de reinicios");
//  }
//  Serial.println(EEPROM.read(0));
//  Serial.println(EEPROM.read(1));
//  quantidade_reinicios = (EEPROM.read(0) << 8) + EEPROM.read(1);
//  quantidade_reinicios++;
//  EEPROM.write(0, (quantidade_reinicios & 0xFF00) >> 8);
//  EEPROM.write(1, (byte)quantidade_reinicios);
//  EEPROM.commit();

//  Serial.print(quantidade_reinicios);
//  Serial.println(" reinicializacoes ate' agora");
    DS3231_init();

  display_init();

  display_escrever_completo("Iniciando\nservidor");
  
  setup_servidor();
  Serial.println("Iniciando sistema de biometria");
  display_escrever_completo("Iniciando\nbiometria");
  iniciar_serial_biometria();
  mudar_auto_identificacao(false);
  //delay(1000);
//  display_escrever_completo("Iniciando\nRTC");
 
  //delay(1000);
//  display_escrever_completo("Modo automatico\nbiometrico");
  mudar_auto_identificacao(true);
  
//  altera_status_led_rapido(false);
//  display_escrever_completo("Posicione o\ndedo no leitor");
  //delay(1000);
//  Serial.println("Sistema pronto");
  duracao = millis();
  bufferTx[0] = ID_prod_pur;               //CABEÇALHO DE SINCRONISMO 1_ Produto PUR
  bufferTx[1] = ID_prod_bio;               //CABEÇALHO DE SINCRONISMO 2_ Produto Biometrico (0x20 ou 32)
  bufferTx[2] = ID_unid_sen ;              //ID unidade sensora biometrico
  bufferTx[3] = 0;                         //NAO SE APLICA - PODE SER UTILIZADO PARA MELHORIAS FUTURAS
  bufferTx[4] = 0;                         //NAO SE APLICA - PODE SER UTILIZADO PARA MELHORIAS FUTURAS
  bufferTx[5] = 0x4C;                      //COMANDO BIOMETRICO - AÇAO A SER TOMADA
  bufferTx[6] = 0xFF;                      //NAO SE APLICA - PODE SER UTILIZADO PARA MELHORIAS FUTURAS
  bufferTx[7] = 0XFF;                      //NAO SE APLICA - PODE SER UTILIZADO PARA MELHORIAS FUTURAS
  //display_escrever_completo("Iniciando\nTransmissor");
  //delay(1000);
  Serial.println("LoRa Sender");
  pinMode(SS, OUTPUT);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6))
  {
    Serial.println(".");
    Serial.println("Starting LoRa failed!");
    display_escrever_completo("Falha de HW\nTransmissor");
    while (1);
  }
  else
  {
    LoRa.setPreambleLength(20);
    LoRa.setSyncWord(53);
    //display_escrever_completo("Preambulo\nLora");
    //delay(1000);
    Serial.println("Preambulo e sync word definidos.");

    if (LoRa.beginPacket())
    {
      //display_escrever_completo("Pre\nend packet");
      //delay(1000);
      LoRa.write(bufferTx[0]);
      LoRa.endPacket();
      //display_escrever_completo("Pos\nend packet");
      //delay(1000);
    }
  }
  //  LoRa.onReceive(onReceive);
  //display_escrever_completo("Pre\nreceive");
  //delay(1000);
  LoRa.receive();
  //display_escrever_completo("Iniciando o\nsistema de logs");
  //delay(1000);
  int resposta = logs_init();
  if (resposta == 0)
    Serial.println("Sistema de logs inicializado");
  else
  {
    Serial.println("Sistema de logs com falhas. Erro: ");
    Serial.println(resposta);
    display_escrever_completo("Falha de HW\nMemoria");
    while (1);
  }
  //delay(1000);
  display_escrever_completo("Obtendo o status\ndo sistema");
  //delay(500);
  lora_obter_status();
  //display_escrever_completo("Sistema foi\nquestionado");
  unsigned int aguardando = millis();
  while (!lora_receive_data() && (millis() - aguardando < 10000));
  if (millis() - aguardando >= 10000) {
    display_escrever_completo("Unidade remota\nnao respondeu");
    while(1);
  }
  else {
    if (lora_msg_1[8] == 0x4C)
    {
      Serial.println("Status: liberado");
      status_sistema = false;
      display_escrever_completo("Unidade remota\nliberada");
    }
    else
    {
      Serial.println("Status: bloqueado");
      status_sistema = true;
      display_escrever_completo("Unidade remota\nbloqueada");
    }
    digitalWrite(LED_PIN, !status_sistema);
  }
  pisca_led(10);
  display_escrever_completo("Sistema\ninicializado");
  delay(2000);
  atualizar_display();
  digitalWrite(LED_BLOQ, status_sistema);
  digitalWrite(LED_LIB, !status_sistema);
  Serial.println("********\nSistema pronto\n********");
}
void loop()
{
//  data_hora[24] = 0;
  byte quantidade_clientes = WiFi.softAPgetStationNum();
  if (quantidade_clientes == 0)
  {
    if (millis() - duracao >= 1000)
      atualizar_display();
  }
  else
  {
    if (millis() - duracao >= 1000)
    {
      display_escrever_completo(" Acesso remoto:\n    ativado");
      duracao = millis();
    }
      
  }
  
  
//  Serial.println(data_hora);
//  display_escrever_completo(data_hora); //Essa parte comentada esta dando problemas: a conexao fica reiniciando
//  delay(1000);
//  display_escrever_completo("Sistema pronto");
//  display_escrever_completo("aaaaaa bbbbbb ccccccc dddddddd");
//  Serial.println(quantidade_header());
//  delay(10000);
//  cmd_read_data(0, buffer_saida, 100);
//  for (int i=0; i<100; i++)
//  {
//    Serial.print(buffer_saida[i], HEX);
//    Serial.print(" ");
//  }
//  Serial.println("\n***");
//  for (int i=0; i<resposta*HEADER_DATA_SIZE; i++)
//  for (int i=0; i<1; i++)
//  {
//    Serial.print(saida[i], HEX);
//    if (i%HEADER_DATA_SIZE == 0)
//      Serial.println("\n\n");
//  }
//  delay(10000);
//  Serial.println();
  check_client();

  //Descomente este bloco para testar o reconhecimento automático de digital
  resposta = ler_resultado_auto_identificacao(&matricula);
  if (resposta == RESULT_SUCCEEDED)
  {
    matricula_end = procurar_matricula(matricula);
    Serial.print("\nUsuario identificado. Matricula: ");
    Serial.println(matricula);
    if (matricula_end != -1)
    {
      unsigned long epoch = obter_data_horario();
      validade = pegar_validade(matricula_end);
      if ((validade >= epoch) || (validade == 0)) //Validade ok ou indefinida (0)
      {
        Serial.println("Usuario tem validade OK");
//        status_sistema = !status_sistema;
//        altera_status_led_rapido(status_sistema);
        
        byte saida[8];
//        for (int i=0; i<1015; i++)
//        {
        saida[0] = (byte)((epoch & 0xFF000000) >> 24);
        saida[1] = (byte)((epoch & 0xFF0000) >> 16);
        saida[2] = (byte)((epoch & 0xFF00) >> 8);
        saida[3] = (byte)((epoch & 0xFF));
      
        saida[4] = (byte)((matricula & 0xFF0000) >> 16);
        saida[5] = (byte)((matricula & 0xFF00) >> 8);
        saida[6] = (byte)((matricula & 0xFF));
    
        saida[7] = (byte)status_sistema;
//        for (int i=0; i<1023; i++) {
//          adicionar_log(saida);
//          Serial.println(i);
//        }
          
        
//          Serial.println(i);
//          epoch++;
//        }
//        for (int i=0; i<500; i++) {
//          adicionar_log(saida);
//          Serial.println(i);
//        }
        if (status_sistema) 
        {
          display_escrever_completo("Enviando comando\nAguarde");
          lora_envia_abertura();
          //delay(2000);
          unsigned int aguardando = millis();
          while (!lora_receive_data() && (millis() - aguardando < 10000));
          if (millis() - aguardando >= 10000) 
          {
              Serial.println("Timeout 1: unidade remota ainda nao respondeu");
              display_escrever_completo("Aguarde mais\num pouco");
              lora_reset();
              lora_envia_abertura();
              unsigned int aguardando = millis();
              while (!lora_receive_data() && (millis() - aguardando < 10000));
              if (millis() - aguardando >= 10000) 
              {
                Serial.println("Timeout 2: unidade remota nao respondeu");
                display_escrever_completo("Timeout\nTente novamente");
                delay(2000);
                display_escrever_completo("Unidade remota\nnao respondeu");
                lora_reset();
              }
              else 
              {
                display_escrever_completo("Acesso liberado\nEquip. ligado");
                digitalWrite(LED_PIN, true);
                status_sistema = false;
                digitalWrite(LED_BLOQ, status_sistema);
                digitalWrite(LED_LIB, !status_sistema);
                lora_reset();
              }
          }
          else 
          {
            Serial.println("Abertura concluida: feedback recebido");
            lora_reset();
            if (adicionar_log(saida) == LOG_GRAVADO_SUCESSO) 
            {
              Serial.println("Log gravado");
              display_escrever_completo("Acesso liberado\nEquip. ligado");
              digitalWrite(LED_PIN, true);
              status_sistema = false;
              digitalWrite(LED_BLOQ, status_sistema);
              digitalWrite(LED_LIB, !status_sistema);
             }              
            else
              Serial.println("Log nao gravado");
          }
            
          //if (lora_receive_data()) {
            //Serial.println("Recebido retorno");
            //if (lora_msg[8] == 0x4C) {
            //  Serial.println("Equipamento ligado: feedback recebido");
            //}
          //}
          //else
          //  Serial.println("Nenhum feedback");
        }
          
        else {
          display_escrever_completo("Enviando comando\nAguarde");
          lora_envia_fechamento();
          //delay(2000);
          unsigned int aguardando = millis();
          while (!lora_receive_data() && (millis() - aguardando < 10000));
          if (millis() - aguardando >= 10000) {
            Serial.println("Timeout 1: unidade remota ainda nao respondeu");
            display_escrever_completo("Aguarde mais\num pouco");
            lora_reset();
            lora_envia_fechamento();
            unsigned int aguardando = millis();
            while (!lora_receive_data() && (millis() - aguardando < 10000));
            if (millis() - aguardando >= 10000) {
              Serial.println("Timeout 2: unidade remota nao respondeu");
              display_escrever_completo("Timeout\nTente novamente");
              delay(2000);
              display_escrever_completo("Unidade remota\nnao respondeu");
              lora_reset();
            }
            else {
              display_escrever_completo("Acesso liberado\nEquip. desligado");
              digitalWrite(LED_PIN, false);
              status_sistema = true;
              digitalWrite(LED_BLOQ, status_sistema);
              digitalWrite(LED_LIB, !status_sistema);
              lora_reset();
            }
          }
          else {
            Serial.println("Abertura concluida: feedback recebido");
            lora_reset();
            if (adicionar_log(saida) == LOG_GRAVADO_SUCESSO) {
              Serial.println("Log gravado");
              display_escrever_completo("Acesso liberado\nEquip. desligado");
              digitalWrite(LED_PIN, false);
              status_sistema = true;
              digitalWrite(LED_BLOQ, status_sistema);
              digitalWrite(LED_LIB, !status_sistema);
            }              
            else
              Serial.println("Log nao gravado");
          }
//          display_escrever_completo("Acesso liberado\nEquip. desligado");
//          lora_envia_fechamento();
//          status_sistema = true;
//          delay(2000);
//          if (lora_receive_data()) {
//            if (lora_msg[8] == 0x44) {
//              Serial.println("Equipamento desligado: feedback recebido");
//            }
//          }
        }
                  
        delay(2000);
        atualizar_display();
      }
      else
      {
        Serial.println("Usuario tem validade expirada");
//          altera_status_led_lento(status_sistema);
        display_escrever_completo("Esta digital\nesta' expirada");
        delay(2000);
        atualizar_display();
      }          
    }    
  }
  else if (resposta == RESULT_FP_NAO_ENCONTRADA)
  {
    Serial.println("Digital lida, mas essa pessoa nao possui cadastro");
    display_escrever_completo("Acesso negado\n             ");
    delay(2000);
    atualizar_display();
  }
    
    

  //Descomente este bloco para testar o cadastro de biometria
  /*pre_comando();
  resposta = entrar_modo_mestre();
  if (resposta == RESULT_SUCCEEDED)
  {
    matricula = 1239;
    resposta = cadastrar_biometria(matricula);
    if (resposta == RESULT_SUCCEEDED)
      Serial.println("Cadastrado");
    else if (resposta == RESULT_USED_ID)
      Serial.println("Matricula ja existe");
    else
    {
      Serial.print("Erro generico: ");
      Serial.println(resposta);
    }
  }
  pos_comando();*/

  //Descomente este bloco para excluir todos os usuários
  /*pre_comando();
  resposta = entrar_modo_mestre();
  if (resposta == RESULT_SUCCEEDED)
  {
    resposta = excluir_todos_usuarios(false);
    if (resposta == RESULT_SUCCEEDED)
      Serial.println("Todos os usuarios foram deletados");
    else
      Serial.println(resposta);
  }*/
  
//  delay(2000);
}

//void altera_status_led_rapido(bool status_led)
//{
//  for (int i=0; i<10; i++)
//  {
//    digitalWrite(LED_PIN, !(digitalRead(LED_PIN)));
//    delay(100);
//  }
//  digitalWrite(LED_PIN, status_led);
//}
//
//void altera_status_led_lento(bool status_led)
//{
//  for (int i=0; i<10; i++)
//  {
//    digitalWrite(LED_PIN, !(digitalRead(LED_PIN)));
//    delay(500);
//  }
//  digitalWrite(LED_PIN, status_led);
//}

void atualizar_display()
{
  data_hora[0] = ' ';
  data_hora[1] = ' ';
  data_hora[2] = ' ';
  data_hora[3] = ' ';
  obter_data_horario_string(&data_hora[4]);
  display_escrever_completo(data_hora); //Essa parte comentada esta dando problemas: a conexao fica reiniciando
//  display_escrever_completo("Posicione o dedo\nno leitor");
//  byte contador[6];
//  long_to_char(quantidade_reinicios, contador);
//  contador[4] = '\n';
//  contador[5] = 0;
//  display_escrever_completo((char*)contador);
  duracao = millis();
}

void lora_envia_abertura(void)
{
  bufferTx[5] = 0x4C;
  bufferTx[0] = ID_prod_pur ;
  bufferTx[1] = ID_prod_bio;
  bufferTx[2] = ID_unid_sen ;//ID unidade sensora biometrico
      
  if (LoRa.beginPacket())
  {
    LoRa.write(bufferTx[0]);
    LoRa.write(bufferTx[1]);
    LoRa.write(bufferTx[2]);
    LoRa.write(bufferTx[3]);
    LoRa.write(bufferTx[4]);
    LoRa.write(bufferTx[5]);
    LoRa.write(bufferTx[6]);
    LoRa.write(bufferTx[7]);
    LoRa.endPacket();
    Serial.println("Sending packet");
    Serial.println("Liga");
    Serial.println("");
    LoRa.receive();
  }
  else
    Serial.println("Falha na abertura");
  
}

void lora_envia_fechamento(void)
{
  bufferTx[5] = 0x44;
  bufferTx[0] = ID_prod_pur ;
  bufferTx[1] = ID_prod_bio;
  bufferTx[2] = ID_unid_sen ;//ID unidade sensora biometrico
    
  if (LoRa.beginPacket())
  {
    LoRa.write(bufferTx[0]);
    LoRa.write(bufferTx[1]);
    LoRa.write(bufferTx[2]);
    LoRa.write(bufferTx[3]);
    LoRa.write(bufferTx[4]);
    LoRa.write(bufferTx[5]);
    LoRa.write(bufferTx[6]);
    LoRa.write(bufferTx[7]);
    LoRa.endPacket();
    Serial.println("Sending packet");
    Serial.println("Desliga");
    Serial.println("");
    LoRa.receive();
  }
  else
    Serial.println("Falha na abertura");
}

void lora_obter_status(void)
{
  bufferTx[5] = 0x4F;
  bufferTx[0] = ID_prod_pur ;
  bufferTx[1] = ID_prod_bio;
  bufferTx[2] = ID_unid_sen ;//ID unidade sensora biometrico
  
  if (LoRa.beginPacket())
  {
    LoRa.write(bufferTx[0]);
    LoRa.write(bufferTx[1]);
    LoRa.write(bufferTx[2]);
    LoRa.write(bufferTx[3]);
    LoRa.write(bufferTx[4]);
    LoRa.write(bufferTx[5]);
    LoRa.write(bufferTx[6]);
    LoRa.write(bufferTx[7]);
    LoRa.endPacket();
    Serial.println("Sending packet");
    Serial.println("Obtendo o status");
    Serial.println("");
    LoRa.receive();
  }
  else
    Serial.println("Falha na obtencao do status");
}

void pisca_led(unsigned int quantidade)
{
  for (unsigned int i=0; i<quantidade; i++)
  {
    digitalWrite(LED_PIN, true);
    delay(100);
    digitalWrite(LED_PIN, false);
    delay(100);
  }
  digitalWrite(LED_PIN, !status_sistema);
}

void lora_reset(){
  // perform reset
    digitalWrite(RST, LOW);
    delay(10);
    digitalWrite(RST, HIGH);
    delay(10);

    LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6))
  {
    Serial.println("Starting LoRa failed!");
    display_escrever_completo("Falha de\nhardware");
    while (1);
  }
  else
  {
    LoRa.setPreambleLength(20);
    LoRa.setSyncWord(53);

    if (LoRa.beginPacket())
    {
      LoRa.write(bufferTx[0]);
      LoRa.endPacket();
    }
  }
  //  LoRa.onReceive(onReceive);
  LoRa.receive();
//  OK_D = 1;
}

