
  float tdsValue;
  float temperature;


//inizializzazione sensore temperatura 
#include <DS18B20.h>
#define LOW_ALARM 15
#define HIGH_ALARM 35
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




  float pHValue;
  bool acidValue;
  bool basicValue;
  bool neutralValue;


//Alcune variabili e parametri necessari per il Sensore
#define SensorPin A5          //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;

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
  gravityTds.setAref(3.3);  //reference voltage on ADC, default 5.0V on Arduino UNO 
  gravityTds.setAdcRange(4096);  //1024 for 10bit ADC;4096 for 12bit ADC 
  gravityTds.begin();  //initialization 
 

	//Retta
	m=(4.0-7.0)/(calibph4 - calibph7);
    b=7.0-m*calibph7;
  
}

void loop() {
static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float voltage;
  
  if(millis()-samplingTime > samplingInterval){
    pHArray[pHArrayIndex++]=analogRead(SensorPin);
    if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
    //Serial.print(avergearray(pHArray, ArrayLenth));
    //Calcolo del valore del pH
    voltage = avergearray(pHArray, ArrayLenth)*3.3/1024;
    pHValue = m*voltage+b;
    samplingTime=millis();
  }

  Serial.println(analogRead(A5));
  
  //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  if(millis() - printTime > printInterval){   
    Serial.print("Voltage:");
      Serial.print(voltage,2);
    Serial.print("    pH value: ");
      Serial.println(pHValue,2);
    printTime=millis();
  }
  //Gestione del display e delle variabili collegate alla dashboard a seconda della sostanza
  //Verifica soluzione neutra
  if(pHValue >= 6.7 && pHValue <= 7.3){
  
  acidValue=false;
  basicValue=false;
  neutralValue=true;
  

  } else if(pHValue < 6.7 && pHValue > 3.0){
  
  acidValue=true;
  basicValue=false;
  neutralValue=false;

  } else if(pHValue > 7.3){
  
  acidValue=false;
  basicValue=true;
  neutralValue=false;
  

    //Qualora non ci fosse la sostanza deve essere segnalato sul display
  } else {
    
  acidValue=false;
  basicValue=false;
  neutralValue=false;
   
  }
  delay(500);
  // Your code here 
  temperature = ds.getTempC();
  gravityTds.setTemperature(temperature);
  gravityTds.update();
  tdsValue = gravityTds.getTdsValue();
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
  delay(1000);
}

double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
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

/*
  Since BasicValue is READ_WRITE variable, onBasicValueChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onBasicValueChange()  {
  // Add your code here to act upon BasicValue change
}

/*
  Since AcidValue is READ_WRITE variable, onAcidValueChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onAcidValueChange()  {
  // Add your code here to act upon AcidValue change
}

/*
  Since NeutralValue is READ_WRITE variable, onNeutralValueChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onNeutralValueChange()  {
  // Add your code here to act upon NeutralValue change
}




/*
  Since PHValue is READ_WRITE variable, onPHValueChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onPHValueChange()  {
  // Add your code here to act upon PHValue change
}



