 
#include <SPI.h>
//#include "Adafruit_MAX31855.h"
//#include <LiquidCrystal.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS A1 
#define SENSOR_RESOLUTION 9

/* Display and keyboard */
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//const int rs = A0, en = 13, d4 = 12, d5 = 11, d6 = 10, d7 = 9;
//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // 
LiquidCrystal_I2C lcd(0x27,16,2); 

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {10, 9, 8, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 5, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); 
  
int cycles = 0;
int shift = 5;

// Termostat object
OneWire oneWire(ONE_WIRE_BUS); 
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
//DeviceAddress sensorDeviceAddress;
 
/* PCR VARIABLES*/
double DENATURE_TEMP = 92;
double ANNEALING_TEMP = 58.00;
double EXTENSION_TEMP = 70;

// Phase durations in ms. Suggested adding 3-5 seconds to
// the recommended times because it takes a second or two
// for the temps to stabilize
unsigned int DENATURE_TIME = 60000;
unsigned int ANNEALING_TIME= 60000;
unsigned int EXTENSION_TIME = 60000;

// Most protocols suggest having a longer denature time during the first cycle
// and a longer extension time for the final cycle.
unsigned int INITIAL_DENATURE_TIME = 180000;
unsigned int FINAL_EXTENSION_TIME = 360000;

// how many cycles we should do. (most protocols recommend 32-35)
int NUM_CYCLES = 32;  
 
/* Hardware variables */
int heatPin =  A5;//7;  // pin that controls the relay w resistors
/*
// Thermocouple pins
int MAXDO =   A0;
int MAXCS  = A1;
int MAXCLK = A2;
// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
*/
int fanPin = 13;//A4;//9; // pin for controling the fan
 
//safety vars
short ROOM_TEMP = 10; // if initial temp is below this, we assume thermocouple is diconnected or not working
short MAX_ALLOWED_TEMP = 100; // we never go above 100C
double MAX_HEAT_INCREASE = 2.5; // we never increase the temp by more than 2.5 degrees per 650ms


/* stuff that the program keeps track of */
short CURRENT_CYCLE = 0; // current cycle (kept track by the program)

// current phase. H-> heating up
char CURRENT_PHASE='H'; 

unsigned long time;  // used to track how long program is running
double curTemp; // current temperature
 
void startupMenu() {
      lcd.print("Standard program"); 
      lcd.setCursor(0, 1);
      lcd.print("*=YES #=NO"); 
      char key = keypad.getKey();
      while( key != '*'  &&  key != '#' )
      {
      key = keypad.getKey(); //UPDATE VALUE 
    }
      lcd.clear();
  
      if( key != '*')
      {
         customProgram();
      } 
}

int readVal()
{  
   int key  = keypad.getKey();
   int key2 = keypad.getKey();
   while( key == NO_KEY )
   { key = keypad.getKey() ;  }
   key = key -48;
   lcd.print(key);
   while( key2 == NO_KEY )
   { key2 = keypad.getKey();    }
   key2 = key2 -48;
   lcd.print(key2);   
   return key*10+key2;
}

