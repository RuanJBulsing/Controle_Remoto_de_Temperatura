/*
  PROJETO REDES INDUSTRIAIS
  RUAN BULSING
  2022
  sensor de temperatura com o ds18b20
  manda o valor para a serial apenas.
  para o trabalho, é necessário enviar
  numero com 6 casas decimais. entao,
  o valor em float da temperatura
  sera multiplicado por 10^6 para enviar
  e dividido por 10^6 no servidor.
  que fará o mesmo com o MV para o atuador.
*/

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ESP8266WiFi.h>//Biblioteca do WiFi.
#include <WiFiUdp.h>//Biblioteca do UDP.

#define dados 2 /*o pino de dados do sensor está ligado na porta 2 do Arduino*/
OneWire oneWire(dados);  /*Protocolo OneWire*/
DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/
DeviceAddress tempDeviceAddress;

WiFiUDP udp;//Cria um objeto da classe UDP.

float temp;//Variavel para ser enviada.

WiFiServer server(5005);// porta usada que nao interfere no udp
String req = "";//String que armazena os dados recebidos pela rede.
int env = 0;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  req.reserve(64); // reserva um espaco na memoria para manipular a string
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);//Define o ESP8266 como Station.

  Serial.print("Connecting to ");
  Serial.println("NaoFunciona");
  WiFi.begin("NaoFunciona", "naoseipq1"); // a rede
  delay(1000);

  while (WiFi.status() != WL_CONNECTED) { // inica na serial que esta
    digitalWrite(LED_BUILTIN, LOW);       // tentando conectar
    delay(500);//-
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected."); // diz se conectou
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  udp.begin(5005);//Inicializa a recepçao de dados UDP na porta 5005
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, 12); // seta a resolução maxima do sensor de temp
}

void loop()//com7 a de cima com 8 a de baixo
{

  //connect();//Sub-rotina para conectar-se ao host.

  send();//Sub-rotina para enviar os dados ao host. É só isso aqui.

  //listen();
}
/*
void connect()//Sub-rotina para verificar a conexao com o host.
{
  if (WiFi.status() != WL_CONNECTED)//Caso nao esteja conectado ao host, ira se conectar.
  {
    Serial.print("Connecting to ");
    Serial.println("NaoFunciona");
    WiFi.begin("NaoFunciona", "naoseipq1");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);//-
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.print(".");
  }

}
*/

void send()//Sub-rotina para enviar dados ao host.
{
  if (WiFi.status() == WL_CONNECTED)//Só ira enviar dados se estiver conectado.
  {
    Temperature();// capta a temperatura
    env = temp * 1000000; // envia um valor inteiro
    udp.beginPacket("10.0.0.255", 5005);//Inicializa o pacote de transmissao por broadcast e PORTA.
                                        // precisa saber o ip, mas a ultima sentenca e 255 para o broadcast
    udp.println(env);//Adiciona-se o valor ao pacote.
    udp.endPacket();//Finaliza o pacote e envia.
    delay(10);//-
    Serial.print("env: ");// debug raiz
    Serial.println(env);//Printa a string recebida no Serial monitor.
    digitalWrite(LED_BUILTIN, HIGH);// indica que esta enviando
    delay(10);//-
    digitalWrite(LED_BUILTIN, LOW);
    env = 0; // limpa o valor depois de enviar
  }

}
/*
void listen()//Sub-rotina que verifica se há pacotes UDP's para serem lidos.
{
  if (udp.parsePacket() > 0)//Se houver pacotes para serem lidos
  {
    while (udp.available() > 0)//Enquanto houver dados para serem lidos
    {
      char z = udp.read();//Adiciona o byte lido em uma char
      req += z;//Adiciona o char à string
    }
    Serial.println("");
    Serial.print("listen: ");
    Serial.println(req);//Printa a string recebida no Serial monitor.

    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);//-
    digitalWrite(LED_BUILTIN, LOW);
    req = ""; // limpa a mensagem depois de trabalhar nela
  }
  //Enviar Resposta
}
*/

void Temperature() { // capta e salva a temperatura do sensor

  sensors.requestTemperatures(); /* Envia o comando para leitura da temperatura */
  temp = sensors.getTempCByIndex(0);
  // Serial.println("");
  // Serial.print("temp: ");
  // Serial.println(temp);//debug raiz
  delay(5);//evita erro

}
