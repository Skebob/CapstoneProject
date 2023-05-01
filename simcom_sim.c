#include "simcom_sim.h"

bool modem_synced = false;

// Will initiate UART1 for AT communication 
bool SIM7000_Init(void){
	modem_synced = false;
	// give sim7000 3s to ensure power is stable
	// TODO set DTR, PWR_EN
	// modem was off, need to initiate UART1 as AT command channel
	UART1_init(9600);

}
// sends "AT" to SIM7000 untill "OK" response
bool SIM7000_SyncBaud(void){
	while(isModemSync() == false){
		UART1_CMD_POLL("AT;");
		delayMS(50);
	}
}

// send cmd of UART1, will wait for SIM7000 to echo back command before moving on
bool SIM7000_SendCommand(const char* cmd){
	//bool waitrequired = str_cmp(cmd, "AT+SMCONN;", 8);
	int timeout = 100;
	bool waitrequired = false; // does this command need a simple wait? (wait for command echo and OK from SIM)
	
	char expected_resp[8];
	int resp_len = 2;
	
	expected_resp[0] = 'O';
	expected_resp[1] = 'K';
	
	if(str_cmp(cmd, "AT+CNACT", 8)){
		timeout = 3000; //ms
		resp_len = 0;
	}
	else if(str_cmp(cmd, "AT+SMCONN", 9)){
		timeout = 0; //ms
	}
	else if(str_cmp(cmd, "AT+CIPSTATUS", 12)){
		timeout = 150; //ms
		resp_len = 0;
		
	}
	else if(str_cmp(cmd, "AT+SMPUB", 8)){
		timeout = 1500; // ms
		resp_len = 0;
		
	}
	else if(str_cmp(cmd, "AT+SMCONF", 9)){
		timeout = 0; // ms
		
	}
	else if(str_cmp(cmd, "AT+SMSUB", 8)){
		timeout = 0; // ms
	}
	else {
		timeout = 0; //ms
		resp_len = 2;
	}
	
	UART1_AT_CMD(cmd, expected_resp, resp_len, timeout);
}

// parse digits of num as a char array into str
void inttodigit5(int num, char* str){
	int temp = num; // 4096
	
	int digit_3 = temp / 1000; //4
	temp = temp - (digit_3 * 1000); //0096
	
	int digit_2 = temp / 100; // 0
	temp = temp - (digit_2 * 100); // 0096
	
	int digit_1 = temp / 10; // 9
	temp = temp - (digit_1 * 10); // 0006
	
	int digit_0 = temp % 10; 
	
	bool check = (num == (digit_0 + (digit_1*10) + (digit_2*100) + (digit_3*1000)));
	
	str[3] = digit_0+'0';
	str[2] = digit_1+'0';
	str[1] = digit_2+'0';
	str[0] = digit_3+'0';
}

// val is 0-4096
#define topic_sensor_motion "AT+SMPUB=\"sensor/motion\",\"4\",1,1;"
#define topic_sensor_moisture "AT+SMPUB=\"sensor/moisture\",\"4\",1,1;"
#define topic_sensor_light "AT+SMPUB=\"sensor/light\",\"4\",1,1;"
#define topic_test "AT+SMPUB=\"test\",\"4\",1,1;"
#define topic_mcustate "AT+SMPUB=\"mcu/state\",\"4\",1,1;"
char topic_connect_buffer[50];

void set_topic_connect(const char* topic_name){
	char AT_PUB_HEAD[] = "AT+SMPUB=\";";
	char AT_PUB_TAIL[] = "\",\"4\",1,1;";
	
	const char* src = AT_PUB_HEAD;
	
	int j = 0;
	for(int i = 0; src[i] != ';'; i++){
		topic_connect_buffer[j] = src[i];
		j++;
	}
	src = topic_name;
	for(int i = 0; src[i] != ';'; i++){
		topic_connect_buffer[j] = src[i];
		j++;
	}
	src = AT_PUB_TAIL;
	for(int i = 0; src[i] != ';'; i++){
		topic_connect_buffer[j] = src[i];
		j++;
	}
	topic_connect_buffer[j] = ';';
}

