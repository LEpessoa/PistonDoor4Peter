// #include <EEPROM.h>
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Definição dos pinos
const int LED_1_porta_Fechada = 13;
const int IN1_relay1_abertura = 8;
const int IN2_relay2_fechamento = 10;
const int IN3_relay3_trancamento = 4;        
// const int LED_A1_azul_externo = 5;
const int LED_VD1_verde_externo = 6;
const int LED_VM1_vermelho_externo = 7;
const int B_EXT_input_B_externo = 2;
const int SFC_fimDeCurso = 9;
const int B_INT_input_B_interno = 3;
const int LED_A2_azul_interno = 11;
const int LED_VD2_verde_interno = 12;
// const int LED_VM2_vermelho_interno = 13;
const int apertouSeFudeu = 5;

// Variáveis de tempo
unsigned long tempo_para_pistao_alcancar_sfc = 3000;                   // Tempo em milissegundos para acionamento do IN1
unsigned long tempo_para_pistao_recolher = 3000;                       // Tempo em milissegundos para acionamento do IN2
unsigned long tempo_para_pressurizar_trancamento = 500;                // Tempo em milissegundos para acionamento do IN3
unsigned long delay_que_sfc_pressionado_para_ativar_trancamento = 500; // Tempo em milissegundos de espera para acionamento do IN3
unsigned long delay_B_externo_ativacao_real = 500;                     // Tempo de pressionamento dos botões para acionamento dos relés
unsigned long delay_B_interno_ativacao_real = 500;
unsigned long delay_B_apertou_se_fudeu = 5000;

void setup()
{
    // Configuração dos pinos de saída
    pinMode(IN1_relay1_abertura, OUTPUT);
    pinMode(IN2_relay2_fechamento, OUTPUT);
    pinMode(IN3_relay3_trancamento, OUTPUT);
    // pinMode(LED_A1_azul_externo, OUTPUT);
    pinMode(LED_VD1_verde_externo, OUTPUT);
    pinMode(LED_VM1_vermelho_externo, OUTPUT);
    pinMode(LED_A2_azul_interno, OUTPUT);
    pinMode(LED_VD2_verde_interno, OUTPUT);
    // pinMode(LED_VM2_vermelho_interno, OUTPUT);
    pinMode(apertouSeFudeu, OUTPUT);
    pinMode(LED_1_porta_Fechada, OUTPUT);

    // Configuração dos pinos de entrada com resistores pull-up internos
    pinMode(B_EXT_input_B_externo, INPUT_PULLUP);
    pinMode(B_INT_input_B_interno, INPUT_PULLUP);
    pinMode(SFC_fimDeCurso, INPUT_PULLUP);
    pinMode(apertouSeFudeu, INPUT_PULLUP);

    // Inicializar pinos de saída
    digitalWrite(IN1_relay1_abertura, HIGH);
    digitalWrite(IN2_relay2_fechamento, HIGH); // intermitente!
    digitalWrite(IN3_relay3_trancamento, HIGH);
    digitalWrite(LED_1_porta_Fechada, LOW);
	
    setupCores();
  	setupAbertaOuFechada();
    Serial.begin(9600);
}

//  ######################### INFORMACAO CONTROLE DE ESTADO #########################
// SFC Solto => Botoes de dentro e de fora verdes
// 0 - Botao Externo | 1 - Botao Interno
// De dentro - verde ou azul(trancado)
// De fora -  verde ou vermelho(ocupado - trancado por dentro)
// fechada ou aberto (Posso ou nao passar pela porta)
// trancada ou destrancada
//
// Se (SFC Pressionado) => Botao de fora vermelho | Botao de dentro azul
// Se (SFC Não pressionado) => Ambos verdes

bool portaTrancada = LOW;
bool portaFechada = LOW;
bool newClickExterno = true;
bool newClickInterno = true;

String corInterna = "verde";
String  corExterna = "verde";

unsigned long timeToWaitBeforeSleeping = 5000;
unsigned long timeSinceLastButtonPress= 0;
unsigned long wakeUpTime = 0;

