#ifndef RADIO_H
#define RADIO_H

#include <SPI.h>
#include <LoRa.h>

//// Parâmetros da comunicação
#define LORA_TIMEOUT 5000
// ID da comunicação
//#define ID_COM 5
//#define ID_unid_sen      0x05 
#define ID_unid_sen      0x04 // Alterar aqui o ID da unidade sensora/// tAMBEM TEM QUE ALTERAR EM MAIN.CPP
#define COD_LAMINA_1 100

// Variaveis globais
extern unsigned char lora_msg_1[11];

// Prototipo das funções
unsigned char lora_receive_data(void);


#endif // RADIO_H
