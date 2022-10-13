#define RxD 8  //wire between ESP and RO
#define TxD 9  //wire between ESP and DI
#define ENA 10  //wire between ESP and DE+RE

#define SER_DBG_MAIN  false    //set to false if you want to connect to EXCEL
//set to true if you want to show the values on the serial monitor

#include "src/a2000/a2000.h"
int i = 0; // zelf bijgezet
A2000 meter;              //create class meter of type A2000
struct readings *p_data;  //create a pointer to the data of the meter

uint32_t lastPrintMillis;

void setup() {
#if defined SER_DBG_MAIN && SER_DBG_MAIN == true
  Serial.begin(115200); //set to speed you setup in serial monitor
  Serial.println("A2000 demo on Arduino UNO");
#else
  Serial.begin(9600);   //set to speed you setup in PLC_DAQ
  Serial.println("CLEARSHEET"); //zelf bijgezet
  Serial.println("LABEL,U,I,P,Q,PF,freq"); //zelf bijgezet
#endif
  meter.setupPins(TxD, RxD, ENA);
  p_data = meter.begin();       //start the measurement, get a pointer to the data
}

void loop() {
  //don't delay the loop, otherwise incoming data will not be processed, you can (and may) print the values as follows:
#if defined SER_DBG_MAIN && SER_DBG_MAIN == true
  if ((lastPrintMillis + 3000) < millis()) {
    //print the values when 3 seconds have passed
    //calculate time difference between measurement and now
    float deltaT = millis() - p_data->lastReading;
    deltaT /= 1000;
    Serial.print("Data aged ");
    Serial.print(deltaT, 2);
    Serial.print(" seconds ago:\r\n\t U:");
    Serial.print(p_data->phase[0].U, 2);
    Serial.print("V, I:");
    Serial.print(p_data->phase[0].I, 2);
    Serial.print("A, P:");
    Serial.print(p_data->phase[0].P, 2);
    Serial.print("W, Q:");
    Serial.print(p_data->phase[0].Q, 2);
    Serial.print("VAr, PF:");
    Serial.print(p_data->phase[0].PF, 2);
    Serial.print(", freq:");
    Serial.println(p_data->frequency, 2);
    //update last print time
    lastPrintMillis = millis();
  }
#else
  if ((lastPrintMillis + 1000) < millis()) {
    Serial.print("DATA,");
    Serial.print(p_data->phase[0].U, 2);
    Serial.print(",");
    Serial.print(p_data->phase[0].I, 2);
    Serial.print(",");
    Serial.print(p_data->phase[0].P, 2);
    Serial.print(",");
    Serial.print(p_data->phase[0].Q, 2);
    Serial.print(",");
    Serial.print(p_data->phase[0].PF, 2);
    Serial.print(",");
    Serial.println(p_data->frequency, 2);
    //update last print time
    lastPrintMillis = millis();

  }
#endif

  meter.worker();
}