void loop()
{   
    if ((millis() - timeSinceLastButtonPress) > timeToWaitBeforeSleeping) {
      //goToSleep();
    }  

    if ((millis() - wakeUpTime) < 3000) {return;}

    if (!digitalRead(apertouSeFudeu) == HIGH) {
        delay(500);
        if (!digitalRead(apertouSeFudeu) == HIGH) {
            int tempoTotal = 0;
            int miniDelay = 300;
            while (tempoTotal < delay_B_apertou_se_fudeu) {
                delay(miniDelay);
                if(!digitalRead(apertouSeFudeu) == LOW) {return;}
                tempoTotal += miniDelay;
            }
            abrirPorta();
        }        
    }

  	digitalWrite(LED_1_porta_Fechada, portaFechada);
    // Leitura dos estados dos botões e do sensor de fim de curso
    bool estadoB_EXT = !digitalRead(B_EXT_input_B_externo);
    bool estadoB_INT = !digitalRead(B_INT_input_B_interno);
    delay(100);
    estadoB_EXT = !digitalRead(B_EXT_input_B_externo);
    estadoB_INT = !digitalRead(B_INT_input_B_interno);

    if (!estadoB_EXT){newClickExterno=true;}
    if (!estadoB_INT){newClickInterno=true;}

    if (estadoB_INT) {
        logica_B_INT();
        return;
    }

    if (estadoB_EXT) {
        logica_B_EXT();
    }

    printState();
}

unsigned long timeSinceLastPrint = 0;
void printState() {
  if ((millis() - timeSinceLastPrint) < 2000){return;}
  timeSinceLastPrint = millis();
  Serial.print("Fechada: ");
  Serial.print(portaFechada);
  Serial.print(" | Trancada: ");
  Serial.print(portaTrancada);
  Serial.print(" | CorInterna: ");
  Serial.print(corInterna);
  Serial.println(" | CorExterna: ");
  Serial.println(corExterna);
}

void logica_B_INT() {
    timeSinceLastButtonPress = millis();
    if (!newClickInterno) {return;}
    newClickInterno = false;
    bool estadoB_INT = !digitalRead(B_INT_input_B_interno);
    if (portaFechada)
    {   
        if (estadoB_INT ){
            delay(delay_B_interno_ativacao_real);
            if(!estadoB_INT){return;}
            abrirPorta();
            return;
        }      
    }
    if (!portaFechada) {
        if (estadoB_INT){
            delay(delay_B_interno_ativacao_real);
            if(!estadoB_INT){return;}
            trancarPorta();
            return;
        }
    }
}

void logica_B_EXT() {
    timeSinceLastButtonPress = millis();
    if (!newClickExterno){return;}
    bool estadoB_EXT = !digitalRead(B_EXT_input_B_externo);
    if (portaFechada) {
        if (estadoB_EXT)
        {
            delay(delay_B_externo_ativacao_real);
            if (!estadoB_EXT){return;}
            if (portaTrancada){return;}
            abrirPorta();
            return;
        }
    }

    if (!portaFechada) {
        if(estadoB_EXT) {
            delay(delay_B_externo_ativacao_real);
            if (!estadoB_EXT){return;}
            fecharPorta();
            return;
        }
    }
}

//setupInicialDaPorta
void setupAbertaOuFechada() {
  if(!digitalRead(SFC_fimDeCurso)) {portaFechada=HIGH;return;}  
}

// Funcoes controle da porta
void abrirPorta() {
    portaFechada = LOW;
    portaTrancada = LOW;
    setupCores();
    recolherPistaoAbrirPorta();
}

void fecharPorta() {
    empurrarPistaoFecharPorta();
    setupCores();
}

void trancarPorta() {
    empurrarPistaoFecharPorta();
    pressurizarParaTrancamento();
    setupCores();
}

// ########## Funcoes de controle dos relays/valvulas ##########
void pressurizarParaTrancamento()
{   
    if (digitalRead(SFC_fimDeCurso)) {
        return;
    } else {
        delay(delay_que_sfc_pressionado_para_ativar_trancamento);
        if (digitalRead(SFC_fimDeCurso)) {return;}
    }
    digitalWrite(IN3_relay3_trancamento, LOW);
    delay(tempo_para_pressurizar_trancamento);
    digitalWrite(IN3_relay3_trancamento, HIGH);
    portaTrancada = HIGH;
}

