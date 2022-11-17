#include "RF24.h"
#include "nRF24L01.h"
#include "SPI.h"
#include <LiquidCrystal_I2C.h>

#define CE_PIN 7
#define CSN_PIN 8
#define listenDelay 50
#define sendTime 150

enum ActiveMode{
  off = 0,
  distance,
  photocell,
  compass,
};

enum ActuatorMode {
  none = 0,
  vibration,
  buzzer,
  all,
};

struct sensorPayload {
  int16_t data1;
  int16_t data2;
  int16_t data3;
  int16_t data4;
  int16_t data5;

  uint8_t activeMode;
  uint8_t actuatorMode;
}inPayload;

struct terminalRequestPayload {
  uint8_t activeMode; // set/update the active mode
  uint8_t actuatorMode; // set/update the actuator mode
}outPayload;


// LCD STUFF -----------------
byte backSlash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};
byte customCharDot[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b00000,
  0b00000
};
byte customCharDotSel[8] = {
  0b00000,
  0b00000,
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
  0b00000
};

uint64_t cycleStartTime, currentCycleTime;
int16_t distL, distC, distR, lux, compassHeading; // lux = brightness
bool readNotWrite, isListening;

int cs = 3; // compass shift

const byte headToWristAddr[6] = "00001";
const byte wristToHeadAddr[6] = "00002";

ActiveMode activeMode;
ActuatorMode actuatorMode;

RF24 radio(CE_PIN, CSN_PIN);
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  distL = distC = distR = lux = compassHeading = 0;

  activeMode = distance;
  actuatorMode = vibration;

  Serial.begin(9600);
  while(!radio.begin()) Serial.println("failed wiring");
  Serial.println("wiring valid");

  radio.setAutoAck(false); // append Ack packet
  radio.setDataRate(RF24_250KBPS); // transmission rate
  radio.setPALevel(RF24_PA_LOW); // distance and energy consump.
  radio.openWritingPipe(wristToHeadAddr);
  radio.openReadingPipe(0, headToWristAddr);

  readNotWrite = true;
  isListening = true;
  radio.startListening();
  cycleStartTime = millis();

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, customCharDot);
  lcd.createChar(1, customCharDotSel);
  lcd.createChar(2, backSlash);
  lcd.setCursor(3,0);

  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
}

void loop() {
  communicate();

  switch(activeMode) {
    case 0: printAll(); break;
    case 1: printLidar(); break;
    case 2: printPhotoCell(); break;
    case 3: printCompass(); break;
  }
}

void communicate() {
  currentCycleTime = millis() - cycleStartTime;

  if (readNotWrite) {
    if(!isListening) {
      radio.startListening();
      cycleStartTime = millis();
      isListening = true;
    } 
    
    else if (currentCycleTime > listenDelay) {
      if(radio.available() > 0)
        readInPayload();
    }
  
  } else {
    if (isListening) {
      radio.stopListening();
      cycleStartTime = millis();
      isListening = false;
    }
    
    else if(currentCycleTime < sendTime) {
      outPayload.activeMode = activeMode;
      outPayload.actuatorMode = actuatorMode;
      //radio.write(&outPayload, sizeof(outPayload));
    } 
    
    else
      readNotWrite = true;
  }
}

void readInPayload() {
  radio.read(&inPayload, sizeof(inPayload));

  distL = inPayload.data1;
  distC = inPayload.data2;
  distR = inPayload.data3;
  lux = inPayload.data4;
  compassHeading = inPayload.data5;

  /*
  Serial.print("  Active mode is ");
  Serial.print(inPayload.activeMode);
  Serial.print("  Actuator mode is ");
  Serial.print(inPayload.actuatorMode);
  */
  
  Serial.print("l Data is ");
  Serial.print(distL);
  Serial.print("  c Data is ");
  Serial.print(distC);
  Serial.print("  r Data is ");
  Serial.print(distR);
  Serial.print("  lux is ");
  Serial.print(lux);
  Serial.print("  compassHeading is ");
  Serial.println(compassHeading);
}

void printAll() {
  lcd.setCursor(3,0);
  lcd.print(char(0b01111111));
  lcd.setCursor(9,0);
  lcd.print("^");
  lcd.setCursor(15,0);
  lcd.print(char(0b01111110));
  
  String dist1 = String(distL);
  int cursor1 = 4 - dist1.length();
  lcd.setCursor(0, 1);
  for (int i = 0 ; i < cursor1 ;i++) lcd.print(" ");
  lcd.print(dist1);
  lcd.print("cm");

  String dist2 = String(distC);
  int cursor2 = 10 - dist2.length();
  lcd.setCursor(6, 1);
  for (int i = 6 ; i < cursor2 ;i++) lcd.print(" ");
  lcd.print(dist2);
  lcd.print("cm");

  String dist3 = String(distR);
  int cursor3 = 16 - dist3.length();
  lcd.setCursor(12  , 1);
  for (int i = 12 ; i < cursor3 ;i++) lcd.print(" ");
  lcd.print(dist3);
  lcd.print("cm");



  int luxL = String(lux).length();
  lcd.setCursor(2-luxL,2);
  lcd.print("   ");
  lcd.setCursor(5-luxL,2);
  lcd.print(lux);
  lcd.print(" Lux");



  lcd.setCursor(11,2);
  lcd.print("  ");
  String degS = String(compassHeading);
  lcd.setCursor(13,2);
  lcd.print(degS);
  lcd.print(char(0b11011111));
  lcd.print("  ");
  

  
  lcd.setCursor(15,3);
  lcd.print(char(1));
  lcd.print(char(0));
  lcd.print(char(0));
  lcd.print(char(0));
}

