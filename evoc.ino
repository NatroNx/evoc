/******************************************************************************
A very first try to get my EV (hyundai Ioniq) connected to thinger.io

Software can be used as is and is licensed under GPLv3
******************************************************************************/

#include <FreematicsPlus.h>

#define PIN_LED 4
#define DEBUG Serial
#define CONNECT_OBD 1

static SPISettings settings = SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0);

COBDSPI obd;
bool connected = false;
unsigned long count = 0;
bool needNewData=true;
char a2105[8][7][3];
char a2101[8][9][3];
char a2102[8][6][3];
float evData[30][2];
uint32_t minute5;
int errors=0;
/**
evData:
0: State of Charge of Battery(BMS)
Available Charge Power
Available Discharge Power
Battery Current
Battery DC Voltage
Battery Max Temperature
Battery Min Temperature
Battery Module 01 Temperature
Battery Module 02 Temperature
Battery Module 03 Temperature
Battery Module 04 Temperature
Battery Module 05 Temperature
Battery Inlet Temperature
Max Cell Voltage
Max Cell Voltage No.
Min Cell Voltage
Min Cell Voltage No.
Auxiliary Battery Voltage
Cumulative Charge Current
Cumulative Discharge Current
Cumulative Charge Energy
Cumulative Discharge Energy
Cumulative Operating Time
Inverter Capacitor Voltage
Isolation Resistance

**/




const uint8_t header[4] = {0x24, 0x4f, 0x42, 0x44};  //means obd



void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  delay(1000);
  digitalWrite(PIN_LED, LOW);
  Serial.begin(115200);


  byte ver = obd.begin();



  Serial.print("OBD Firmware Version ");
  Serial.println(ver);
}


