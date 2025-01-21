

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>



float tdsValue1, tdsValue2;
float temperature1, temperature2;


//inizializzazione sensore temperatura 
#include <DS18B20.h>
DS18B20 ds1(3);
DS18B20 ds2(5);
uint8_t address[] ={40, 250, 31, 218, 4, 0, 0, 52};
uint8_t selected;

/* 

  Per reperire la libreria GravityTDS navigare a questo link:
  https://github.com/MRcodino07/HELIOS_SensoriArduinoUno/tree/main/Librerie

  Poi in Arduino IDE andare su Sketch/Include libraries/Add .zip e aggiungere il file scaricato NON ESTRATTO!

*/

//#include "GravityTDS1.h"
#include "GravityTDS.h"
#define TdsSensorPin1 A1
#define TdsSensorPin2 A2
GravityTDS gravityTds1, gravityTds2;

#define VALVOLA 8

/* CALIBRAZIONE CONDUCIMETRO!!!
  1. A sensore stabilizzato, scrivere nel Serial Monitor "enter"
  2. Scrivere successivamente "cal:" con in aggiunta la conducibilità della sostanza esaminata
    in uS/cm / 1.56 (per esempio se la conducibilità è 1500 uS/cm bisogna scrivere "cal: 961")
  3. Scrivere infine exit e premere invio per salvare la calibrazione
*/ 

// pH
// temperatura
// conducimetro


  float pHValue1, pHValue2;


//Alcune variabili e parametri necessari per il Sensore
#define SensorPin1 A5
#define SensorPin2 A6          //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate

#define ArrayLength  40    //times of collection
int pHArray1[ArrayLength], pHArray2[ArrayLength]; 
int tempArray1[ArrayLength], tempArray2[ArrayLength];  //Store the average value of the sensor feedback

//Parametri necessari per la linearizzazione
float calibph1_7=1.647, calibph2_7=1.647;
float calibph1_4=0.35, calibph2_4=0.35;
float m1, m2; 
float b1, b2;

StaticJsonDocument<200> jsonBuffer;

float calcRetta(float ph4, float ph7){
  return (4.0-7.0)/(ph4 - ph7);
}
float calcB(float m, float ph7){
  return 7.0-m*ph7;
}


// chiamata in loop quando c'è un comando dalla seriale (mandato dal raspberry qui per arduino)
void gestSerialCmd() {
    static String riga;

    riga = Serial.readString();  // legge il json come stringa
    deserializeJson(jsonBuffer, riga); // lo converte in jsonBuffer

    if (jsonBuffer.containsKey("cal1")) {
      gravityTds1.setTemperature(temperature1); 
      jsonBuffer["ritorno"] = gravityTds1.calibra(jsonBuffer["cal1"]);
      sendData(jsonBuffer);
    } 
    else if (jsonBuffer.containsKey("cal2")) { 
      gravityTds1.setTemperature(temperature2);
      jsonBuffer["ritorno"] = gravityTds2.calibra(jsonBuffer["cal2"]);
      sendData(jsonBuffer);
    }
    else if (jsonBuffer.containsKey("cal ph")) { // { "cal ph": 1, "ph4":0.5, "ph7":2.4 }
      if (jsonBuffer["cal ph"]== 1){
        calibph1_4=jsonBuffer["ph4"];
        calibph1_7=jsonBuffer["ph7"];
        m1=calcRetta(calibph1_4,calibph1_7);
        b1=calcB(m1,calibph1_7);
      }
      else { // ph 2
        calibph2_4=jsonBuffer["ph4"];
        calibph2_7=jsonBuffer["ph7"];
        m2=calcRetta(calibph2_4,calibph2_7);
        b2=calcB(m2,calibph2_7);
      }
    }
    // eventuali altri comandi... 
}

// Funzione per la calibrazione dei due ph
void calibratePh(){
  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output

  for (int i = 1; i<=2; i++){
    jsonBuffer.clear();
    // chiediamo parametri del  sensore PH i-esimo (1 o 2)
    jsonBuffer["cal ph"] = i;
    sendData(jsonBuffer);
    while (!Serial.available()){ // aspettiamo risposta da raspberry
      delay(500);
    }
    gestSerialCmd();

    // facciamo lampeggiare il led per capire che va tutto bene
    for (int k=0;k<4;k++){
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);  
        delay(100);                     
    }
  }
}


