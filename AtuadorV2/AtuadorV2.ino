/*
*
*
*
*Ruan Bulsing ago 2022
*/
#include <PID_v1.h>
#include <ESP8266WiFi.h>//Biblioteca do WiFi.
#include <WiFiUdp.h>//Biblioteca do UDP.
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Timing.h> // contador amostragem /s

#define PIN_INPUT A0
#define PIN_OUTPUT D3

WiFiUDP udp;//Cria um objeto da classe UDP.
LiquidCrystal_I2C lcd(0x27, 16, 2);
Timing timerContador;

// Set web server port number to 80
WiFiServer server(5004);

// Variable to store the HTTP request
String header;
String req;//String que armazena os dados recebidos pela rede.
String sred   = ""; // separa o valor de atuacao no red
String sgreen = ""; // separa o valor de atuacao no green
String sblue  = ""; // separa o valor de atuacao no blue
String stemp  = ""; // separa o valor da temperatura
String serro  = ""; // separa o valor do erro de req
String sfailState = "";// separa o valor do fail de req
int vpot   = 0;
int red    = 0; // potencia para o led vermelho
int green  = 0; // potencia para o led verde
int blue   = 0; // potencia para o azul
int failState = 0; // contador de erros de comunicacao
int failStateA = 0; // comparador do fail recebido com o anterior
int fail = 0; // dois fails de comunicacao para teste 
int failA = 0; // ver uma forma de otimizar isso pq tem muita variavel para isso 
float temp = 0.0; // temperatura da conversao de srting stemp
float erro = 0.0; // erro da conversao de string serro

bool estado_botao = 0;
int taxaAmostras = 0; // tentar fazer uma taxa de amostras por segundo
byte a[8] = {B00110, B01001, B00110, B00000, B00000, B00000, B00000, B00000,}; // caracter °

double Setpoint = 0, Input, Output;// baseado no valor do erro, quero erro 0 entao o set point é 0
double Kp = 20, Ki = 4, Kd = 1; //valores experimentados considerados suficientes
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);


