/******************************************************************************
A very first try to get my EV (hyundai Ioniq) connected to thinger.io

Software can be used as is and is licensed under GPLv3
******************************************************************************/
//#define _DEBUG_  //thinger debugging

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <FreematicsPlus.h>
#include <WiFiClientSecure.h>
#include <ThingerESP32.h>
#include "credentials.h"  //just a file where I store my credentials in.


// This Secion is for adjustable User Settings
uint32_t updateTimerCharge=70;       //time between the car will update data to the cloud while Charging (in Seconds) - don't go below 1 Minute
uint32_t updateTimerDrive=240;       //time between the car will update data to the cloud while Driving (in Seconds) - 0 will Disable Upload while Driving - don't go below 1 Minute
uint32_t sleepTimer=310;              //time for the OBD Arduino to sleep, when car is off (in Seconds)(Note: a sleeping Arduino won't ceck if you car goes online)
uint32_t delayBeforeSleep=300;       //delay before the Arduino falls asleep when no OBD Data is received (in seconds)
bool wifiWhileDriving=true;         //should the wifi dongle be online while driving?(to offer wifi to the ioniq itself for example?)
bool dataWhileDriving=true;         //should the Dongle send data while driving??
bool logToFile=true;
// Settings Section END



#define PIN_LED 4
#define DEBUG Serial
#define CONNECT_OBD 1


bool wifiState;

File file;




//thinger Stuff

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);




static SPISettings settings = SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0);

COBDSPI obd;
bool connected = false;
unsigned long count = 0;
bool needNewData=true;
char a2105[8][7][3];
char a2101[8][9][3];
char a2101A[8][3][3];
char a2102[8][6][3];

uint32_t ttime;
uint32_t minute5;
uint32_t lastHeartBeatTimer;
uint32_t AliveCheckTimer;
uint32_t updateCheckTimerCharge;
uint32_t updateCheckTimerDrive;


int errors=0;


int l01=0; //current active field in array for each answer
int l02=0; //current active field in array
int l05=0; //current active field in array
int l01A=0; //current active field in array


float evData[30][2];
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
25: State of Charge of Battery(Display)
Drive mode (32 park - 8 drive)

**/




const uint8_t header[4] = {0x24, 0x4f, 0x42, 0x44};  //means obd



void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
   file.close();
}






void appendFile(fs::FS &fs, const char * path, const char * message){
    //Serial.printf("Appending to file: %s\n", path);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(!file.print(message)){
            Serial.println("Append failed");
    }
//  file.close();
}




void printAndLogln(String messageString)
{  Serial.println(messageString);
  if(logToFile)
  {
    if(!file.println(messageString)){
            Serial.println("Append failed");
    }
  }

}

void printAndLog(String messageString)
{  Serial.print(messageString);
  if(logToFile)
  {
    if(!file.print(messageString)){
            Serial.println("Append failed");
    }
  }
}



void setup() {




btStop();
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  pinMode(PIN_GPS_POWER, OUTPUT); //used for turning Wifi on / off



  delay(1000);
  digitalWrite(PIN_LED, LOW);
  // turn on Wifi power

  Serial.begin(115200);

  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
    file = SD.open("/log.txt", FILE_APPEND);
    printAndLogln("---- START -----");


  //thingerIo Stuff
  thing.add_wifi(SSID, SSID_PASSWORD);
  thing["Ioniq"] >> [](pson & out){
  out["SOC"] =  evData[25][l05];
  out["Charge Power"] =  (evData[3][l01]*evData[4][l01])/-1000;
  out["12V Battery"] =  (evData[17][l01]);
  out["BatMax"] =  (evData[5][l01]);
  out["BatLow"] =  (evData[6][l01]);
};

//setting the longest UpdateTimer as AliveCheckTimer  -  so the active check will only trigger when the normal routine brings no data back
 AliveCheckTimer=updateTimerDrive+20;
  if(updateTimerDrive<updateTimerCharge)
  {AliveCheckTimer=updateTimerCharge+20;
  }

  updateCheckTimerCharge=-updateTimerCharge*1000;  //set start values to trigger on boot
  lastHeartBeatTimer=-AliveCheckTimer*1000; //set start values to trigger on boot
  updateCheckTimerDrive=-updateTimerDrive*1000; //set start values to trigger on boot

  Serial.println("WakeUp");
  byte ver = obd.begin();

#ifdef DEBUG

printAndLog("OBD Firmware Version");
printAndLogln(String(ver));

#endif




}


