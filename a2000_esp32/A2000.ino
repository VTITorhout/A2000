#include "src/a2000/a2000.h"

A2000 meter;              //create class meter of type A2000
struct readings *p_data;  //create a pointer to the data of the meter

#define RxD 32  //wire between ESP and RO
#define TxD 25  //wire between ESP and DI
#define ENA 33  //wire between ESP and DE+RE

TaskHandle_t taskLoop2;


void setup(){
  //basic things
    Serial.begin(115200); //serial monitor
  //meter setup
    meter.setupUart(UART_NUM_1);  //use second UART (UART0 = serial monitor)
    meter.setupPins(TxD,RxD,ENA);
    p_data = meter.begin();       //start the measurement, get a pointer to the data
  //add worker to second core to prevent stalling of it
      xTaskCreatePinnedToCore(
        loop2,      //function name
        "loop2",    //description of the task
        10000,      //stack size
        NULL,       //task input parameters
        0,          //task priority
        &taskLoop2, //handle to the task
        0           //run on core 0, core 1 is for Arduino loop
      ); 
}

void loop(){
  //wait some time between prints
    delay(3000);  //wait 3 seconds, ugly implementation (but possible due to 2 core)
  //calculate time difference between measurement and now
    float deltaT = millis()-p_data->lastReading;
    deltaT /= 1000;
  //display the first phase, data of second and third phase can be reached by phase[1] and phase[2]
    Serial.printf("Data aged %03.2f seconds ago:\r\n\t U:%03.2fV, I:%03.2fA, P:%01.0fW, Q:%01.0fVAr, PF:%03.2f, freq:%03.2fHz\r\n",
      deltaT,
      p_data->phase[0].U,
      p_data->phase[0].I,
      p_data->phase[0].P,
      p_data->phase[0].Q,
      p_data->phase[0].PF,
      p_data->frequency);
}

void loop2(void * parameter){
  while(1){
    meter.worker();
    delay(1); //give the watchdog also some time...
  }
}