void customProgram() {

     lcd.setCursor(0, 0);
     lcd.print("Insert only "); 
     lcd.setCursor(0, 1);
     lcd.print("numeric values"); 
     delay(2000);
     lcd.clear();
  
     // Select number of cycle
     lcd.setCursor(0, 0);
     lcd.print("Number of cycles"); 
     lcd.setCursor(0, 1);
     lcd.print("2 digits: "); 
     NUM_CYCLES =   readVal();
     delay(2000);
     lcd.clear();
     
     // Select DENATURE_TEMP
     lcd.setCursor(0, 0);
     lcd.print("Denature temp. "); 
     lcd.setCursor(0, 1);
     lcd.print("in celcius: ");
     DENATURE_TEMP = readVal();
     delay(2000);
     lcd.clear();
                   
     // Select ANNEALING_TEMP
     lcd.setCursor(0, 0);
     lcd.print("Annealing temp."); 
     lcd.setCursor(0, 1);
     lcd.print("in celcius: "); 
     ANNEALING_TEMP =  readVal(); 
     delay(2000);
     lcd.clear(); 
     
     // Select EXTENSION_TEMP
     lcd.setCursor(0, 0);
     lcd.print("Extension temp."); 
     lcd.setCursor(0, 1);
     lcd.print("in celcius: "); 
     EXTENSION_TEMP =   readVal(); 
     delay(2000);
     lcd.clear(); 
           
     lcd.setCursor(0, 0);
     lcd.print("Now insert "); 
     lcd.setCursor(0, 1);
     lcd.print("always 5 digits:"); 
     delay(3000);
     lcd.clear(); 
     
     // Select DENATURE_TIME
     lcd.setCursor(0, 0);
     lcd.print("Denature time"); 
     lcd.setCursor(0, 1);
     lcd.print("in ms "); 
     int key =  NO_KEY; 
     while( key == NO_KEY )
          { key = keypad.getKey();    }
     key = key -48;
     lcd.print(key);
     int first =  readVal(); 
     int second = readVal(); 
     DENATURE_TIME = key*10000+first*100+second;
     delay(2000);
     lcd.clear();  
 
     // Select ANNEALING_TIME
     lcd.setCursor(0, 0);
     lcd.print("Insert annealing "); 
     lcd.setCursor(0, 1);
     lcd.print("time in ms ");  
     key =  NO_KEY; 
     while( key == NO_KEY )
          { key = keypad.getKey();    }
     key = key -48;
     lcd.print(key);
     first =  readVal(); 
     second = readVal(); 
     ANNEALING_TIME = key*10000+first*100+second;
     delay(2000);
     lcd.clear();  
           
     // Select EXTENSION_TIME
     lcd.setCursor(0, 0);
     lcd.print("Insert extension "); 
     lcd.setCursor(0, 1);
     lcd.print("time in ms "); 
     key =  NO_KEY; 
     while( key == NO_KEY )
          { key = keypad.getKey();    }
     key = key -48;
     lcd.print(key);
     first =  readVal(); 
     second = readVal(); 
     EXTENSION_TIME = key*10000+first*100+second;
     delay(2000);
     lcd.clear();  
}

/* Print out current phase, temperature, how long it's been running, etc
startTime -> when specific phase started running in ms
*/
void printTempStats(unsigned long startTime) {
   lcd.clear();
   unsigned long timeElapsed = millis() - startTime;
   lcd.setCursor(0, 0);
   lcd.print("CCL: ");
   lcd.print(CURRENT_CYCLE);
   lcd.print(" PHS: ");
   lcd.print(CURRENT_PHASE);
   lcd.setCursor(0, 1);
   //lcd.print("ET:");
  // lcd.print(timeElapsed);
  // lcd.print(" TT:");
 //  lcd.print(millis());
   lcd.print("TMP:  ");
   lcd.println(curTemp);
   lcd.print("  ");
}

