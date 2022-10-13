#include <Arduino.h>
#include <math.h>
#include "driver/uart.h"
//#include "components/driver/include/driver/uart.h"
//#include "components/driver/include/driver/uart.h"

#define txPreDelay	1000
#define txPostDelay	150

#define SER_DEBUG	true

const char getDim[] = {0x68, 0x03, 0x03, 0x68, 0xFA, 0x89, 0x32, 0xB5, 0x16};
	//0x68 = full record (no short message)
	//0x03 = lenght of message between address and closing byte
	//0x03 = lenght (repeat)
	//0x68 = full record  (repeat)
	//0xFA = address (250 --> can be adjusted on meter)
	//0x89 = function --> request data from A2000
	//0x32 = get dimensions
	//0xB5 = CRC, sum of 0xFA+0x89+0x32=0x1B5
	//0x16 = last byte, always 0x16

const char getValues[] = {0x10, 0xFA, 0x89, 0x83, 0x16};
	//0x10 = short message
	//0xFA = address (250 --> can be adjusted on meter)
	//0x89 = function --> request data from A2000
	//0x83 = CRC, sum of 0xFA+0x89=0x183
	//0x16 = last byte, always 0x16

//create enum for RxT
typedef enum{
	NO_INIT,		//after boot, not yet initialized
	IDLE,			//no communication going on
	RET_DIM,		//retrieve dimensions
	DIM_ERR,		//could not retrieve dimensions
	RET_VAL,		//retrieve values
	VAL_ERR,		//could not retrieve values
	COM_ERROR		//something went wrong on rx/tx level
} A2000_STATUS;

struct measurement{
	float U;
	float I;
	float P;
	float Q;
	float PF;
};

struct readings{
	uint32_t lastReading;
	float frequency;
	measurement phase[3];
};

class A2000{
	
	public:				
		bool setupPins(int tx, int rx, int ena);	//define default pins
			int tx=-1,rx=-1,ena=-1;
		bool setupUart(int8_t port); 			//setup the hardware for communications
			int8_t uart_port=-1;
			uart_port_t uart_enum;
		readings *begin(void);
			readings reading;
		void worker(void);							//background worker
			A2000_STATUS stateMachine=NO_INIT;
			char rxBuff[40];
			uint32_t timeout;
			bool wait = false;
			uint8_t retry = 0;
			uint8_t total_length = 0;
			int8_t dimU;
			int8_t dimI;
			int8_t dimP;
			
	private:
		bool writeData(const char* data, uint8_t length);
			uint32_t txTimeout;
		bool checkEndOfTx(void);
		
};