void setup() {
  Wire.begin(D2, D1); // inicio o lcd
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  AGUARDE CONEXAO");
  Serial.begin(9600); // inicia o serial para monitoramento apenas // com7
  myPID.SetMode(AUTOMATIC); // pid
  pinMode(LED_BUILTIN, OUTPUT); // pino d4
  pinMode(A0, INPUT); // entrada potenciometro
  pinMode(D5, OUTPUT);//b
  pinMode(D6, OUTPUT);//g
  pinMode(D7, OUTPUT);//r
  pinMode(D3, OUTPUT);// MOTOR
  pinMode(D0, INPUT); // BOTAO
  analogWrite(D3, 0); // desliga o motor

  analogWriteFreq(20000);// aumenta a frequencia do pwm para evitar ruido audivel

  timerContador.begin(0); // inicia o contador/ cuidar essa mudança da frequencia

  digitalWrite(LED_BUILTIN, LOW); // liga o led para indicar que comecou a conexao
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println("NaoFunciona");
  WiFi.begin("NaoFunciona", "naoseipq1");
  delay(1000);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH );
    delay(500);//-
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH); // desliga o led quando conecta
  server.begin();
  udp.begin(5004);//Inicializa a recepçao de dados UDP na porta 5004
  delay(1000); // tentativa de evitar de entrar no whatdog protection
  req.reserve(256); // reserva um espaco na memoria para manipular a string

}
void loop() { // com7

  lcd.setCursor(0, 0);  // Move the cursor at origin
  //       ("********************");   tamanho de 20 colunas e limpar os valores
  lcd.print("TEMPERATURA:        ");
  lcd.setCursor(13, 0);
  lcd.print(temp);
  lcd.createChar(1, a); //Atribui a "1" o valor do array "A", que desenha o simbolo de grau
  lcd.setCursor(18, 0); //Coloca o cursor na coluna 7, linha 1
  lcd.write(1);         //Escreve o simbolo de grau
  lcd.setCursor(19, 0); //Coloca o cursor na coluna 7, linha 1
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("MOTOR%:              ");
  lcd.setCursor(8, 1);
  lcd.print(Output * 100 / 255); // printa a potencia aplicada no motor em %
  lcd.setCursor(0, 2);
  lcd.print("TAXA 3/s:");  // amostras dentro de 3 segundos
  //lcd.setCursor(10, 2);
  //lcd.print(taxaAmostras);
  lcd.setCursor(13, 2);
  lcd.print("FS:    ");  // contador de falhas de comunicacao
  lcd.setCursor(17, 2);
  lcd.print(failState);
  lcd.setCursor(0, 3);
  lcd.print("MODO:              "); // controle do motor em manual ou automatico
  // lcd.setCursor(7, 3);
  // lcd.print(MANUAL);



  if (digitalRead(D0) == HIGH ) { // se o boao for apertado, inverte o estado do botao
    while (digitalRead(D0) == HIGH) {}  // evita um pouco de debouce
    delay(100);
    estado_botao = !estado_botao;
  }
  
  if (estado_botao == HIGH) // opcao manual
  {
    lcd.setCursor(6, 3);
    lcd.print("MANUAL");
    digitalWrite(D7, HIGH); // vermelho on
    digitalWrite(D6, LOW); // verde e azul of
    digitalWrite(D5, LOW);
    int vpot = analogRead(A0); // le potenciometro
    vpot = map(vpot, 0, 1023, 0, 255); // adapta pro motor
    analogWrite(D3, vpot); // escreve no motor
  }

  else {
    lcd.setCursor(6, 3);
    lcd.print("REMOTO");
    //analogWrite(D3, 0); // desliga o motor
    listen(); // recebe do pc
    if (timerContador.onTimeout(3000)) { //Imprime a contagem regressiva no monitor serial a cada 3 segundos
      if (taxaAmostras == 0) {                                  // se estivr no modo remoto
        failState  = failState + 1; // se nao receber nada em 3 segundos, conta o fail stade
        //Serial.print("aqui");  // debug raiz
      }
      lcd.setCursor(10, 2);
      lcd.print("   ");
      lcd.setCursor(10, 2);
      lcd.print(taxaAmostras); // imprimi a quantidade a cada ciclo
      taxaAmostras = 0; // zera no final da contagem
    }
  }
    if (failState >= 20) { // se o contador de fails for até 20
    estado_botao = HIGH;   //entra no modo manual
    failState = 0;         // zera o contador de fails
  }

}

