#include "Servidor_web.hpp"
#include "Biometrico.hpp"
#include "DS3231.hpp"
#include <Arduino.h>
#include "logs.hpp"

WebServer server(80);
bool server_logado = false;
String ssid_wifi, login_admin, senha_admin, senha_wifi;
unsigned int matriculas[MAX_USUARIOS];
String nomes[MAX_USUARIOS];
unsigned long epoch_atual = 1593779905UL;
void usuarios_conectados()
{
  Serial.println("Usuarios conectados: " + String(WiFi.softAPgetStationNum()));
  for (int i = 0; i < WiFi.softAPgetStationNum(); i++)
  {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}

byte setup_servidor()
{

  if (!SPIFFS.begin(true))
  {
    Serial.println("Falha ao montar SPIFFS");
    return SERVIDOR_ERRO_SPIFFS;
  }
  Serial.println("montar SPIFFS");

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    Serial.print(file.name());
    Serial.print(" | ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
  carregarConfiguracoes();
  // Set WiFi mode to AP and disable station mode
  WiFi.mode(WIFI_AP);
  WiFi.disconnect(false);
  delay(100);

  if (WiFi.softAP(ssid_wifi, senha_wifi))
  {
    Serial.println("Ponto de acesso criado");
  }
  else
  {
    Serial.println("Falha ao criar ponto de acesso");
    WiFi.printDiag(Serial);
  } // Nome da rede e senha

  delay(1000); // Obrigatorio -> Sem esse delay, a CPU 0 entrara em panic'ed

  IPAddress local_ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  if (WiFi.softAPConfig(local_ip, gateway, subnet))
  {
    Serial.println("Configuração de rede bem sucedida");
    Serial.print("IP do servidor: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("SSID: Biometrico_5");
    Serial.print(" - Clientes conectados: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
  else
  {
    Serial.println("Configuração de rede falhou");
    WiFi.printDiag(Serial);
  } // Configuração da rede
  delay(1000); // Obrigatorio -> Sem esse delay, a CPU 0 entrara em panic'ed

  server.enableCrossOrigin(true); // Habilita diferentes origens - Dev apenas
  server.enableCORS(true);        // Habilita CORS - Dev apenas
  // server.on("/", HTTP_GET, handler_home);
  // File paginaInicial = SPIFFS.open("/home.html");
  Serial.println("OKOKOKOK SERVER");

  server.serveStatic("/", SPIFFS, "/pag_login.html"); // Pagina inicial

  // Scripts
  server.serveStatic("/papaParse.js", SPIFFS, "/papaParse.js");
  server.serveStatic("/usuariosRequisicao.js", SPIFFS, "/usuariosRequisicao.js");

  server.on("/login/", HTTP_POST, verifica_login); ////////////

  server.on("/usuarios/", HTTP_GET, obter_usuario);
  server.on("/usuarios/", HTTP_POST, criar_usuario);
  server.on("/usuarios/", HTTP_PUT, atualizar_usuario);
  server.on("/usuarios/", HTTP_DELETE, deletar_usuario);
  server.on("/logs/quantidade/", HTTP_GET, obter_quantidade_logs);
  server.on("/rtc/", HTTP_GET, obter_rtc);
  server.on("/rtc/", HTTP_POST, alterar_rtc);

  server.on("/login/", HTTP_POST, verifica_login); ////////////

  server.on("/configurar/", HTTP_POST, configurar_senha); ///////////////

  server.on("/apagar/", HTTP_POST, apagar_logs);

  server.on("/logs/", HTTP_GET, obter_logs);

  // HTTP_POST, HTTP_PUT também servem
  server.onNotFound(handle_NotFound);

  for (int i = 0; i < MAX_USUARIOS; i++)
  {
    matriculas[i] = i;
    nomes[i] = "Pessoa " + String(i);
    // Serial.println("Dados " + String(nomes[i]) );
  }

  server.begin();
  Serial.println("Servidor iniciado");
  return SERVIDOR_INICIADO_SUCESSO;
}

void verifica_login()
{

  String login_admin_1, senha_admin_1;

  login_admin_1 = server.arg("login_admin");
  senha_admin_1 = server.arg("senha_admin");
  Serial.println("login :" + login_admin_1);
  Serial.println("senha :" + senha_admin_1);
  if (login_admin_1 == login_admin && senha_admin_1 == senha_admin)
  {
    Serial.println("entrou 1");
    server.serveStatic("/pag_home.html", SPIFFS, "/pag_home.html");
    server.serveStatic("/pag_datalog.html", SPIFFS, "/pag_datalog.html");
    server.serveStatic("/pag_cadastro.html", SPIFFS, "/pag_cadastro.html");
    server.serveStatic("/pag_usuarios.html", SPIFFS, "/pag_usuarios.html");
    server.serveStatic("/pag_rtc.html", SPIFFS, "/pag_rtc.html");
    server.serveStatic("/pag_contato.html", SPIFFS, "/pag_contato.html");
    server.serveStatic("/pag_sobre.html", SPIFFS, "/pag_sobre.html");

    server.sendHeader("Location", "/pag_home.html",true);
    server.send(302, "text/plain", "");
    server.send(200, "text/json", "{\"codigo\": 0}");
    Serial.println("Arquivo gravado com sucesso");
    return;
  }
  else
  {
    Serial.println("entrou 2");
    server.send(400, "text/json", "{\"codigo\": 1}"); // senha menor qiue os 8 caracteres
    Serial.println("ERRO 1");
    return;
  }
}

void check_client()
{
  server.handleClient();
  //  delay(1);
}
// TODO existe uma falha quando a quantidade é grande (350, por exemplo)
// Problema aqui, reorganizar memoria
void obter_usuario()
{
  Serial.println("Obtendo usuario");

  String resposta; // Resposta dada como retorno do servidor
  String matricula = server.arg("matricula");
  Serial.println("Matricula: " + String(matricula));
  if (matricula == "") // Deve retornar a lista de todos os usuarios
  {
    resposta = "[";
    unsigned long epoch, matricula_encontrada, nivel;
    String nome = "";

    unsigned int qtd = quantidade_header();
    byte dados_header[qtd * (unsigned long)(HEADER_DATA_SIZE)];
    Serial.print("Qtd: ");
    Serial.println(qtd);
    obter_header(dados_header);
    unsigned int posicao; // quantidade = 0;

    //    Serial.println(qtd);
    for (int i = 0; i < qtd; i++)
    {
      posicao = i * (int)(HEADER_DATA_SIZE);
      Serial.print("posição: ");
      Serial.println(posicao);
      for (int i = 0; i < 7; i++)
      {
        Serial.print("Header: ");
        Serial.println(dados_header[posicao + i]);
      }
      epoch = (dados_header[posicao] << 24) + (dados_header[posicao + 1] << 16) + (dados_header[posicao + 2] << 8) + (dados_header[posicao + 3]);
      matricula_encontrada = (dados_header[posicao + 4] << 16) + (dados_header[posicao + 5] << 8) + (dados_header[posicao + 6]);
      nivel = (dados_header[posicao + 7]);
      posicao += 8;
      String nome((char *)&dados_header[posicao]);

      //      Serial.println(posicao);
      //      Serial.println(epoch);
      //      Serial.println(matricula_encontrada);
      //      Serial.println(i);
      /*for (int j=posicao; j<posicao+40; j++)
        {
        nome.setCharAt(j-posicao, (char*)(dados_header[j]));
        Serial.print(dados_header[j], HEX);
        Serial.print(" ");
        }

        while((dados_header[posicao] != 0) && (quantidade < 40)) //Nao e' null e ainda esta dentro do tamanho especificado
        {
        nome += String(dados_header[posicao]);
        posicao++;
        quantidade++;
        }*/
      if (i == (qtd - 1)) // Ultimo
        resposta += "{\"nome\":\"" + nome + "\","
                                            "\"matricula\":" +
                    String(matricula_encontrada) + ","
                                                   "\"nivel\":" +
                    String(nivel) + ","
                                    "\"epoch\":" +
                    String(epoch) + "}";

      else
        resposta += "{\"nome\":\"" + nome + "\","
                                            "\"matricula\":" +
                    String(matricula_encontrada) + ","
                                                   "\"nivel\":" +
                    String(nivel) + ","
                                    "\"epoch\":" +
                    String(epoch) + "},";
    }
    resposta += "]";
    Serial.println(resposta);
  }
  //  else //Retornar apenas um usuario
  //    resposta = "{\"nome\": \"" + nomes[matricula.toInt()] + "\",\"matricula\":" + matricula.toInt() + ", \"epoch\": 1591372707}";
  server.send(200, "text/json", resposta);
}

void criar_usuario()
{
  unsigned int resposta, tamanho;
  String matricula, nome, nivel, validade, qual_dedo;
  unsigned long matricula_long;
  byte dados[HEADER_DATA_SIZE];

  if (procurar_header_vazio() == ERRO_HEADER_CHEIO)
  {
    server.send(400, "text/json", "{\"codigo\": 7}");
    return;
  }

  matricula = server.arg("matricula");
  nome = server.arg("nome");
  validade = server.arg("epoch");
  nivel = server.arg("nivel");
  qual_dedo = server.arg("qual_dedo");

  Serial.println(matricula);
  Serial.println(nome);
  Serial.println(nivel);
  Serial.println(validade);
  Serial.println(qual_dedo);

  if (validade == "NaN")
    Serial.println("Validade indefinida");

  tamanho = matricula.length();
  matricula_long = matricula.toInt();

  if (tamanho > TAMANHO_MAX_MATRICULA)
  {
    server.send(400, "text/json", "{\"codigo\": 3}"); // ID invalida
    return;
  }

  pre_comando();
  resposta = entrar_modo_mestre();
  if (resposta == RESULT_SUCCEEDED)
  {
    resposta = cadastrar_biometria(matricula_long, 0);
    Serial.println(resposta);
    // Serial.println(matricula.toInt());
    if (resposta == RESULT_SUCCEEDED)
    //    if (true) //Temporario
    {
      formatar_dados(nome, matricula_long, nivel.toInt(), validade.toInt(), dados);
      adicionar_item_header(dados);
      server.send(200, "text/json", "{\"codigo\": 0}"); // Usuario registrado
    }
    else if (resposta == RESULT_USED_ID)
      server.send(400, "text/json", "{\"codigo\": 2}");
    else if (resposta == RESULT_INVALID_DATA)
      server.send(400, "text/json", "{\"codigo\": 3}");
    else if (resposta == RESULT_NOT_IN_TIME)
      server.send(400, "text/json", "{\"codigo\": 4}");
    else if (resposta == RESULT_ANOTHER_FINGER)
      server.send(400, "text/json", "{\"codigo\": 6}");
    else
      server.send(400, "text/json", "{\"codigo\": 5}"); // Erro generico
  }
  else if (resposta == FALHA_CONEXAO)
    server.send(400, "text/json", "{\"codigo\": 1}");
  else
    server.send(400, "text/json", "{\"codigo\": 5}"); // Erro generico
  pos_comando();
}

void atualizar_usuario()
{
  Serial.println("Atualizando usuario");

  String matricula, nome, nivel, validade;

  matricula = server.arg("matricula");
  nome = server.arg("nome");
  nivel = server.arg("nivel");
  validade = server.arg("epoch");

  Serial.println(matricula);
  Serial.println(nome);
  Serial.println(nivel);
  Serial.println(validade);

  long localizacao = procurar_matricula(matricula.toInt());
  if (localizacao == -1) // A matricula nao foi encontrada
  {
    Serial.println("A matricula nao foi localizada");
    server.send(404, "text/json", "");
  }
  else
  {
    byte dados[HEADER_DATA_SIZE];
    formatar_dados(nome, matricula.toInt(), nivel.toInt(), validade.toInt(), dados);
    cmd_sector_erase(localizacao);
    cmd_page_program(localizacao, dados, HEADER_DATA_SIZE);
    Serial.println("A matricula foi encontrada");
    server.send(200, "text/json", "");
  }
}

void deletar_usuario()
{
  long matricula;
  byte resposta;
  unsigned int tamanho;
  long localizacao = 0;
  tamanho = (server.arg("matricula")).length();
  if (tamanho > TAMANHO_MAX_MATRICULA)
  {
    server.send(400, "text/json", "{\"codigo\": 3}");
    return;
  }
  matricula = (server.arg("matricula")).toInt();
  Serial.print("Excluindo matricula: ");
  Serial.println(tamanho);
  Serial.println(matricula);
  pre_comando();
  resposta = excluir_biometria(matricula);
  if (resposta == RESULT_SUCCEEDED)
  {                                              // A matrícula foi encontrada
    localizacao = procurar_matricula(matricula); // Localiza a posição no header onde esta matrícula se encontra
    if (localizacao != -1)
    {                                                   // A posição de memória referente à matrícula foi encontrada
      if (cmd_sector_erase(localizacao) == CMD_SUCESSO) // Exclui o usuário do header
        server.send(200, "text/json", "{\"codigo\": 0}");
      else
        server.send(400, "text/json", "{\"codigo\": 3}"); // Houve um erro durante o processo de apagar da memória
    }
    else
      server.send(400, "text/json", "{\"codigo\": 3}"); // A matrícula não foi encontrada no header da memória
  }
  else if (resposta == FALHA_CONEXAO)
  {
    Serial.println("Falha de conexao");
    server.send(400, "text/json", "{\"codigo\": 1}");
  }
  else
  {
    Serial.print("Falha generica. ");
    Serial.println(resposta);
    server.send(400, "text/json", "{\"codigo\": 3}");
  }
  byte mov_data[HEADER_DATA_SIZE];
  byte next_data[HEADER_DATA_SIZE];
  unsigned int x = 0, b = 0, t = 0;

  while (x == 0)
  {
    b = 0, t = 0;
    cmd_read_data(localizacao, mov_data, HEADER_DATA_SIZE);
    cmd_read_data((localizacao + 0x1000UL), next_data, HEADER_DATA_SIZE);
    Serial.print("\n dados: ");
    for (int y = 0; y < 48; y++)
      Serial.print(mov_data[y]);
    Serial.println("!");
    for (int i = 0; i < HEADER_DATA_SIZE; i++)
    {
      if (next_data[i] == 0xFF)
      {
        t += 1;
      }
      if (mov_data[i] == 0xFF)
      {
        b += 1;
      }
      if (t == 48)
        b = 0;
    }
    Serial.print("\n b e t: ");
    Serial.print(b);
    Serial.print(" e ");
    Serial.println(t);
    if (b == 48)
    {
      localizacao += 0x1000UL;
      cmd_page_program((localizacao - 0x1000UL), next_data, HEADER_DATA_SIZE);
      cmd_sector_erase(localizacao);
      Serial.print("\n Usuario movido: ");
      Serial.println(localizacao);
      Serial.print("\n nxdados: ");
      for (int y = 0; y < 48; y++)
        Serial.print(next_data[y]);
      Serial.println(".");
    }
    else
    {
      x = 1;
    }
  }
  // Serial.println(resposta);
  pos_comando();
}
// TODO por enquanto, só retorna os logs anteriores
void obter_quantidade_logs(void)
{
  //  unsigned long quantidade = 10000;
  //  String saida = String(quantidade);
  //  Serial.print("Ultimo log: ");
  //  Serial.println(ultimo_log());
  Serial.print("Existem ");
  Serial.print(quantidade_logs());
  Serial.println(" logs na memoria");
  server.send(200, "text/json", String(quantidade_logs()));
}

void obter_logs(void)
{
  String saida = "[";
  unsigned long logs_tamanho = 1000, qtd_requisicao, offset, epoch = 1592497210;
  bool acao = true;
  unsigned int contador = 0;
  byte dados[8];
  unsigned long matricula, validade;
  qtd_requisicao = server.arg("quantidade").toInt();
  offset = server.arg("offset").toInt();
  Serial.print("Quantidade: ");
  Serial.println(qtd_requisicao);
  Serial.print("Offset: ");
  Serial.println(offset);
  for (unsigned long i = offset; i < qtd_requisicao + offset; i++)
  {
    ler_log(i, dados);
    validade = (dados[0] << 24) + (dados[1] << 16) + (dados[2] << 8) + dados[3];
    matricula = (dados[4] << 16) + (dados[5] << 8) + dados[6];
    Serial.println(validade);
    Serial.println(matricula);
    Serial.println("%%%%%%");
    for (int i = 0; i < 8; i++)
    {
      Serial.print(dados[i], HEX);
      Serial.print(" ");
    }
    Serial.println("\n%%%%%%%");
    if (i == qtd_requisicao + offset - 1)
      saida += "{\"matricula\":" + String(matricula) + ",\"acao\":" + String(int(dados[7])) + ",\"epoch\": " + String(validade) + "}";
    else
      saida += "{\"matricula\":" + String(matricula) + ",\"acao\":" + String(int(dados[7])) + ",\"epoch\": " + String(validade) + "},";
    //    contador ++;
    //    if (contador == MAX_USUARIOS) //Reinicia o ciclo de matriculas
    //      contador = 0;
    //    acao = !acao;
  }

  saida += "]";
  server.send(200, "text/json", saida);
}

void obter_rtc(void)
{
  Serial.print("RTC atual: ");
  Serial.println(epoch_atual);
  server.send(200, "text/json", "{\"epoch\":" + String(obter_data_horario()) + "}");
}

void alterar_rtc(void)
{
  unsigned long novo_epoch;
  novo_epoch = server.arg("epoch").toInt();
  Serial.print("Novo RTC: ");
  Serial.println(novo_epoch);
  //  if (novo_epoch<1593779905)
  //    server.send(400, "text/json", "");
  //  else
  //  {
  //  epoch_atual = novo_epoch;
  definir_data_horario(novo_epoch);
  server.send(200, "text/json", "{\"epoch\":" + String(obter_data_horario()) + "}");
  //  Serial.println("RTC atualizado");
  //  }
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Nada encontrado");
}

void formatar_dados(String nome, unsigned long matricula, unsigned long nivel, unsigned long validade, byte *saida)
{
  Serial.println(validade);
  Serial.println(nome);
  Serial.println(nivel);
  Serial.println(matricula);

  saida[0] = (byte)((validade & 0xFF000000) >> 24);
  saida[1] = (byte)((validade & 0xFF0000) >> 16);
  saida[2] = (byte)((validade & 0xFF00) >> 8);
  saida[3] = (byte)((validade & 0xFF));

  saida[4] = (byte)((matricula & 0xFF0000) >> 16);
  saida[5] = (byte)((matricula & 0xFF00) >> 8);
  saida[6] = (byte)((matricula & 0xFF));

  saida[7] = (byte)((nivel & 0xFF));

  int tamanho = nome.length();
  if (tamanho > 40)
    tamanho = 40;

  int i;
  for (i = 8; i < 8 + tamanho; i++)
    saida[i] = nome.charAt(i - 8);

  for (int j = i; j < HEADER_DATA_SIZE; j++)
    saida[j] = 0; // Elimina possiveis lixos no buffer

  //  saida[tamanho+7] = 0;

  Serial.println("\n");
  for (int i = 0; i < HEADER_DATA_SIZE; i++)
  {
    Serial.print(saida[i], HEX);
    Serial.print(" ");
  }
  Serial.println("\n");
}

void apagar_logs()
{
  String tipo = server.arg("tipo");
  if (tipo == "log")
  {
    Serial.println("Apagando somente logs");
    for (int i = HEADER; i < ENDERECO_FINAL; i += 0x1000)
    {
      cmd_sector_erase(i);
      Serial.println(i);
    }
  }
  else
  {
    Serial.println("Apagando tudo");
    for (int i = 0; i < ENDERECO_FINAL; i += 0x1000)
    {
      cmd_sector_erase(i);
      Serial.println(i);
    }
    mudar_auto_identificacao(false);
    excluir_todos_usuarios(true);
    mudar_auto_identificacao(true);
  }
  logs_init(); // Reinicia contadores na memoria
  server.send(200, "text/plain", "");
}

void carregarConfiguracoes()
{
  Serial.print("entrou config");
  String linha, chave, valor;
  File arquivo = SPIFFS.open("/Passwords.txt");
  if (!arquivo)
  {
    Serial.println("Falha ao abrir o arquivo de configuracao");
    return;
  }
  while (arquivo.available())
  {
    linha = arquivo.readStringUntil('\n');
    int separador = linha.indexOf('=');
    if (separador != -1)
    {
      chave = linha.substring(0, separador);
      valor = linha.substring(separador + 1);
      valor.trim(); // Remove espaços em branco extras
      Serial.print("Chave: ");
      Serial.print(chave);
      Serial.print(" | Valor: ");
      Serial.println(valor);

      if (chave == "ssid_wifi")
        ssid_wifi = valor;
      if (chave == "senha_wifi")
        senha_wifi = valor;
      if (chave == "login_admin")
        login_admin = valor;
      if (chave == "senha_admin")
        senha_admin = valor;
    }
  }
  arquivo.close();
}

void configurar_senha()
{
  String senha_wifi_1, ssid_wifi_1, login_admin_1, senha_admin_1;
  byte resposta;
  unsigned int tamanho_senha_wifi, tamanho_senha_admin;

  tamanho_senha_wifi = (server.arg("senha_wifi")).length();
  tamanho_senha_admin = (server.arg("senha_admin")).length();

  senha_wifi_1 = server.arg("senha_wifi");
  ssid_wifi_1 = server.arg("ssid_wifi");
  login_admin_1 = server.arg("login_admin");
  senha_admin_1 = server.arg("senha_admin");

  if (tamanho_senha_admin == 0 && tamanho_senha_wifi == 0)
  {
    server.send(400, "text/json", "{\"codigo\": 2}"); // senha menor qiue os 8 caracteres
    Serial.println("ERRO 2");
    return;
  }

  if (tamanho_senha_wifi < 8 && tamanho_senha_admin == 0)
  {
    server.send(400, "text/json", "{\"codigo\": 3}"); // senha menor qiue os 8 caracteres
    Serial.println("ERRO 3");
    return;
  }

  if (tamanho_senha_admin < 5 && tamanho_senha_wifi == 0)
  {
    server.send(400, "text/json", "{\"codigo\": 1}"); // senha menor qiue os 8 caracteres
    Serial.println(tamanho_senha_admin);
    Serial.println("ERRO 1");
    return;
  }

  if (tamanho_senha_admin < 5 && tamanho_senha_wifi < 8)
  {
    server.send(400, "text/json", "{\"codigo\": 4}"); // senha menor qiue os 8 caracteres
    Serial.println(tamanho_senha_admin);
    Serial.println("ERRO 4");
    return;
  }

  File file = SPIFFS.open("/Passwords.txt", FILE_WRITE);
  if (!file)
  {
    Serial.println("Erro ao abrir arquivo para escrita");
    return;
  }
  if (tamanho_senha_admin == 0 && tamanho_senha_wifi != 0)
  {
    file.println("ssid_wifi=" + ssid_wifi_1);
    file.println("senha_wifi=" + senha_wifi_1);
    file.println("login_admin=" + login_admin);
    file.println("senha_admin=" + senha_admin);
  }

  if (tamanho_senha_wifi == 0 && tamanho_senha_admin != 0)
  {
    file.println("ssid_wifi=" + ssid_wifi);
    file.println("senha_wifi=" + senha_wifi);
    file.println("login_admin=" + login_admin_1);
    file.println("senha_admin=" + senha_admin_1);
  }

  if (tamanho_senha_wifi != 0 && tamanho_senha_admin != 0)
  {
    file.println("ssid_wifi=" + ssid_wifi);
    file.println("senha_wifi=" + senha_wifi);
    file.println("login_admin=" + login_admin_1);
    file.println("senha_admin=" + senha_admin_1);
  }

  server.send(200, "text/json", "{\"codigo\": 0}");
  file.close();
  Serial.println("Arquivo gravado com sucesso");
  delay(1000);

  ESP.restart(); // renicia
}