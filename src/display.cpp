#include "display.hpp"
#include <Arduino.h>

LiquidCrystal_I2C lcd(I2C_ENDERECO, N_COLUNAS, N_LINHAS);
bool status_backlight = 0;

/********************************************************************************************************
 * Nome da função: display_init
 * Descrição: inicializa o display LCD
 * Parâmetros: void
 * Saída: void
 *******************************************************************************************************/
void display_init(void)
{
  lcd.begin(N_COLUNAS, N_LINHAS,LCD_5x8DOTS);
  lcd.backlight();
//  lcd.setCursor(0,0);
//  lcd.print("Sistema pronto");
}

/********************************************************************************************************
 * Nome da função: display_escrever_completo
 * Descrição: escreve no display um vetor de char e considera que pode haver um \n neste intervalo
 * Parâmetros: char *ptr => vetor de dados a serem escritos
 * Saída: void
 *******************************************************************************************************/
void display_escrever_completo(char *ptr)
{
  //display_limpar();
  unsigned int tamanho=0;
  while ((ptr[tamanho] != 0) && (tamanho <= STRING_TAMANHO_MAX))
    tamanho++;
    
  char linhas[N_LINHAS][N_COLUNAS+1];
  for (int i=0; i<N_LINHAS; i++) //Limpa as duas linhas
  {
    for (int j=0; j<N_COLUNAS; j++)
      linhas[i][j] = ' ';
  }
  //Limpa o display
//  lcd.setCursor(0,0);
//  lcd.print(linhas[0]);
//  lcd.setCursor(0,1);
//  lcd.print(linhas[1]);
  display_limpar();

//  if (tamanho <= N_COLUNAS)
//  {
//    for (i=0; i<N_COLUNAS; i++)
//    {
//      if (ptr[i] == 0x0A)
//        break;
//      linhas[0][i] = ptr[i];
//    }
//    for (j=i+1; j<N_COLUNAS*2; j++)
//      linhas[1][j-i-1] = ptr[j];
//  }
//  else
//  {
    int i, j;
    for (i=0; i<N_COLUNAS; i++)
    {
      if (ptr[i] == 0x0A)
        break;
      linhas[0][i] = ptr[i];
    }
    for (j=i+1; j<N_COLUNAS*2; j++)
      linhas[1][j-i-1] = ptr[j];
//  }
  linhas[0][N_COLUNAS] = 0;
  linhas[1][N_COLUNAS] = 0;
  lcd.setCursor(0,0);
  lcd.print(linhas[0]);
  lcd.setCursor(0,1);
  lcd.print(linhas[1]);
}

/********************************************************************************************************
 * Nome da função: display_definir_backlight
 * Descrição: define o status do backlight do display
 * Parâmetros: bool ligado => define se o backlight deve ficar ligado ou desligado
 * Saída: void
 *******************************************************************************************************/
void display_definir_backlight(bool ligado)
{
  if (ligado)
    lcd.backlight();
  else
    lcd.noBacklight();
}

/********************************************************************************************************
 * Nome da função: display_limpar
 * Descrição: limpa as duas linhas do display
 * Parâmetros: void
 * Saída: void
 *******************************************************************************************************/
void display_limpar(void)
{
  lcd.clear();
//  lcd.setCursor(0,0);
//  lcd.print("                ");
//  lcd.setCursor(0,1);
//  lcd.print("                ");
}
