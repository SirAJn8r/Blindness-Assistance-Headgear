#include <Arduino.h>
#include <Wire.h>
#include <TFLI2C.h>

#define leftTFL 0x12
#define centerTFL 0x11
#define rightTFL 0x10

#define leftBuz 4
#define centerBuz 5
#define rightBuz 6

#define leftLidarPower 30
#define centerLidarPower 32
#define rightLidarPower 34

#define maxDistance 6 // # of Meters away where it outputs 0
#define scalar 0.8 // Outermost bucket = 20%, 2nd is outer 20% of remaining, etc.
#define numBuckets 16 // Number of buckets to categorize distance into

// Focus on 1 distance if it this factor closer than either other distance
#define closenessFocusFactor 0.5

unsigned long timerLeft = 0;
unsigned long timerCenter = 0;
unsigned long timerRight = 0;

int intervalLeft = 2000;
int intervalCenter = 2000;
int intervalRight = 2000;

const int vibeTimeLeft = 50;
const int vibeTimeCenter = 50;
const int vibeTimeRight = 50;

unsigned long getDataTimer = 0;

int distL = 0;
int distC = 0;
int distR = 0;
TFLI2C tfl;
int16_t tfDist;

void setup()
{
  //Serial.begin(9600);
  Wire.begin();

  pinMode(leftBuz, OUTPUT);
  pinMode(centerBuz, OUTPUT);
  pinMode(rightBuz, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  interrupts();
}


void loop()
{
  unsigned long millisS = millis();
  interrupts();
  //Serial.println(intervalLeft);
  //Serial.println(intervalCenter);
  //Serial.println(intervalRight);
  //Serial.println();

  // Collect distance data only from whoever's turn it is
  if (millisS - getDataTimer > 99){
    getDataTimer = millisS;
  } else if (millisS - getDataTimer > 66){
    digitalWrite(LED_BUILTIN, LOW);
    //digitalWrite(leftLidarPower, LOW);
    //digitalWrite(centerLidarPower, HIGH);
    //digitalWrite(rightLidarPower, LOW);
    if (tfl.getData(tfDist, leftTFL)){
      if(tfDist > 600) tfDist = 600;
      distL = tfDist;
      //Serial.print("  left: ");
      //Serial.print(tfDist);
    }
  } else if (millisS - getDataTimer > 33){
    //digitalWrite(leftLidarPower, LOW);
    //digitalWrite(centerLidarPower, HIGH);
    //digitalWrite(rightLidarPower, LOW);
    if (tfl.getData(tfDist, centerTFL))
      {
        if(tfDist > 600) tfDist = 600;
        distC = tfDist;
        //Serial.print("\tcenter: ");
        //Serial.print(tfDist);
      }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    //digitalWrite(leftLidarPower, LOW);
    //digitalWrite(centerLidarPower, HIGH);
    //digitalWrite(rightLidarPower, LOW);
    if (tfl.getData(tfDist, rightTFL)){
      if(tfDist > 300) tfDist = 300;
      distR = tfDist;
      //Serial.print("\tright: ");
      //Serial.print(tfDist);
    }
  }

  // If one is much closer than the other 2, focus on that distance sensor
  bool foundClosePoint = false;
  if (distL < distC * closenessFocusFactor && distL < distR  * closenessFocusFactor){ // left is closest
    analogWrite(leftBuz, map(bucketize(distL), 0, 15, 0, 255));
    digitalWrite(centerBuz, LOW);
    digitalWrite(rightBuz, LOW);
    foundClosePoint = true;
  } else if (distC < distL  * closenessFocusFactor && distC < distR  * closenessFocusFactor) { // center is closest
    digitalWrite(leftBuz, LOW);
    analogWrite(centerBuz, map(bucketize(distC), 0, 15, 0, 255));
    digitalWrite(rightBuz, LOW);
    foundClosePoint = true;
  } else if (distR < distL  * closenessFocusFactor && distR < distC  * closenessFocusFactor){ // right is closest
    digitalWrite(leftBuz, LOW);
    digitalWrite(centerBuz, LOW);
    analogWrite(rightBuz, map(bucketize(distR), 0, 15, 0, 255));
    foundClosePoint = true;
  }
  if (!foundClosePoint){
    int distAvg = (distL + distC + distR) / 3;
    int distAUL = distAvg * (distAvg * 0.2);
    if (distL < distAUL){
      analogWrite(leftBuz, map(bucketize(distL), 0, 15, 0, 255)));
      foundClosePoint = true;
    } else {
      digitalWrite(leftBuz, LOW);
    }
    if (distC < distAUL){
      analogWrite(centerBuz, map(bucketize(distC), 0, 15, 0, 255)));
      foundClosePoint = true;
    } else {
      digitalWrite(centerBuz, LOW);
    }
    if (distR < distAUL){
      analogWrite(rightBuz, map(bucketize(distR), 0, 15, 0, 255)));
      foundClosePoint = true;
    } else {
      digitalWrite(rightBuz, LOW);
    }
  }
  if (!foundClosePoint){
    analogWrite(leftBuz, map(bucketize(distL), 0, 15, 0, 255)));
    analogWrite(centerBuz, map(bucketize(distC), 0, 15, 0, 255)));
    analogWrite(rightBuz, map(bucketize(distR), 0, 15, 0, 255)));
  }
/*
  if (distL < distR && distL < distC){ // left is closest
    analogWrite(leftBuz, map(tfDist, 0, 600, 255, 0));
    digitalWrite(centerBuz, LOW);
    digitalWrite(rightBuz, LOW);
  } else if (distC < distR && distC < distL) { // center is closest
    analogWrite(centerBuz, map(tfDist, 0, 600, 255, 0));
    digitalWrite(leftBuz, LOW);
    digitalWrite(rightBuz, LOW);
  } else { // right is closest
    analogWrite(rightBuz, map(tfDist, 0, 600, 255, 0));
    digitalWrite(centerBuz, LOW);
    digitalWrite(leftBuz, LOW);
  }
  /*
  Serial.println();
  interrupts();
  if (millisS - (timerLeft) > intervalLeft){
    timerLeft = millis();
  }
  if (millisS - (timerCenter) > intervalCenter){
    timerCenter = millis();
  }
  if (millisS - (timerRight) > intervalRight){
    timerRight = millis();
  }
  interrupts();
  
  if (millisS - timerLeft < vibeTimeLeft){
    analogWrite(leftBuz, 210);
  } else {
    analogWrite(leftBuz, 0);
  }
  if (millisS - timerCenter < vibeTimeCenter){
    analogWrite(centerBuz, 210);
  } else {
    analogWrite(centerBuz, 0);
  }
  if (millisS - timerRight < vibeTimeRight){
    analogWrite(rightBuz, 210);
  } else {
    analogWrite(rightBuz, 0);
  }
  interrupts();
  */
}

/* Groups the given distance (meters) into buckets 0-15, 15 being furthest 
 */
uint8_t bucketize(float dist) {
    uint8_t bucket = numBuckets - 1;
    float maxDist = maxDistance;

    while(dist < maxDist && bucket > 0) {
      bucket--;
      maxDist *= scalar;
    }

    return bucket;
}