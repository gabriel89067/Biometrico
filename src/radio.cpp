/*
 * Código para interfacear com o LoRa
 * Autor: Fabio Guimarães
 */
#include "radio.hpp"
unsigned char lora_msg_1[11];

/*
 *  Retorna 1 se receber resposta OK do receptor
 */
unsigned char lora_receive_data()
{
  unsigned char *buf = &lora_msg_1[0];
  uint32_t timeout = millis();

  int packetSize = LoRa.parsePacket();

  if (packetSize) // Se recebeu algo
  {
    while (LoRa.available() && (millis() - timeout) < LORA_TIMEOUT && (millis() - timeout) >= 0)
    {
      *buf = (char)LoRa.read();
      buf++;
    }

    // Verifica se o ID é igual
    //if ((lora_msg[2] == ID_COM) && (lora_msg[0] == 10) && (lora_msg[1] == 32))
    if ((lora_msg_1[0] == 10)&&(lora_msg_1[1] == 32) && (lora_msg_1[2] == ID_unid_sen ))
    {
      return 1;
    }
  }

  return 0;
}