void listen()//Sub-rotina que verifica se há pacotes UDP's para serem lidos.
{

  if (udp.parsePacket() > 0)//Se houver pacotes para serem lidos
  {
    req    = "";   // limpa a mensagem depois de trabalhar nela
    taxaAmostras ++; // acrescenta uma amostra no segundo
    digitalWrite(LED_BUILTIN, LOW); // pisca o led pra indicar que ta recebendo algo

    while (udp.available() > 0)//Enquanto houver dados para serem lidos
    {
      char z = udp.read();//Adiciona o byte lido em uma char
      req += z;//Adiciona o char à string enquanto tiver bytes
    }
    Serial.println("");
    Serial.print("listen: "); // debug raiz
    Serial.println(req);//Printa a string recebida no Serial monitor.

    //delay(5);//- // pisca o led pra indicar que ta recebendo algo
    digitalWrite(LED_BUILTIN, HIGH);

    //tratando o que recebeu ########################################################
    // message = [failReceive, 'r', red, 'g', green, 'b', blue,'t', data, 'e', erro] o que o servidor manda
    
    req.trim();                 // remove os espaços da string *nao funcionou
    //int length  = req.length(); // acha o tamanho da string
    int iopen   = req.indexOf("[");   // acha a posicao do [
    // o failstate
    int ir      = req.indexOf("'r'"); // acha o 'r'
    //red
    int ig      = req.indexOf("'g'"); // acha o 'g'
    //green
    int ib      = req.indexOf("'b'"); // acha o 'b'
    //blue
    int it      = req.indexOf("'t'"); // acha o 't'
    //temp
    int ie      = req.indexOf("'e'"); // acha o 'e'
    //erro
    int iclose  = req.indexOf("]");   // acha a posicao da ]

    sfailState = req.substring(iopen + 1 , ir - 2);  // separa o valor do failStade no inicio
    sred   =  req.substring(ir + 5 , ig - 2 );    // separa o valor do red, que é do [ até ao g
    sgreen =  req.substring(ig + 5 , ib - 2 );    // separa o valor do green, que é do g até b
    sblue  =  req.substring(ib + 5 , it - 2 );    // separa o valor do blue, que é do b até t
    stemp  =  req.substring(it + 5 , ie - 2 );    // separa o valor da temp, que é do t até e
    serro  =  req.substring(ie + 5, iclose);      // separa o valor do erro, que é do e até ]

/*
    Serial.println("");     //Printa a string tratada no Serial monitor. debug raiz
    Serial.print("sfailState: ");
    Serial.println(sfailState);
    Serial.print("sred: ");
    Serial.println(sred);
    Serial.print("sgreen: ");
    Serial.println(sgreen);
    Serial.print("sblue: ");
    Serial.println(sblue);
    Serial.print("stemp: ");
    Serial.println(stemp);
    Serial.print("serro: ");
    Serial.println(serro);
    */
  }

  // transforma de string para float ou int
  fail = sfailState.toInt(); // se o valor de fail que recebe do servidor
  if ( fail > failA) failState ++;  // maior que o fail anterior, soma o failstate
  red   = sred.toInt();
  green = sgreen.toInt();
  blue  = sblue.toInt();
  temp  = stemp.toFloat();
  erro  = serro.toFloat();
  Input = erro; // o valor que controla o motor é o erro recebido do servidor
  myPID.Compute(); // calcula a atuacao no motor
  
  /*
      Serial.println("");
      Serial.print("temp: ");
      Serial.println(temp);//Printa a string recebida no Serial monitor. debug raiz
      Serial.print("erro: ");
      Serial.println(erro);//Printa a string recebida no Serial monitor.
  */
  /*
    //funcao do map para entendimento. agora, resolvido no servidor.py
    long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
    int setpoint = 21; // diferente do setpoint do motor. esse é de temperatura
    int sobtemp = setpoint * 1.3; // margem alta de temperatura
    int subtemp = setpoint * 0.7; // margem baixa de temperatura
    trabalho apenas com o vermelho e mando para os outros conforme a temperatura
    red = map(temp, subtemp, sobtemp, 0, 255); // vermelho varia proporcional a temp
    blue  = 255 * 0.9 - red * 0.9; // blue recebe o contrario da potencia de red e 90% apenas
    if (red < 127) green = map(red * 0.9, 0, 127, 0, 255); // enquanto for menor que a metade o green aumenta
    if (red > 127) green = map(red * 0.9, 127, 255, 255, 0); // quando for maior que a metade green diminui
  */
  if (green <= 0) green = 0; // controle contra over
  if (green >= 255) green = 255;
  if (red <= 0) red = 0;
  if (red >= 255) red = 255;
  if (blue <= 0) blue = 0;
  if (blue >= 255) blue = 255;
  
  if (failState > failStateA) {
    
    failStateA = failState; // se contou o failstate, nao deixa atualizar
                            // os o motor e o led para evitar uma atuacao muito errada
  }
  else {
    analogWrite(D7, red);
    analogWrite(D6, green);
    analogWrite(D5, blue);
    analogWrite(PIN_OUTPUT, Output);
  }
}
