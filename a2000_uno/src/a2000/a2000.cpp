#include "a2000.h"

void A2000::setupPins(int tx, int rx, int ena){	//define default pins
	enaPin = ena;	//save the pin for the enable
	pinMode(enaPin,OUTPUT);
	a2000Serial = new CustomSoftwareSerial(rx,tx);
	a2000Serial->begin(9600,CSERIAL_8E1);
}

readings *A2000::begin(void){
	digitalWrite(enaPin,LOW);	//disconnect from bus
	stateMachine = IDLE;
	struct readings *p_readings = &reading;
	reading.lastReading = 0;
	return p_readings;
}

void A2000::worker(void){							//background worker
	if(stateMachine!=NO_INIT){
		//UART setup completed, able to communicate?
		if(stateMachine==IDLE){
			//once every second?
			if((reading.lastReading+1000)<millis()){
				stateMachine = RET_DIM;
				wait = false;
				retry = 0;
				total_length = 0;
			}
		}
		if(stateMachine==RET_DIM){
			if(wait){
				//we are awaiting RX, check for data
				while(a2000Serial->available()){
					//there are still bytes
					rxBuff[total_length] = a2000Serial->read();
					total_length++;
				}
				if(millis()>(timeout+100)){
					//times up, how about the feedback + number of retries
					if(total_length>0){
						#if defined SER_DEBUG && SER_DEBUG == true
							//got a reply --> done receiving, show buffer
							for(uint8_t i=0;i<total_length;i++){
							  Serial.print("0x");
							  Serial.print(rxBuff[i],HEX);
							  Serial.print(" ");
							}
							Serial.println();
						#endif
						dimU = rxBuff[7];
						dimI = rxBuff[8];
						dimP = rxBuff[9];
						#if defined SER_DEBUG && SER_DEBUG == true
							//Serial.printf("\r\nU: %d, I: %d, P: %d",dimU,dimI,dimP);
							Serial.print("\r\nU: ");
							Serial.print(dimU);
							Serial.print(", I: ");
							Serial.print(dimI);
							Serial.print(", P: ");
							Serial.print(dimP);
						#endif
						stateMachine = RET_VAL;
						wait = false;
						retry = 0;
						total_length = 0;
					}else{
						if(retry<5){
							//try it again
							retry++;
							wait = false;
						}else{
							stateMachine = DIM_ERR;
						}
					}
				}
			}else{
				//send request
				if(!writeData(getDim,9)){
					stateMachine = COM_ERROR;
				}else{
					timeout = millis();
					wait = true;
				}
			}
		}
		if(stateMachine==RET_VAL){
			if(wait){
				//we are awaiting RX, check for data
				while(a2000Serial->available()){
					//there are still bytes
					rxBuff[total_length] = a2000Serial->read();
					total_length++;
				}
				if(millis()>(timeout+100)){
					//times up, how about the feedback + number of retries
					if(total_length>0){
						#if defined SER_DEBUG && SER_DEBUG == true
							//got a reply --> done receiving, show buffer
							for(uint8_t i=0;i<total_length;i++){
							  Serial.print("0x");
							  Serial.print(rxBuff[i],HEX);
							  Serial.print(" ");
							}
							Serial.println();
						#endif
						
						//save the values to the struct
						reading.lastReading = millis();
						//reading.phase[0].U = ((float)((rxBuff[7]<<8)+rxBuff[6]))*(pow(10,dimU));
						float buff = ((rxBuff[7]&0xFF)<<8)+(rxBuff[6]&0xFF);
						buff *= powf(10,dimU);
						reading.phase[0].U = buff;
						buff = ((rxBuff[9]&0xFF)<<8)+(rxBuff[8]&0xFF);
						buff *= powf(10,dimU);
						reading.phase[1].U = buff;
						buff = ((rxBuff[11]&0xFF)<<8)+(rxBuff[10]&0xFF);
						buff *= powf(10,dimU);
						reading.phase[2].U = buff;
						buff = ((rxBuff[13]&0xFF)<<8)+(rxBuff[12]&0xFF);
						buff *= powf(10,dimI);
						reading.phase[0].I = buff;
						buff = ((rxBuff[15]&0xFF)<<8)+(rxBuff[14]&0xFF);
						buff *= powf(10,dimI);
						reading.phase[1].I = buff;
						buff = ((rxBuff[17]&0xFF)<<8)+(rxBuff[16]&0xFF);
						buff *= powf(10,dimI);
						reading.phase[2].I = buff;
						buff = ((rxBuff[19]&0xFF)<<8)+(rxBuff[18]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[0].P = buff;
						buff = ((rxBuff[21]&0xFF)<<8)+(rxBuff[20]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[1].P = buff;
						buff = ((rxBuff[23]&0xFF)<<8)+(rxBuff[22]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[2].P = buff;
						buff = ((rxBuff[25]&0xFF)<<8)+(rxBuff[24]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[0].Q = buff;
						buff = ((rxBuff[27]&0xFF)<<8)+(rxBuff[26]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[1].Q = buff;
						buff = ((rxBuff[29]&0xFF)<<8)+(rxBuff[28]&0xFF);
						buff *= powf(10,dimP);
						reading.phase[2].Q = buff;
						buff = rxBuff[30];
						buff /= 100;
						reading.phase[0].PF = buff;
						buff = rxBuff[31];
						buff /= 100;
						reading.phase[1].PF = buff;
						buff = rxBuff[32];
						buff /= 100;
						reading.phase[2].PF = buff;
						buff = ((rxBuff[34]&0xFF)<<8)+(rxBuff[33]&0xFF);
						buff /= 100;
						reading.frequency = buff;
						
						stateMachine = IDLE;
					}else{
						if(retry<5){
							//try it again
							retry++;
							wait = false;
						}else{
							stateMachine = VAL_ERR;
						}
					}
				}
			}else{
				//send request
				if(!writeData(getValues,5)){
					stateMachine = COM_ERROR;
				}else{
					timeout = millis();
					wait = true;
				}
			}
		}
		if((stateMachine==DIM_ERR)||(stateMachine==VAL_ERR)){
			#if defined SER_DEBUG && SER_DEBUG == true
				Serial.println("There was an error retrieving values (>5 retries)");
			#endif
			stateMachine = IDLE;
		}
		if(stateMachine==COM_ERROR){
			#if defined SER_DEBUG && SER_DEBUG == true
				Serial.println("There was an communication error (could not transmit message)");
			#endif
			stateMachine = IDLE;
		}
	}
}				

bool A2000::writeData(const char* data, uint8_t length){
	txTimeout = millis();
	digitalWrite(enaPin,HIGH);	//disconnect from bus
	delayMicroseconds(txPreDelay);
	a2000Serial->write(&data[0],length);
	delayMicroseconds(txPostDelay);
	digitalWrite(enaPin,LOW);	//disconnect from bus
	return true;
}