bool storeEvData()
{
	char helper[5];
  char helperLong[9];



 if(strcmp(a2101[0][1],"21")==0)
		 {evData[0][l01]=hex2uint16(a2101[1][1])*0.5;
			 strcpy(helper,a2101[2][1]);
			 strcat(helper,a2101[3][1]);
			evData[1][l01]=hex2uint16(helper)*0.01;
			strcpy(helper,a2101[4][1]);
			strcat(helper,a2101[5][1]);
		 evData[2][l01]=hex2uint16(helper)*0.01;
		 strcpy(helper,a2101[7][1]);
   }
    if(strcmp(a2101[0][2],"22")==0)
    {
		 strcat(helper,a2101[1][2]);
		 evData[3][l01]=(long(hex2uint16(helper)+pow(2,15))%long(pow(2,16))-pow(2,15))*0.1;
     strcpy(helper,a2101[2][2]);
     strcat(helper,a2101[3][2]);
		evData[4][l01]=hex2uint16(helper)*0.1;
    evData[5][l01]=long(hex2uint16(a2101[4][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[6][l01]=long(hex2uint16(a2101[5][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[7][l01]=long(hex2uint16(a2101[6][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[8][l01]=long(hex2uint16(a2101[7][2])+pow(2,7))%long(pow(2,8))-pow(2,7);
  }
  if(strcmp(a2101[0][3],"23")==0)
  {
    evData[9][l01]=long(hex2uint16(a2101[1][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[10][l01]=long(hex2uint16(a2101[2][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[11][l01]=long(hex2uint16(a2101[3][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[12][l01]=long(hex2uint16(a2101[5][3])+pow(2,7))%long(pow(2,8))-pow(2,7);
    evData[13][l01]=hex2uint16(a2101[6][3])*0.02;
    evData[14][l01]=hex2uint16(a2101[7][3]);
  }
  if(strcmp(a2101[0][4],"24")==0)
  {
    evData[15][l01]=hex2uint16(a2101[1][4])*0.02;
    evData[16][l01]=hex2uint16(a2101[2][4]);
    evData[17][l01]=hex2uint16(a2101[5][4])*0.1;
    strcpy(helperLong,a2101[6][4]);
    strcat(helperLong,a2101[7][4]);
    strcat(helperLong,a2101[1][5]);
    strcat(helperLong,a2101[2][5]);
  }
  if(strcmp(a2101[0][5],"25")==0)
  {
    evData[18][l01]=strtol(helperLong, NULL, 16)*0.1;
    strcpy(helperLong,a2101[3][5]);
    strcat(helperLong,a2101[4][5]);
    strcat(helperLong,a2101[5][5]);
    strcat(helperLong,a2101[6][5]);
    evData[19][l01]=strtol(helperLong, NULL, 16)*0.1;
  }
  if(strcmp(a2101[0][6],"26")==0)
  {   strcpy(helperLong,a2101[7][5]);
      strcat(helperLong,a2101[1][6]);
      strcat(helperLong,a2101[2][6]);
      strcat(helperLong,a2101[3][6]);
      evData[20][l01]=strtol(helperLong, NULL, 16)*0.1;

      strcpy(helperLong,a2101[4][6]);
      strcat(helperLong,a2101[5][6]);
      strcat(helperLong,a2101[6][6]);
      strcat(helperLong,a2101[7][6]);
      evData[21][l01]=strtol(helperLong, NULL, 16)*0.1;
    }
    if(strcmp(a2101[0][7],"27")==0)
    {
      strcpy(helperLong,a2101[1][7]);
      strcat(helperLong,a2101[2][7]);
      strcat(helperLong,a2101[3][7]);
      strcat(helperLong,a2101[4][7]);
      evData[22][l01]=strtol(helperLong, NULL, 16)/3600.0;
      strcpy(helper,a2101[6][7]);
      strcat(helper,a2101[7][7]);
     evData[23][l01]=hex2uint16(helper)*0.1;
   }
   if(strcmp(a2101[0][8],"28")==0)
   {
     strcpy(helper,a2101[5][8]);
     strcat(helper,a2101[6][8]);
    evData[24][l01]=hex2uint16(helper);


		 }

     if(strcmp(a2105[0][4],"24")==0)
     {evData[25][l05]=hex2uint16(a2105[7][4])*0.5;
     }

     if(strcmp(a2101A[0][3],"23")==0)
     {evData[26][l01A]=hex2uint16(a2101A[2][3]);
     }





#ifdef DEBUG

		 for(int i=0;i<(sizeof(evData)/sizeof(evData[0]));i++)
		 {printAndLog(String(evData[i][0]));
       printAndLog("     ");
       printAndLogln(String(evData[i][1]));

			/** Serial.print(evData[i][0]);
       Serial.print("   ");
       Serial.println(evData[i][1]);
       */

		 }
       printAndLogln(" ");

#endif


}

bool errorHandling()
{disconnectCheck();
  char buffer[50];
  printAndLogln("Advanced Error Handling!");
  evSendCommand("\r",buffer,sizeof(buffer), 2000);
  delay(1000);
  obd.reset();
  delay(1000);
  evInit();
  delay(1000);
  logTimes();
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
{	if (errors>15)
  {printAndLog("Errors: " );
   printAndLogln(String(errors));
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

else if(strncmp(evCommand, "2101\r ",6)  == 0)  //this time with 7EA header - for testing it's ok to trigger the same condition as 2101 with 7EC
{strncpy(buffer, "7EA 10 16 61 01 FF E0 00 00 \r 7EA 21 09 21 5A 06 06 16 03 \r 7EA 22 00 00 00 00 58 76 34 \r 7EA 23 04 20 00 00 00 00 00", sizeof(buffer));
}
else if(strncmp(evCommand, "2101",4)  == 0)
{strncpy(buffer, "7EC 10 3D 61 01 FF FF FF FF \r 7EC 21 59 26 48 26 48 A3 FF \r 7EC 22 A2 0D B3 1B 1A 1B 1B \r 7EC 23 1B 1A 1C 00 19 B6 42 \r 7EC 24 B6 13 00 00 91 00 00 \r 7EC 25 BE 99 00 00 BF 58 00 \r 7EC 26 00 46 34 00 00 44 BE \r 7EC 27 00 28 07 BB 09 01 5E \r 7EC 28 00 00 00 00 03 E8 00 \r 7EC 28 00 00 00 00 03 E8 00 01 FF FF FF FF", sizeof(buffer));
}
else if(strncmp(evCommand, "2102",4)  == 0)
{strncpy(buffer, "7EC 10 26 61 02 FF FF FF FF \r 7EC 21 C2 C2 C2 C2 C2 C2 C2 \r 7EC 22 C2 C2 C2 C2 C2 C2 C2 \r 7EC 23 C2 C2 C2 C2 C2 C2 C2 \r 7EC 24 C2 C2 C2 C2 C2 C2 C2 \r 7EC 25 C2 C2 C2 C2 00 00 00", sizeof(buffer));
}





#endif
      //Serial.println();
			if (strncmp(buffer, "7EC 10 2D",9)  == 0)  //answer to 2105 - we are talking
			{ case210X=5;
        l05=(l05+1)%2;
			}
			if (strncmp(buffer, "7EC 10 3D",9)  == 0)  //answer to 2101 - we are talking
			{ case210X=1;
        l01=(l01+1)%2;
		 }
		 if (strncmp(buffer, "7EC 10 26",9)  == 0)  //answer to 2102 - we are talking
		 { case210X=2;
       l02=(l01+1)%2;
		 }
     if (strncmp(buffer, "7EA 10 16",9)  == 0)  //answer to 2101 with 7ea filter  - we are talking
     { case210X=3;
       l01A=(l01A+1)%2;
     }


       if (strncmp(buffer, "7E",2)  == 0)  //answer to 210X - we are talking
       {  pointer = strtok (buffer," ");

          while (pointer != NULL)
            {  if((!strncmp(pointer,"7EC",3) ==0) && (!strncmp(pointer,"7EA",3) ==0))
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

                        case 3:
  											strcpy(a2101A[x][y],pointer);
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
        lastHeartBeatTimer=millis();
        storeEvData();
        return true;
       }
       else
       { evSendCommand("\r",buffer,sizeof(buffer), 500);
         errors++;
         delay(100);
         return false;
       }
}



int evSendCommand(const char* cmd, char* buf, int bufsize, unsigned int timeout)
{ obd.write(cmd);
	return evReceive(buf, bufsize, 5000);
}


#ifdef DEBUG
void evDebugOutput(const char *s)
{
	printAndLog("[");
	printAndLog(String(millis()));
	printAndLog("]");
	printAndLogln(String(s));
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
		Serial.print(c);
			if(c==0xD)
{
Serial.println("");
}

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


bool isCanOn()
{

  if (millis() - lastHeartBeatTimer > AliveCheckTimer*1000)
  {if (getGearPos())
    {
    #ifdef DEBUG
    printAndLogln("AliveCheck Successful");
    #endif
    return true;
    }
    else
    #ifdef DEBUG
    printAndLogln("AliveCheck Unsuccessful");
    #endif
    {return false;
    }
  }
  else
  {
  return true;
}
}



bool getGearPos()
{
  #ifdef DEBUG
	printAndLogln("get Gear Position");
#endif
  char buffer[300];
  #if CONNECT_OBD
  if (!connected)
  {
    digitalWrite(PIN_LED, HIGH);
    printAndLog("Connecting to OBD...");
    if (evInit()) {
      printAndLogln("OK");
      connected = true;
    } else {
    }
    digitalWrite(PIN_LED, LOW);

  }
#endif
#if CONNECT_OBD
evInit();
//while(!obd.sendCommand("ATCF7EA\r", buffer, sizeof(buffer), OBD_TIMEOUT_LONG) || !strstr(buffer, "OK"));
#endif
#if !CONNECT_OBD
getEvPid("2101\r ");      //if you don't have obd data and need to test -  call this to get 2101 7EA dummy data
#endif
for (int i=0; i<13; i++)
{   if(getEvPid("2105\r"))
  { i=15; //first true breaks out
    lastHeartBeatTimer=millis();
    delay(1000);
    #if CONNECT_OBD
    evInit();
    #endif
    return true;
  }
}
return false;
}


bool evInit()
{
	const char *initcmd[] = { "ATZ\r", "ATE0\r", "ATH1\r", "ATSP6\r" , "ATCFC1\r","ATCM7FF\r", "\r"};
	char buffer[300];
	byte stage;

	for (byte n = 0; n < 3; n++) {

		stage = 0;
		if (n != 0)
    {  evSendCommand("\r",buffer,sizeof(buffer), 2000);
       delay(500);
        if (obd.sendCommand("ATCF7EC\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT) || strstr(buffer, "NO READY SIGNAL") || strstr(buffer, "RECV TIMEOUT"))
        { printAndLogln("P  A  N  I  C");

          errorHandling();
        }
        disconnectCheck();
    }
		for (byte i = 0; i < sizeof(initcmd) / sizeof(initcmd[0]); i++) {
			delay(10);
			if (!obd.sendCommand(initcmd[i], buffer, sizeof(buffer), OBD_TIMEOUT_SHORT)) {
				continue;
			}
		}
		stage = 1;
    delay(10);
    if (obd.sendCommand("ATCF7EC\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT) || strstr(buffer, "OK")) {
      stage=2;
      n=3;
  continue;
    }
  delay(200);
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

void controlWifi(bool state)
{  if (state != wifiState)
  {
    #ifdef DEBUG
    		printAndLog("Wifi state changes to: ");
    		printAndLogln(String(state));
    #endif
  wifiState=state;
  digitalWrite(PIN_GPS_POWER, wifiState);

if(state)
{printAndLogln("Enable Wifi - delay 25 seconds to get WiFi online");
lastHeartBeatTimer=millis()+25000; //setting the last heartbeat to be 25 seconds in the future - this avoids to trigger isCanOn right after the wifi is up (on short timers)
delay(25000);
}

  }
}

void disconnectCheck()
{
  if (millis() - lastHeartBeatTimer > delayBeforeSleep*1000)
  {
  if(evData[3][l01]<0) //CAN is down, but last state was charging - tell your master
  { printAndLogln("No CAN - last state was charging");
    evData[3][l01]=0; //no CAN data probably means charging stopped  -send 0Amp
    controlWifi(true);
    thing.handle();
    thing.write_bucket("freematicsbucket", "Ioniq");
    thing.call_endpoint("ChargeStop", thing["Ioniq"]);
    delay(2000);
  }

    #ifdef DEBUG
    	printAndLog("Can is off for ");
  		printAndLog(String((millis()-lastHeartBeatTimer)/1000));
      printAndLog("seconds, which is more then delayBeforeSleep: ");
      printAndLog(String(delayBeforeSleep));
      printAndLog("seconds - going to sleep for ");
      printAndLogln(String(sleepTimer));
    #endif

    controlWifi(false);
    lastHeartBeatTimer=millis();

    file.close();
    delay(500);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_enable_timer_wakeup(sleepTimer*1000000);
    esp_deep_sleep_start();

  }
}


void gatherData()
{  while(!getEvPid("2101\r"))
    {   printAndLog("..wait for obd data:");
        printAndLogln(String(errors));
    }
    while(!getEvPid("2105\r"))
    {   printAndLog("..wait for obd data:");
        printAndLogln(String(errors));
    }
}

void logTimes()
{

          printAndLog("UpdatecheckTimerCharge    ");
          printAndLog(String((millis() - updateCheckTimerCharge)/1000));
          printAndLog("  :  ");
          printAndLogln(String(updateTimerCharge));

            printAndLog("updateCheckTimerDrive  ");
            printAndLog(String((millis() - updateCheckTimerDrive)/1000));
            printAndLog("  :  ");
            printAndLogln(String(updateTimerDrive));




            printAndLog("AliveCheckTimer  ");
            printAndLog(String((millis() - lastHeartBeatTimer)/1000));
            printAndLog("  :  ");
            printAndLogln(String(AliveCheckTimer));



            printAndLog("delayBeforeSleep  ");
            printAndLog(String((millis() - lastHeartBeatTimer)/1000));
            printAndLog("  :  ");
            printAndLogln(String(delayBeforeSleep));


}

void loop()
{
  if(millis()-minute5>5000)
  {
    logTimes();
    minute5=millis();
  }



    if(isCanOn())
    {if(evData[26][l01A]==32)
      {//Serial.println("Car is in P Mode");
        if(millis() - updateCheckTimerCharge > updateTimerCharge*1000)
        { gatherData();
          controlWifi(true);
          updateCheckTimerCharge=millis();

          if(evData[3][l01]<0)                                    //P Mode and negative discharge current  - we are Charging
          {
            #ifdef DEBUG
            printAndLogln("Charging");
            #endif
          }
          else if (evData[3][(l01+1)%2]<0)          //P mode on and last state was charging - now we aren't - charge stopped!
          {
            #ifdef DEBUG
            printAndLogln("P Mode and  charge stopped");
            #endif//send a Mail with some details
            thing.call_endpoint("ChargeStop", thing["Ioniq"]);
          }
          else  //last state was no charge as well - so we are just parking - let's check gearPos - maybe we'll start driving
          {
            #ifdef DEBUG
            printAndLogln("P Mode and no charge - checking gearPosition");
            #endif
            getGearPos();

          }
          printAndLogln("updating thinger");
          thing.handle();
          thing.write_bucket("freematicsbucket", "Ioniq");
          delay(500);
          printAndLog("...and sleep for ");
          printAndLogln(String(updateTimerCharge-30));
          delay(500);

          controlWifi(false);
          esp_wifi_stop();
          obd.enterLowPowerMode();
          delay(300);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
          esp_sleep_enable_timer_wakeup((updateTimerCharge-30)*1000000);
          esp_light_sleep_start();
          delay(300);
          esp_wifi_start();
          obd.leaveLowPowerMode();
          file = SD.open("/log.txt", FILE_APPEND);
        }
    }
    else                                                                              //NO P mode - we're probably driving
      {controlWifi(wifiWhileDriving);
        if(millis() - updateCheckTimerDrive > updateTimerDrive*1000)
          {if(dataWhileDriving)
             {gatherData();
              controlWifi(true);
             #ifdef DEBUG
             printAndLogln("D Mode");
             printAndLogln("updating thinger");
             #endif
             thing.handle();
             thing.write_bucket("freematicsbucket", "Ioniq");
              delay(500);
              printAndLog("...and sleep for ");
              printAndLogln(String(updateTimerDrive-30));
              delay(500);
              lastHeartBeatTimer=millis();
              file.close();
              esp_wifi_stop();
              obd.enterLowPowerMode();
              delay(300);
            //gpio_pad_hold(PIN_GPS_POWER);
            esp_sleep_enable_timer_wakeup((updateTimerDrive-30)*1000000);
            esp_light_sleep_start();
              delay(300);
              esp_wifi_start();
            obd.leaveLowPowerMode();
            file = SD.open("/log.txt", FILE_APPEND);

             }
             else
             {

             }
          getGearPos();
          updateCheckTimerDrive=millis();


          }
      }
  }
  else                                                                                                      //CAN IS OFF
    { appendFile(SD, "/log.txt","noData?\r\n");
      disconnectCheck();
    }
}
