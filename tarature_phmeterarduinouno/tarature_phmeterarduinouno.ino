#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

float tdsValue;
float temperature;


//inizializzazione sensore temperatura 
#include <DS18B20.h>
DS18B20 ds(3);
uint8_t address[] = {40, 250, 31, 218, 4, 0, 0, 52};
uint8_t selected;


/* 

  Per reperire la libreria GravityTDS navigare a questo link:
  https://github.com/CoccodrillooXDS/GravityTDS/releases/tag/v1.0.1

  Poi in Arduino IDE andare su Sketch/Include libraries/Add .zip e aggiungere il file scaricato NON ESTRATTO!

*/

#include "GravityTDS.h"
#define TdsSensorPin A1
GravityTDS gravityTds;

/* CALIBRAZIONE CONDUCIMETRO!!!
  1. A sensore stabilizzato, scrivere nel Serial Monitor "enter"
  2. Scrivere successivamente "cal:" con in aggiunta la conducibilità della sostanza esaminata
    in uS/cm / 1.56 (per esempio se la conducibilità è 1500 uS/cm bisogna scrivere "cal: 961")
  3. Scrivere infine exit e premere invio per salvare la calibrazione
*/ 

// pH
// temperatura
// conducimetro


  float pHValue;


//Alcune variabili e parametri necessari per il Sensore
#define SensorPin A5          //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate

#define ArrayLength  40    //times of collection
int pHArray[ArrayLength]; 
int tempArray[ArrayLength];  //Store the average value of the sensor feedback

//Parametri necessari per la linearizzazione
float calibph7=1.18;
float calibph4=0.20;
float m; 
float b;

void setup() {

  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO 
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC 
  gravityTds.begin();  //initialization 
 

	//Retta
	m=(4.0-7.0)/(calibph4 - calibph7);
    b=7.0-m*calibph7;
  
}

// VERO -> pH; FALSO -> TDS

StaticJsonDocument<200> jsonBuffer;
bool turno = false;
const int READ_INTERVAL = 1000;
void loop() {
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float voltage;
  
  if(millis()-samplingTime > READ_INTERVAL){

    temperature = ds.getTempC();
 
    for (int i = 0; i < ArrayLength; i++) {
      pHArray[i] = analogRead(SensorPin);
      delay(20);
    }
    voltage = averagearray(pHArray, ArrayLength)*5.0/1024;
    pHValue = m*voltage+b;

    samplingTime=millis();

    gravityTds.setTemperature(temperature);
    gravityTds.update();
    tdsValue = gravityTds.getTdsValue(); 
     

    jsonBuffer["temperatura"] = temperature;
    jsonBuffer["pH"] = pHValue;
    jsonBuffer["conducimetro"] = tdsValue*1.56;
    sendData(jsonBuffer);
  }
  //Gestione del display e delle variabili collegate alla dashboard a seconda della sostanza
  //Verifica soluzione neutra
  // Your code here 
  
  /*
  Serial.print(tdsValue,0);
  Serial.print(" ppm - ");
  Serial.print(tdsValue*1.56,0);
  Serial.print(" uS/cm - ");
  Serial.print(temperature);
  Serial.print("°C - ");
  if (ds.hasAlarm()){
    Serial.println("ALARM");
  } else {
    Serial.println();
  }
  */
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
