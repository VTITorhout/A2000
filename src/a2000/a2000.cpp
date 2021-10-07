#include "a2000.h"

bool A2000::setupUart(int8_t port){ 			//setup the hardware for communications
	switch(port){
		case 0:		uart_enum = UART_NUM_0;
					break;
		case 1:		uart_enum = UART_NUM_1;
					break;
		case 2:		uart_enum = UART_NUM_2;
					break;			
		default:	return false;
	}
	uart_port = port;
	return true;
}

bool A2000::setupPins(int tx, int rx, int ena){	//define default pins
	if(uart_port!=-1){
		ESP_ERROR_CHECK(uart_set_pin(uart_enum, tx, rx, ena, UART_PIN_NO_CHANGE));
		return true;
	}else{
		return false;
	}
}

readings *A2000::begin(void){
	uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_EVEN,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	ESP_ERROR_CHECK(uart_param_config(uart_enum, &uart_config));
	ESP_ERROR_CHECK(uart_driver_install(uart_enum, 1024, 1024, 0, NULL, 0));
	ESP_ERROR_CHECK(uart_set_rts(uart_enum, 1)); //disconnect from bus
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
				uint8_t length;
				ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_enum, (size_t*)&length));//check number of bytes
				if(length>0){
					//there is data, retrieve the data
					total_length += uart_read_bytes(uart_enum, (uint8_t*)&rxBuff[total_length], length, 100);
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
							Serial.printf("\r\nU: %d, I: %d, P: %d",dimU,dimI,dimP);
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
				uint8_t length;
				ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_enum, (size_t*)&length));//check number of bytes
				if(length>0){
					//there is data, retrieve the data
					total_length += uart_read_bytes(uart_enum, (uint8_t*)&rxBuff[total_length], length, 100);
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
						float buff = (rxBuff[7]<<8)+rxBuff[6];
						buff *= powf(10,dimU);
						reading.phase[0].U = buff;
						buff = (rxBuff[9]<<8)+rxBuff[8];
						buff *= powf(10,dimU);
						reading.phase[1].U = buff;
						buff = (rxBuff[11]<<8)+rxBuff[10];
						buff *= powf(10,dimU);
						reading.phase[2].U = buff;
						buff = (rxBuff[13]<<8)+rxBuff[12];
						buff *= powf(10,dimI);
						reading.phase[0].I = buff;
						buff = (rxBuff[15]<<8)+rxBuff[14];
						buff *= powf(10,dimI);
						reading.phase[1].I = buff;
						buff = (rxBuff[17]<<8)+rxBuff[16];
						buff *= powf(10,dimI);
						reading.phase[2].I = buff;
						buff = (rxBuff[19]<<8)+rxBuff[18];
						buff *= powf(10,dimP);
						reading.phase[0].P = buff;
						buff = (rxBuff[21]<<8)+rxBuff[20];
						buff *= powf(10,dimP);
						reading.phase[1].P = buff;
						buff = (rxBuff[23]<<8)+rxBuff[22];
						buff *= powf(10,dimP);
						reading.phase[2].P = buff;
						buff = (rxBuff[25]<<8)+rxBuff[24];
						buff *= powf(10,dimP);
						reading.phase[0].Q = buff;
						buff = (rxBuff[27]<<8)+rxBuff[26];
						buff *= powf(10,dimP);
						reading.phase[1].Q = buff;
						buff = (rxBuff[29]<<8)+rxBuff[28];
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
						buff = (rxBuff[34]<<8)+rxBuff[33];
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

bool A2000::checkEndOfTx(void){
	bool ret = false;
	switch(uart_port){
		case 0:	ret = UART0.int_raw.tx_done;
				break;
		case 1:	ret = UART1.int_raw.tx_done;
				break;
		case 2:	ret = UART2.int_raw.tx_done;
				break;
	}
    if(ret){
		switch(uart_port){
			case 0:	UART0.int_clr.tx_done = 1;  // clear bit
					break;
			case 1:	UART1.int_clr.tx_done = 1;  // clear bit
					break;
			case 2:	UART2.int_clr.tx_done = 1;  // clear bit
					break;
		}
    }else{
      delayMicroseconds(50);
    }
    return ret;
}

bool A2000::writeData(const char* data, uint8_t length){
	txTimeout = millis();
	ESP_ERROR_CHECK(uart_set_rts(uart_enum, 0)); //connect to bus
	delayMicroseconds(txPreDelay);
	uart_write_bytes(uart_enum, &data[0], length);  //write the bytes
	while(!checkEndOfTx()){  //await tx_done
		if(millis()>txTimeout+100){
			ESP_ERROR_CHECK(uart_set_rts(uart_enum, 1)); //disconnect from bus
			return false;
		}
	}
	delayMicroseconds(txPostDelay);
	ESP_ERROR_CHECK(uart_set_rts(uart_enum, 1)); //disconnect from bus
	return true;
}