void SIM7000_MQTT_PUBLISH_STATE(char* state){
	char at_pub[] = topic_mcustate;
	
	UART1_AT_CMD(at_pub, "FUCK", 0, 2000);
	UART1_AT_CMD(state, "FUCK", 0, 3000);
}

void SIM7000_MQTT_PUBLISH_4INT(int val, int topicID, char* topic){
	
	char at_sensorA_pub[] = topic_sensor_motion;
	char at_sensorB_pub[] = topic_sensor_moisture;
	char at_sensorC_pub[] = topic_sensor_light;
	char at_test_pub[] = topic_test;
	
	int cmd_wait_time = 2000; //ms
	int timeout = 3000; //ms
	
	char num_str[5];
	num_str[4] = ';'; // val: 4096, num_str:{6, 9, 0, 4,:}
	
	inttodigit5(val, num_str);
	
	if(topicID == 0){
	// motion
		UART1_AT_CMD(at_sensorA_pub, "ERRR", 0, cmd_wait_time);
		UART1_AT_CMD(num_str, "ERRR", 0, timeout);
	}
	else if(topicID == 1){
	// moisture
		UART1_AT_CMD(at_sensorB_pub, "ERRR", 0, cmd_wait_time);
		UART1_AT_CMD(num_str, "ERRR", 0, timeout);
	}
	else if(topicID == 2){
	// light
		UART1_AT_CMD(at_sensorC_pub, "ERRR", 0, cmd_wait_time);
		UART1_AT_CMD(num_str, "ERRR", 0, timeout);
	}
	else{
		set_topic_connect(topic);
		UART1_AT_CMD(topic_connect_buffer, "ERRR", 0, cmd_wait_time);
		UART1_AT_CMD(num_str, "ERRR", 0, timeout);
	}
}

bool connected = false;
void SIM7000_MQTT_CONNECT(void){
	const char* script[] = {
	"ATI;",
	"AT+CSQ;",
	"AT+CNMP=38;",
	"AT+CMNB=1;",
	"AT+CIPSTATUS;",
	"AT+CIPSHUT;",
	"AT+CIPSTATUS;"
	"AT+CSTT=\"hologram\";", 
	"AT+CNACT=1,\"hologram\";",
	"AT+CNACT?;",
	"AT+SMCONF=\"USERNAME\",\"USERNAME\";", // placeholder username security
	"AT+SMCONF=\"PASSWORD\",\"PASSWORD\";", // placeholder password for security
	"AT+SMCONF=\"URL\",\"driver.cloudmqtt.com\",18893;",
	"AT+SMCONF=\"KEEPTIME\",60;",
	"AT+SMCONN;",
	"AT+SMSUB=\"website/state\",1;",
	"AT+SMSUB=\"website/cmd\",1;",
	"EOF"
	};
	
	int cmd_ptr = 0;
	while(!str_cmp(script[cmd_ptr], "EOF", 3)){
		SIM7000_SendCommand(script[cmd_ptr]);
		cmd_ptr++;
	}
	
	connected = true;
}

bool SIM7000_MQTT_CHECK_CONNECT(void){
	return connected;
}

// msg in web/cmd is at most 2 bytes
bool MQTT_GET_TOPIC_WEBCMD(char* src){
	bool ret = false;
	if(buffer_is_topic_webcmd()){
		for(int i = 0; i < 3; i++){
			src[i] = bufferGet(i);
			
		}
		ret = true;
	}
	return ret;
}

bool MQTT_CHECK_TOPIC_WEBCMD(const char* str, int len){
	int ret = false;
	if(buffer_is_topic_webcmd()){
		ret = msg_is_this(str, len);
	}
	return ret;
}

bool MQTT_CHECK_TOPIC_WEBSTATE(const char* str, int len){
	int ret = false;
	if(!buffer_is_topic_webcmd()){
		ret = msg_is_this(str, len);
	}
	return ret;
}