// Variabili e costanti per la gestione dell'elettrovalvola
bool aperto = false;
#define MINIMO 800;
#define MASSIMO 1400;
// Metodo che apre o chiude l'elettrovalvola in base
void gestioneValvola() {
  if (aperto || (((tdsValue1+tdsValue2)/2) *1.56) < MINIMO) {
    jsonBuffer["valvolaAperta"] = 1;
    digitalWrite (VALVOLA, HIGH);

    if ((((tdsValue1+tdsValue2)/2) *1.56) < MASSIMO) {
      aperto = true;
    } else {
      aperto = false;
    }

  } else {
    jsonBuffer["valvolaAperta"] = 0;
    digitalWrite (VALVOLA, LOW);
  }
}


void setup() {

  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  
  gravityTds1.setPin(TdsSensorPin1);
  gravityTds1.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO 
  gravityTds1.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC 
  gravityTds1.begin();
  gravityTds2.setPin(TdsSensorPin2);
  gravityTds2.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO 
  gravityTds2.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC 
  gravityTds2.begin();  //initialization 
 
  pinMode(8, OUTPUT); //Pin della valvola

	//Retta
	m1=calcRetta(calibph1_4,calibph1_7);
  b1=calcB(m1,calibph1_7);
	m2=calcRetta(calibph2_4,calibph2_7);
  b2=calcB(m2,calibph2_7);

  calibratePh();
}



// VERO -> pH; FALSO -> TDS

const int COND_INTERVAL = 5000;
const int PH_INTERVAL = 5000;
const int TEMP_INTERVAL = 5000;

void loop() {
  static unsigned long millisCOND = millis();
  static unsigned long millisPH = millis();
  static unsigned long millisTEMP = millis();
  static float voltage1, voltage2;

  // qualche comando da Raspberry?
  if (Serial.available()){ 
    gestSerialCmd();
  } 
  
  
  if(millis()-millisCOND > COND_INTERVAL){

    gravityTds1.setTemperature(temperature1);
    gravityTds1.update();
    tdsValue1 = gravityTds1.getTdsValue(); 

    jsonBuffer["conducimetro1"] = tdsValue1*1.56;

    gravityTds2.setTemperature(temperature2);
    gravityTds2.update();
    tdsValue2 = gravityTds2.getTdsValue(); 

    jsonBuffer["conducimetro2"] = tdsValue2*1.56;

    millisCOND=millis();

    gestioneValvola()

    sendData(jsonBuffer);
  }

  if(millis()-millisPH > PH_INTERVAL){

    for (int i = 0; i < ArrayLength; i++) {
      pHArray1[i] = analogRead(SensorPin1);
      delay(20);
    }
    voltage1 = averagearray(pHArray1, ArrayLength)*5.0/1024;
    pHValue1 = m1*voltage1+b1;
    jsonBuffer["pH1"] = pHValue1;
    jsonBuffer["voltage1"] = voltage1;

    for (int i = 0; i < ArrayLength; i++) {
      pHArray2[i] = analogRead(SensorPin2);
      delay(20);
    }
    voltage2 = averagearray(pHArray2, ArrayLength)*5.0/1024;
    pHValue2 = m2*voltage2+b2;
    jsonBuffer["pH2"] = pHValue2;
    jsonBuffer["voltage2"] = voltage2;

    millisPH=millis();
    
    sendData(jsonBuffer);
    
  }

  if(millis()-millisTEMP > TEMP_INTERVAL) {

    temperature1 = ds1.getTempC();
    jsonBuffer["temperatura1"] = temperature1;
    temperature2 = ds2.getTempC();
    jsonBuffer["temperatura2"] = temperature2;

    millisTEMP=millis();
    
    sendData(jsonBuffer);
    
  }
  
}

void sendData(StaticJsonDocument<200> &jBuff){
  if (jBuff.isNull())
    return;
  serializeJson(jBuff, Serial); // Serializza il buffer JSON e lo invia sulla seriale
  Serial.println(); // Aggiunge un newline alla fine del messaggio
  
  jBuff.clear(); // Pulisce il buffer JSON per il prossimo utilizzo
}

double averagearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  double amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