/* Heat up to the desired temperature.
maxTemp-> Temperature we should heat up to
printTemps -> whether or not we should print debug stats
*/
boolean heatUp(double maxTemp, boolean printTemps = true){
  unsigned long startTime = millis(); 

  sensors.requestTemperatures();  
 // double prevTemp = thermocouple.readCelsius() - shift;
 // curTemp = thermocouple.readCelsius() -shift;
  double prevTemp = sensors.getTempCByIndex(0);
  sensors.requestTemperatures(); 
  curTemp = sensors.getTempCByIndex(0);
 
  if (curTemp < ROOM_TEMP) {
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.println("STARTING TMP");
   lcd.setCursor(0, 1);
   lcd.println("TOO LOW");
   lcd.println(prevTemp);
   delay(2000);
   return false;
  }
  int curIteration = 0;
  
  while (curTemp < maxTemp) {
    curIteration++;
    sensors.requestTemperatures(); 
//    int pulseDuration = min(650, ((600*(maxTemp-curTemp))+80)); // as we approach desired temp, heat up slower
    int pulseDuration = max(1000, ((950+(maxTemp-curTemp))+50)); // as we approach desired temp, heat up slower
    digitalWrite(heatPin, HIGH);
    delay(pulseDuration);
    digitalWrite(heatPin, LOW);

    curTemp=sensors.getTempCByIndex(0);// thermocouple.readCelsius() - shift;
    if(curTemp >= maxTemp)
       break;
    if(printTemps) {
       printTempStats(startTime);
    }

   if((maxTemp-curTemp) < 1 || curIteration % 30 == 0) {
     // as we approach desired temperature, wait a little bit longer between heat
     // cycles to make sure we don't overheat. It takes a while for heat to transfer
     // between resistors and heating block. As a sanity check, also do this every 20
     // iterations to make sure we're not overheating
      do  {
        prevTemp = curTemp;
        delay(15);//250 
        sensors.requestTemperatures();
        curTemp = sensors.getTempCByIndex(0); //thermocouple.readCelsius() - shift;
      }      while(curTemp > prevTemp);
    }

   if(curTemp >= maxTemp)
     break;
     
   if ((curIteration%2) == 0) {
    if(curTemp < (prevTemp-1.25)) {
      lcd.setCursor(0, 0);
      lcd.print("Temperature is not increasing... ");
      lcd.print(curTemp);
      lcd.print("   ");
      lcd.println(prevTemp);
      delay(2000);
      return false; 
    }
   } else {   
      prevTemp = curTemp;
   }
     
   while ((curTemp-prevTemp) >= MAX_HEAT_INCREASE) {
     prevTemp = curTemp;
     lcd.setCursor(0, 0);
     lcd.print("HEATING UP TOO FAST! ");
     delay(1000);
     sensors.requestTemperatures();
     curTemp = sensors.getTempCByIndex(0); //thermocouple.readCelsius() -shift;
     lcd.println(curTemp);
   }
   
   while(curTemp >= MAX_ALLOWED_TEMP) {
     delay(2000);
     lcd.setCursor(0, 0);
     lcd.print("OVERHEATING");
     lcd.println(curTemp);
   }
   
  } 
  return true;
}


/* Cool down to the desired temperature by turning on the fan. 
minTemp-> Temperature we want to cool off to
maxTimeout -> how often we poll the thermocouple (300ms is good)
printTemps -> whether or not to print out stats
*/
void coolDown(double minTemp, int maxTimeout = 300, boolean printTemps = true) {  
  unsigned long startTime = millis(); 
 // while ((curTemp = thermocouple.readCelsius() - shift) > (minTemp+0.75)) {

   while ((curTemp =  sensors.getTempCByIndex(0) ) > (minTemp+0.75)) {
    // I've found that turning off the fan a bit before the minTemp is reached
    // is best (because fan blades continue to rotate even after fan is turned off.
    digitalWrite(fanPin, HIGH);
    if(printTemps) {
       printTempStats(startTime);
     }
   delay(maxTimeout);
   sensors.requestTemperatures();
   } 
   digitalWrite(fanPin, LOW);
}


/* 
Try to stay close to the desired temperature by making micro adjustments to the 
resistors and fan. Assumes that the current temperature is already very close
to the idealTemp.
idealTemp -> desired temperature
duration ->  how long we should keep the temperature (in ms)
*/
boolean holdConstantTemp(long duration, double idealTemp) {
  unsigned long startTime = millis();
  long timeElapsed = millis() - startTime;
  // keep track of how much time passed
  while (timeElapsed < duration) {
    sensors.requestTemperatures();
    curTemp = sensors.getTempCByIndex(0); //thermocouple.readCelsius() -shift;
    printTempStats(startTime);
      if(curTemp < idealTemp) {  
        // turn up the heat for 90ms if the temperature is cooler
        digitalWrite(heatPin, HIGH);
        delay(350);
        digitalWrite(heatPin, LOW);
      } else if (curTemp > (idealTemp+0.5)) {
        // turn up the fan if the temp is too high
        // generally if temp is within 0.5degrees, don't use the fan
        // waiting for the temp to cool naturally seems to be more stable
        digitalWrite(fanPin, HIGH);
        delay(90);
        digitalWrite(fanPin, LOW);
      }
     delay(210);
     timeElapsed = millis() - startTime;
  }
  return true; 
}