bool storeEvData()
{
	char helper[5];
  char helperLong[9];



 if(strcmp(a2101[0][1],"21")==0)
		 {evData[0][0]=hex2uint16(a2101[1][1])*0.5;
			 strcpy(helper,a2101[2][1]);
			 strcat(helper,a2101[3][1]);
			evData[1][0]=hex2uint16(helper)*0.01;
			strcpy(helper,a2101[4][1]);
			strcat(helper,a2101[5][1]);
		 evData[2][0]=hex2uint16(helper)*0.01;
		 strcpy(helper,a2101[7][1]);
   }
    if(strcmp(a2101[0][2],"22")==0)
    {
		 strcat(helper,a2101[1][2]);
		 evData[3][0]=(long(hex2uint16(helper)+pow(2,15))%long(pow(2,16))-pow(2,15))*0.1;
     strcpy(helper,a2101[2][2]);
     strcat(helper,a2101[3][2]);
		evData[4][0]=hex2uint16(helper)*0.1;
    evData[5][0]=long(hex2uint16(a2101[4][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[6][0]=long(hex2uint16(a2101[5][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[7][0]=long(hex2uint16(a2101[6][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[8][0]=long(hex2uint16(a2101[7][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
  }
  if(strcmp(a2101[0][3],"23")==0)
  {
    evData[9][0]=long(hex2uint16(a2101[1][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[10][0]=long(hex2uint16(a2101[2][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[11][0]=long(hex2uint16(a2101[3][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[12][0]=long(hex2uint16(a2101[5][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[13][0]=hex2uint16(a2101[6][3])*0.02;
    evData[14][0]=hex2uint16(a2101[7][3]);
  }
  if(strcmp(a2101[0][4],"24")==0)
  {
    evData[15][0]=hex2uint16(a2101[1][4])*0.02;
    evData[16][0]=hex2uint16(a2101[2][4]);
    evData[17][0]=hex2uint16(a2101[5][4])*0.1;
    strcpy(helperLong,a2101[6][4]);
    strcat(helperLong,a2101[7][4]);
    strcat(helperLong,a2101[1][5]);
    strcat(helperLong,a2101[2][5]);
  }
  if(strcmp(a2101[0][5],"25")==0)
  {
    evData[18][0]=strtol(helperLong, NULL, 16)*0.1;
    strcpy(helperLong,a2101[3][5]);
    strcat(helperLong,a2101[4][5]);
    strcat(helperLong,a2101[5][5]);
    strcat(helperLong,a2101[6][5]);
    evData[19][0]=strtol(helperLong, NULL, 16)*0.1;
  }
  if(strcmp(a2101[0][6],"26")==0)
  {   strcpy(helperLong,a2101[7][5]);
      strcat(helperLong,a2101[1][6]);
      strcat(helperLong,a2101[2][6]);
      strcat(helperLong,a2101[3][6]);
      evData[20][0]=strtol(helperLong, NULL, 16)*0.1;

      strcpy(helperLong,a2101[4][6]);
      strcat(helperLong,a2101[5][6]);
      strcat(helperLong,a2101[6][6]);
      strcat(helperLong,a2101[7][6]);
      evData[21][0]=strtol(helperLong, NULL, 16)*0.1;
    }
    if(strcmp(a2101[0][7],"27")==0)
    {
      strcpy(helperLong,a2101[1][7]);
      strcat(helperLong,a2101[2][7]);
      strcat(helperLong,a2101[3][7]);
      strcat(helperLong,a2101[4][7]);
      evData[22][0]=strtol(helperLong, NULL, 16)/3600.0;
      strcpy(helper,a2101[6][7]);
      strcat(helper,a2101[7][7]);
     evData[23][0]=hex2uint16(helper)*0.1;
   }
   if(strcmp(a2101[0][8],"28")==0)
   {
     strcpy(helper,a2101[5][8]);
     strcat(helper,a2101[6][8]);
    evData[24][0]=hex2uint16(helper);


		 }
		 for(int i=0;i<(sizeof(evData)/sizeof(evData[0]));i++)
		 {
			 Serial.println(evData[i][0]);
		 }

		Serial.println("end");


}

bool errorHandling()
{char buffer[50];
  Serial.println("Advanced Error Handling!");
  evSendCommand("\r",buffer,sizeof(buffer), 2000);
  delay(500);
  obd.reset();
  delay(500);
  evInit();
  delay(500);
  errors=0;
/**obd.enterLowPowerMode();
delay(1000);
obd.leaveLowPowerMode();
delay(1000);
obd.sendCommand("ATWS\r", buffer, sizeof(buffer));
delay(1000);
obd.reset();
evInit();
*/


}

bool getEvPid(const char* evCommand)
{	if (errors>9)
  {Serial.print("Errors: " );
    Serial.println(errors);
    errorHandling();
  }
  char buffer[300];
  char * pointer;
  int x =0;
  int y=0;
	byte case210X=0;
//if (!evSendCommand(evCommand, buffer, sizeof(buffer), 6000) || obd.checkErrorMessage(buffer))
evSendCommand(evCommand,buffer,sizeof(buffer), 1000);
#if !CONNECT_OBD

if(strncmp(evCommand, "2105",4)  == 0)
{strncpy(buffer, "7EC 10 2D 61 05 FF FF FF FF \r 7EC 21 00 00 00 00 00 1C 1C \r 7EC 22 1C 1A 1B 1C 1B 26 48 \r 7EC 23 26 48 00 01 50 1A 1B  \r 7EC 24 03 E8 1A 03 E8 09 9B \r 7EC 25 00 29 00 00 00 00 00 \r 7EC 26 00 00 00 00 00 00 00", sizeof(buffer));
}
else if(strncmp(evCommand, "2101",4)  == 0)
{strncpy(buffer, "7EC 10 3D 61 01 FF FF FF FF \r 7EC 21 59 26 48 26 48 A3 FF \r 7EC 22 A2 0D B3 1B 1A 1B 1B \r 7EC 23 1B 1A 1C 00 19 B6 42 \r 7EC 24 B6 13 00 00 91 00 00 \r 7EC 25 BE 99 00 00 BF 58 00 \r 7EC 26 00 46 34 00 00 44 BE \r 7EC 27 00 28 07 BB 09 01 5E \r 7EC 28 00 00 00 00 03 E8 00 \r 7EC 28 00 00 00 00 03 E8 00 01 FF FF FF FF", sizeof(buffer));
}
else if(strncmp(evCommand, "2102",4)  == 0)
{strncpy(buffer, "7EC 10 26 61 02 FF FF FF FF \r 7EC 21 C2 C2 C2 C2 C2 C2 C2 \r 7EC 22 C2 C2 C2 C2 C2 C2 C2 \r 7EC 23 C2 C2 C2 C2 C2 C2 C2 \r 7EC 24 C2 C2 C2 C2 C2 C2 C2 \r 7EC 25 C2 C2 C2 C2 00 00 00", sizeof(buffer));
}

#endif
      //Serial.println();
			if (strncmp(buffer, "7EC 10 2D",9)  == 0)  //answer to 210X - we are talking
			{ case210X=5;
			}
			if (strncmp(buffer, "7EC 10 3D",9)  == 0)  //answer to 210X - we are talking
			{ case210X=1;
		 }
		 if (strncmp(buffer, "7EC 10 26",9)  == 0)  //answer to 210X - we are talking
		 { case210X=2;
		 }


       if (strncmp(buffer, "7EC 10",6)  == 0)  //answer to 210X - we are talking
       {  pointer = strtok (buffer," ");

          while (pointer != NULL)
            {  if(!strncmp(pointer,"7EC",3) ==0)
              {  if(strncmp(pointer,"\r",1) ==0)
                    {
                      y++;
                      x=0;
                    }
                  else
                    {
  										switch (case210X)
  										{ case 5:
  										  strcpy(a2105[x][y],pointer);
  	                    break;

  											case 1:
  	                    strcpy(a2101[x][y],pointer);
                        break;

  											case 2:
  											strcpy(a2102[x][y],pointer);
  											break;

  											default:
  											Serial.println("NOT 210X");
  											break;
  										}
                    x++;
                  }
          }
            pointer = strtok (NULL, " ");
        }
        errors=0;
        needNewData=false;
        return true;
       }
       else
       { evSendCommand("\r",buffer,sizeof(buffer), 500);
         errors++;
         delay(100);
         return false;
       }

  /**Serial.print("Array a2101[4][1] am Ende: ");
  Serial.println(a2101[4][1]);
    Serial.print("Array a2101[4][4] am Ende: ");
  Serial.println(a2101[4][4]);
	Serial.print("Array a2102[2][2] am Ende: ");
	Serial.println(a2102[2][2]);
		Serial.print("Array a2102[4][4] am Ende: ");
	Serial.println(a2102[4][4]);
	Serial.print("Array a2105[2][2] am Ende: ");
	Serial.println(a2105[2][2]);
		Serial.print("Array a2105[4][4] am Ende: ");
	Serial.println(a2105[4][4]);
  */


}

int evSendCommand(const char* cmd, char* buf, int bufsize, unsigned int timeout)
{ obd.write(cmd);
	return evReceive(buf, bufsize, timeout);
}


#ifdef DEBUG
void evDebugOutput(const char *s)
{
	DEBUG.print('[');
	DEBUG.print(millis());
	DEBUG.print(']');
	DEBUG.println(s);
}
#endif


int evReceive(char* buffer, int bufsize, unsigned int timeout)
{//Serial.println("Rec STARTS HERE");
	int n = 0;
	bool eos = false;
	bool matched = false;
	//portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
	uint32_t t = millis();
	do {
		while (digitalRead(SPI_PIN_READY) == HIGH) {
			delay(1);
			if (millis() - t > timeout) {
#ifdef DEBUG
				evDebugOutput("NO READY SIGNAL");
#endif
				break;
			}
		}
#if SPI_SAFE_MODE
		delay(10);
#endif
//		taskYIELD();
//		portENTER_CRITICAL(&mux);
		digitalWrite(SPI_PIN_CS, LOW);
		while (digitalRead(SPI_PIN_READY) == LOW && millis() - t < timeout) {
			char c = SPI.transfer(' ');
/**			Serial.print(c);
			if(c==0xD)
{
	Serial.println("");
}*/
			if (eos) continue;
			if (!matched) {
				// match header
				buffer[n++] = c;
				if (n == sizeof(header)) {
		matched = memcmp(buffer, header, sizeof(header)) == 0;
		if (matched) {
			n = 0;
		} else {
			memmove(buffer, buffer + 1, --n);
		}
	}

				continue;
			}
			if (n > 3 && c == '.' && buffer[n - 1] == '.' && buffer[n - 2] == '.') {
				// SEARCHING...
				n = 0;
				timeout += OBD_TIMEOUT_LONG;
			} else if (c != 0 && c != 0xff) {
				if (n == bufsize - 1) {
					int bytesDumped = dumpLine(buffer, n);
					n -= bytesDumped;
#ifdef DEBUG
					evDebugOutput("BUFFER FULL");
#endif
				}
				buffer[n] = c;
				if (n > 0) {
					eos = (c == 0x9 && buffer[n - 1] =='>');
				}
				n++;
			}
		}
		digitalWrite(SPI_PIN_CS, HIGH);
    SPI.endTransaction();
	    //portEXIT_CRITICAL(&mux);
	} while (!eos && millis() - t < timeout);
#ifdef DEBUG
	if (!eos && millis() - t >= timeout) {
		// timed out
	}
#endif
	if (eos) {
		// eliminate ending char
		n -= 2;
	}
	buffer[n] = 0;
#ifdef DEBUG
	evDebugOutput(buffer);
#endif
	// wait for READY pin to restore high level so SPI bus is released
	while (digitalRead(SPI_PIN_READY) == LOW) delay(1);
	return n;
}





bool evInit()
{
	const char *initcmd[] = {"ATZ\r", "ATE0\r", "ATH1\r", "ATSP6\r", "ATCFC1\r","ATCM7FF\r"};
	char buffer[300];
	byte stage;

	for (byte n = 0; n < 3; n++) {
    Serial.print(n);
		stage = 0;
		if (n != 0) obd.reset();
		for (byte i = 0; i < sizeof(initcmd) / sizeof(initcmd[0]); i++) {
			delay(10);
			if (!obd.sendCommand(initcmd[i], buffer, sizeof(buffer), OBD_TIMEOUT_SHORT)) {
				continue;
			}
		}
		stage = 1;
	/**		sprintf(buffer, "ATSP6\r");
			delay(10);
			if (!obd.sendCommand(buffer, buffer, sizeof(buffer), OBD_TIMEOUT_SHORT) || !strstr(buffer, "OK")) {
				continue;
			}
		stage = 2;
		delay(10);
		if (!obd.sendCommand("ATCFC1\r", buffer, sizeof(buffer), OBD_TIMEOUT_LONG) || !strstr(buffer, "OK")) {

      continue;
		}

    delay(10);
    if (!obd.sendCommand("ATCM7FF\r", buffer, sizeof(buffer), OBD_TIMEOUT_LONG) || !strstr(buffer, "OK")) {
      continue;
    }

*/
    delay(10);

    if (obd.sendCommand("ATCF7EC\r", buffer, sizeof(buffer), OBD_TIMEOUT_LONG) || strstr(buffer, "OK")) {
      stage=2;
      n=3;
      continue;
    }



    delay(300);


	}
	if (stage == 2) {


		return true;
	} else {
#ifdef DEBUG
		DEBUG.print("Stage:");
		DEBUG.println(stage);
#endif
		obd.reset();
		return false;
	}
}






void loop() {
  uint32_t ts = millis();
//  digitalWrite(PIN_LED, HIGH);
  // put your main code here, to run repeatedly:


if(needNewData)
{

#if CONNECT_OBD
  if (!connected) {
    digitalWrite(PIN_LED, HIGH);
    Serial.print("Connecting to OBD...");
    if (evInit()) {
      Serial.println("OK");
      connected = true;
    } else {
      Serial.println();
    }
    digitalWrite(PIN_LED, LOW);
    return;
  }
#endif

  int value;
  Serial.print('[');
  Serial.print(millis());
  Serial.print("] #");
  Serial.print(count++);
  delay(300);
while(!getEvPid("2102\r"))
{Serial.print("..wait for obd data:");
Serial.println(errors);
}
delay(300);
while(!getEvPid("2105\r"))
{Serial.print("..wait for obd data:");
Serial.println(errors);
}
delay(300);
while(!getEvPid("2101\r"))
{Serial.print("..wait for obd data:");
Serial.println(errors);
}
storeEvData();
Serial.print(" BATTERY:");
Serial.print(obd.getVoltage());
Serial.print('V');
#ifdef ESP32
int temp = (int)readChipTemperature() * 165 / 255 - 40;
Serial.print(" CPU TEMP:");
Serial.print(temp);
#endif
Serial.println();
}



  if (obd.errors > 2) {
    Serial.println("OBD disconnected");
    connected = false;
    obd.reset();
  }
  digitalWrite(PIN_LED, LOW);



if(millis()-minute5>1000*60*1)			//every 5 Minutes do this
{minute5=millis();
needNewData=true;
}


  delay(100);
}