void despressurizarParaDestrancar()
{
    digitalWrite(IN3_relay3_trancamento, HIGH);
    portaTrancada = LOW;
}

void empurrarPistaoFecharPorta()
{
    despressurizarParaDestrancar();
    digitalWrite(IN1_relay1_abertura, HIGH);
    digitalWrite(IN2_relay2_fechamento, LOW);
    delay(tempo_para_pistao_alcancar_sfc);
  	if(digitalRead(SFC_fimDeCurso)){
      abrirPorta();
      return;
    }else{
      delay(delay_que_sfc_pressionado_para_ativar_trancamento);
      if(digitalRead(SFC_fimDeCurso)){
        abrirPorta();
        return;
      }
    }  
    digitalWrite(IN2_relay2_fechamento, HIGH);
  	portaFechada = HIGH;
}

void recolherPistaoAbrirPorta()
{
    despressurizarParaDestrancar();
    digitalWrite(IN2_relay2_fechamento, HIGH);
    digitalWrite(IN1_relay1_abertura, LOW);
    delay(tempo_para_pistao_recolher);
    digitalWrite(IN1_relay1_abertura, HIGH);
  	portaFechada = LOW;
}

// ########## Funcoes de definicao das cores dos botoes ##########
void setupCores()
{
    if (portaTrancada) {
        setupTrancada();
    } else {
        setupAberta();
    }
}

void setupTrancada()
{
    botaoDentroAzul();
    botaoForaVermelho();
}

void setupAberta()
{
    botaoDentroVerde();
    botaoForaVerde();
}


void botaoDentroAzul()
{
  corInterna = "azul";
    digitalWrite(LED_A2_azul_interno, HIGH);
    digitalWrite(LED_VD2_verde_interno, LOW);
    // digitalWrite(LED_VM2_vermelho_interno, LOW);
}

void botaoDentroVerde()
{   
    corInterna = "verde";
    digitalWrite(LED_A2_azul_interno, LOW);
    digitalWrite(LED_VD2_verde_interno, HIGH);
    // digitalWrite(LED_VM2_vermelho_interno, LOW);
}

void botaoForaVermelho()
{
    corExterna = "vermelho";
    // digitalWrite(LED_A1_azul_externo, LOW);
    digitalWrite(LED_VD1_verde_externo, LOW);
    digitalWrite(LED_VM1_vermelho_externo, HIGH);
}

void botaoForaVerde()
{
    corExterna = "verde";
    // digitalWrite(LED_A1_azul_externo, LOW);
    digitalWrite(LED_VD1_verde_externo, HIGH);
    digitalWrite(LED_VM1_vermelho_externo, LOW);
}

// ######################## SLEEP MODE ########################
void goToSleep() {
  Serial.println("Going to sleep...");
  delay(1000);
  attachInterrupt(digitalPinToInterrupt(B_EXT_input_B_externo), wakeUp, FALLING);
  attachInterrupt(digitalPinToInterrupt(B_INT_input_B_interno), wakeUp, FALLING);
  // Disable ADC (saves ~150µA)
  ADCSRA = 0;
  
  // Power down all non-essential peripherals (timers, SPI, I2C, etc.)
  power_all_disable();

  // Configure sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  // Disable Brown-Out Detection (saves power)
  sleep_bod_disable();

  // Enable interrupts and sleep
  sei();
  sleep_cpu();
  
  // After waking, re-enable peripherals if needed
  sleep_disable();
  power_all_enable(); // Re-enable peripherals (e.g., for Serial)
  
  wakeUpTime = millis();
  timeSinceLastButtonPress = millis();
  Serial.println("Waking up...");
  detachInterrupt(digitalPinToInterrupt(B_EXT_input_B_externo));
  detachInterrupt(digitalPinToInterrupt(B_INT_input_B_interno));
}

void wakeUp() {
  
}