/* Execute the desired number of PCR cycles.
*/
void runPCR() {
  for (; cycles < NUM_CYCLES; cycles++) {
    CURRENT_CYCLE = cycles;
    unsigned long cycleStartTime = millis();
    lcd.setCursor(0, 0);
    lcd.print("CYCLE  ");
    lcd.print(cycles);
      
    time = millis();
    lcd.setCursor(0, 1);
    lcd.print("HEATING UP");
    ;
    CURRENT_PHASE='H';
    if(!heatUp(DENATURE_TEMP)){
      delay(2000);
      // if unable to heat up, stop
      lcd.setCursor(0, 0);
      lcd.print("Unable to heat up... something is wrong :(");
      cycles = NUM_CYCLES;
      break;
    }
    
    long dif = millis() - time;
    lcd.setCursor(0, 0);
    lcd.print("***TOTAL HEAT TIME ");
    lcd.print(dif);
    //Serial.println();
   
    time = millis();
    lcd.setCursor(0, 0);
    lcd.print("DENATURATION");
    CURRENT_PHASE='D';
    if(cycles > 0) {
      holdConstantTemp(DENATURE_TIME, DENATURE_TEMP);
    } else {
      // if this is the first cycles, hold denature temp for longer
      holdConstantTemp(INITIAL_DENATURE_TIME, DENATURE_TEMP);
    }
   // Serial.println();
   
    lcd.setCursor(0, 0);
    lcd.print("COOLING");
    time = millis();
    CURRENT_PHASE='C';
    coolDown((ANNEALING_TEMP));
    dif = millis()-time;
    lcd.setCursor(0, 1);
    lcd.print("***TOTAL COOLING TIME ");
    lcd.print(dif);
    //Serial.println();
     
    lcd.setCursor(0, 0);
    lcd.print("ANNEALING");
    time = millis();
    CURRENT_PHASE='A';
    holdConstantTemp(ANNEALING_TIME, ANNEALING_TEMP);
    dif = millis()-time;
    lcd.setCursor(0, 1);
    lcd.print("***TOTAL ANNEALING TIME ");
    lcd.print(dif);
  //  Serial.println();
    
    lcd.setCursor(0, 0);
    lcd.print("HEATING UP");
    time =millis();
    CURRENT_PHASE='H';
    heatUp((EXTENSION_TEMP));
    dif = millis()-time;
    lcd.setCursor(0, 1);
    lcd.print("***TOTAL HEAT UP TIME IS ");
    lcd.print(dif);
   // Serial.println();
  
    lcd.setCursor(0, 0);
    lcd.print("EXTENSION");
    time = millis();
    CURRENT_PHASE='E';
    if (cycles<(NUM_CYCLES-1)) {
      holdConstantTemp(EXTENSION_TIME, EXTENSION_TEMP);
    } else {
       // if this is the last cycle, hold extension temp for longer
       holdConstantTemp(FINAL_EXTENSION_TIME, EXTENSION_TEMP);
    }
    dif = millis()-time;
    lcd.setCursor(0, 1);
    lcd.print("***TOTAL EXTENSION TIME IS ");
    lcd.print(dif);
    //Serial.println();
    //Serial.println();
    
    lcd.setCursor(0, 0);
    lcd.print("///TOTAL CYCLE TIME: ");
    lcd.print(millis()-cycleStartTime);
   // Serial.println();
} 
  delay(5000);
  lcd.setCursor(0, 1);
  lcd.print("DONE, Yaw loves you");
}


/* Set up all pins */
void setup() {

 sensors.begin();  
 sensors.setResolution( SENSOR_RESOLUTION); //sensorDeviceAddress,
 // set up the LCD's number of columns and rows:
 //lcd.begin(16, 2);
 lcd.init();
 lcd.backlight();//Power on the back light
 // Print a message to the LCD.
 //Serial.begin(9600);  
 
 pinMode(heatPin, OUTPUT); 
 digitalWrite(heatPin, LOW);  
 pinMode(fanPin, OUTPUT);
 digitalWrite(fanPin, LOW);
 
 // time out for 5 seconds
  // set the cursor to column 0, line 1
 // (note: line 1 is the second row, since counting begins with 0):
 //lcd.setCursor(0, 1);
 // print the number of seconds since reset:
 lcd.print("Starting in"); 
 //Serial.println("Starting in");
 for (int i = 5; i > 0; i--) {
   lcd.setCursor(0, 1);
   lcd.print("... ");
   lcd.print(i);
   delay(1000);
 }
 lcd.clear();
 
 startupMenu(); 
 //Serial.println();
 runPCR();
}
 
void loop() {     
 //nothing
}