void printPhotoCell() {
  int luxL = String(lux).length();
  lcd.setCursor(7-luxL,2);
  lcd.print("   ");
  lcd.setCursor(10-luxL,2);
  lcd.print(lux);
  lcd.print(" Lux");

  //print brightness bar
  int bars = (lux * 9 / 1000)+1;
  lcd.setCursor(4,1);
  lcd.print("|");
  for (int i = 0 ; i < bars; i++)
    lcd.print(char(0b11111111));
  for (int i = bars ; i <= 9; i++)
    lcd.print(" ");
  lcd.print("|");

  lcd.setCursor(15,3);
  lcd.print(char(0));
  lcd.print(char(0));
  lcd.print(char(0));
  lcd.print(char(1));
}

void printLidar() {
  lcd.setCursor(3,0);
  lcd.print(char(0b01111111));
  lcd.setCursor(9,0);
  lcd.print("^");
  lcd.setCursor(15,0);
  lcd.print(char(0b01111110));

  String dist1 = String(distL);
  int cursor1 = 4 - dist1.length();
  lcd.setCursor(cursor1, 1);
  lcd.print(dist1);
  lcd.print("cm");

  String dist2 = String(distC);
  int cursor2 = 10 - dist2.length();
  lcd.setCursor(cursor2, 1);
  lcd.print(dist2);
  lcd.print("cm");

  String dist3 = String(distR);
  int cursor3 = 16 - dist3.length();
  lcd.setCursor(cursor3, 1);
  lcd.print(dist3);
  lcd.print("cm");

  lcd.setCursor(15,3);
  lcd.print(char(0));
  lcd.print(char(1));
  lcd.print(char(0));
  lcd.print(char(0));
}

void printCompass() {
  lcd.setCursor(11,1);
  lcd.print("  ");
  String degS = String((compassHeading/10) * 10);
  lcd.setCursor(13,1);
  lcd.print(degS);
  lcd.print(char(0b11011111));
  lcd.print("  ");

  // direction
  if (compassHeading >= -23 && compassHeading <= 23) { // north
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print(" N ");
    lcd.setCursor(cs + 1,1);
    lcd.print("  |  ");
    lcd.setCursor(cs + 1,2);
    lcd.print("     ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  } 
  else if (compassHeading >= 24 && compassHeading <= 66) { // north east
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("   / ");
    lcd.setCursor(cs + 1,2);
    lcd.print("     ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }
  else if (compassHeading >= 67 && compassHeading <= 113) { // east
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("   __");
    lcd.setCursor(cs + 1,2);
    lcd.print("     ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }
  else if (compassHeading >= 114 && compassHeading <= 156) { // south east
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("     ");
    lcd.setCursor(cs + 1,2);
    lcd.print("   ");
    lcd.print(char(2));
    lcd.print(" ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }
  else if (compassHeading > 157 || compassHeading < -157) { // south
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("     ");
    lcd.setCursor(cs + 1,2);
    lcd.print("  |  ");
    lcd.setCursor(cs + 2,3);
    lcd.print(" | ");
  }
  else if (compassHeading >= -66 && compassHeading <= -24) { // north west
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print(" ");
    lcd.print(char(2));
    lcd.print("   ");
    lcd.setCursor(cs + 1,2);
    lcd.print("     ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }
  else if (compassHeading >= -113 && compassHeading <= -67) { // west
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("__   ");
    lcd.setCursor(cs + 1,2);
    lcd.print("     ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }
  else if (compassHeading >= -156 && compassHeading <= -114) { // south west
    printCompassOutline();
    lcd.setCursor(cs + 2,0);
    lcd.print("   ");
    lcd.setCursor(cs + 1,1);
    lcd.print("     ");
    lcd.setCursor(cs + 1,2);
    lcd.print(" /   ");
    lcd.setCursor(cs + 2,3);
    lcd.print("   ");
  }

  lcd.setCursor(15,3);
  lcd.print(char(0));
  lcd.print(char(0));
  lcd.print(char(1));
  lcd.print(char(0));
}

void printCompassOutline() {
  lcd.setCursor(cs + 1,0);
  lcd.print("/");
  lcd.setCursor(cs + 5,0);
  lcd.print(char(2));
  lcd.setCursor(cs,1);
  lcd.print("|");
  lcd.setCursor(cs + 6,1);
  lcd.print("|");
  lcd.setCursor(cs,2);
  lcd.print("|");
  lcd.setCursor(cs + 6,2);
  lcd.print("|");
  lcd.setCursor(cs + 1,3);
  lcd.print(char(2));
  lcd.setCursor(cs + 5,3);
  lcd.print("/");
}