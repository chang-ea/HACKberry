/*
    Arduino micro code for HACKberry.
    Origially created by exiii Inc.
    edited by Genta Kondo on 2017/6/11
*/
#include <Servo.h>

//Settings
const boolean isRight = 1;//right:1, left:0
int speed = 4;  //speed of opening and closing arm

//For right hand, find optimal values of ThumbMin, IndexMax and OtherMax first.
//For left hand, find optimal values of ThumbMax, IndexMin and OtherMin first.
//Then, calculate the remaining values by following rules.
//Difference of ThumbMin and ThumbMax is 86
//Difference of IndexMin and IndexMax is 117
//Difference of OtherMin and OtherMax is 55
//Calibration code
const int outThumbMax = 140;//right:open, left:close
const int outIndexMax = 140;//right:open, left:close
const int outOtherMax = 120;//right:open, left:close

const int outThumbMin = 140-86;//right:close, left:open
const int outIndexMin = 140-117; //right:close, left:open
const int outOtherMin = 120-55;//right:close, left:open

//These variables are not used anywhere in this code...
const int speedMax = 6; //This might be used just to tell the user not to change int speed (line 10) up to 6 nor down to 0
const int speedMin = 0; 
const int speedReverse = -3;
const int thSpeedReverse = 15;//0-100
const int thSpeedZero = 30;//0-100

//bool for turning on the serial monitor, can be 1 if you want data printed on serial monitor
const boolean onSerial = 0;

//Hardware
Servo servoIndex; //index finger
Servo servoOther; //other three fingers
Servo servoThumb; //thumb
int pinCalib; //start calibration
//int pinTBD;
int pinThumb; //open/close thumb
int pinOther; //lock/unlock other three fingers
int pinSensor = A1;  //myoWare sensor input1
int pinSensor2 = A2; //myoWare sensor input2

//Software
boolean isThumbOpen = 1; //boolean for whether thumb is turned to outThumbMax or not
boolean isOtherLock = 0; //boolean for whether other fingers is locked
int swCount0, swCount1, swCount2, swCount3 = 0;
int sensorValue = 0; // value read from the sensor 1
int sensorValue2 = 0; // value read from the sensor 2
int sensorMax = 700;
int sensorMin = 0;
int sensorMax2 = 700;
int sensorMin2 = 0;
int position = 0;
const int positionMax = 100;
const int positionMin = 0;
int prePosition = 0;
int outThumb, outIndex, outOther = 90;
int outThumbOpen, outThumbClose, outIndexOpen, outIndexClose, outOtherOpen, outOtherClose;

void setup() {
  Serial.begin(9600); //baud rate
  //Initializations for right-handed arm
  if (isRight) { 
    pinCalib =  A6;
    //pinTBD =  A7;
    pinThumb =  A0;
    pinOther =  10;
    //refer to line 19-25
    outThumbOpen = outThumbMax; //outThumbOpen = 140
    outThumbClose = outThumbMin; //outThumbClose = 140-86, and so forth
    outIndexOpen = outIndexMax; outIndexClose = outIndexMin;
    outOtherOpen = outOtherMax; outOtherClose = outOtherMin;
  }
  //Initializations for a left-handed arm
  else {
    pinCalib =  A6;
    //pinTBD =  A7;
    pinThumb =  A0;
    pinOther =  10;
    //refer to line 19-25
    outThumbOpen = outThumbMin; //outThumbOpen = 140-86
    outThumbClose = outThumbMax; //outThumbClose = 140 
    outIndexOpen = outIndexMin; outIndexClose = outIndexMax;
    outOtherOpen = outOtherMin; outOtherClose = outOtherMax;
  }
  
  servoIndex.attach(5); //attaches servo of index finger to digital pin 5
  servoOther.attach(6); //attaches servo of other fingers to digital pin 6
  servoThumb.attach(9); //attaches servo of thumb to digital pin 9
  //pinMode(pinCalib, INPUT);//A6  //commented out code
  //digitalWrite(pinCalib, HIGH);
  //pinMode(pinTBD, INPUT);//A5
  //digitalWrite(pinTBD, HIGH);
  pinMode(pinThumb, INPUT);//A0 to thumb
  digitalWrite(pinThumb, HIGH);
  pinMode(pinOther, INPUT);//Digital 10 to other fingers
  digitalWrite(pinOther, HIGH);
}

void loop() {
  //==waiting for calibration==
  if (onSerial) Serial.println("======Waiting for Calibration======");
  while (1) {
    servoIndex.write(outIndexOpen); //Writing max value to servo
    servoOther.write(outOtherOpen); //Writing max value to servo
    servoThumb.write(outThumbOpen); //Writing max value to servo

    if (onSerial) serialMonitor();
    delay(10);
    if (DigitalRead(pinCalib) == LOW) //digitalReading an analog pin, if it's below a certain threshold then calibrate
    {
      calibration(); //refer to line 194
      break;
    }
  }
  //==control==
  position = positionMin;
  prePosition = positionMin;
  while (1) {
    if (DigitalRead(pinCalib) == LOW) swCount0 += 1;
    else swCount0 = 0;
    if (swCount0 == 10) {
      swCount0 = 0;
      calibration();
    }
    if (digitalRead(pinThumb) == LOW) swCount2 += 1;
    else swCount2 = 0;
    if (swCount2 == 10) {
      swCount2 = 0;
      isThumbOpen = !isThumbOpen;
      while (digitalRead(pinThumb) == LOW) delay(1);
    }
    if (digitalRead(pinOther) == LOW) swCount3 += 1;//A3
    else swCount3 = 0;
    if (swCount3 == 10) {
      swCount3 = 0;
      isOtherLock = !isOtherLock;
      while (digitalRead(pinOther) == LOW) delay(1);
    }

    sensorValue = readSensor(); //Read sensor value, refer to line 165
    sensorValue2 = readSensor2(); //Reads sensor value, refer to line 176

    delay(25);
    if (sensorValue < sensorMin) sensorValue = sensorMin;
    else if (sensorValue > sensorMax) sensorValue = sensorMax;
    if (sensorValue2 < sensorMin) sensorValue2 = sensorMin;
    else if (sensorValue2 > sensorMax) sensorValue2 = sensorMax;

    if (sensorValue > ((sensorMax - sensorMin) / 2 + sensorMin) )position = prePosition + speed;
    else if (sensorValue2 > ((sensorMax2 - sensorMin2) / 2 + sensorMin2) )position = prePosition - speed;
    else position = prePosition;
    if (position < positionMin) position = positionMin;
    if (position > positionMax) position = positionMax;
    prePosition = position;

    outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose);
    servoIndex.write(outIndex);
    if (!isOtherLock) {
      outOther = map(position, positionMin, positionMax, outOtherOpen, outOtherClose);
      servoOther.write(outOther);
    }
    if (isThumbOpen) servoThumb.write(outThumbOpen);
    else servoThumb.write(outThumbClose);
    if (onSerial) serialMonitor();
  }
}

//掌屈側センサの読み取り Palm sensor reading
int readSensor() {
  int i, sval; 
  for (i = 0; i < 10; i++) {
    sval += analogRead(pinSensor);
  }
  sval = sval / 10; 
  return sval;
}

//背屈側センサの読み取り Dorsal muscle of arm
int readSensor2() {
  int i, sval;
  for (i = 0; i < 10; i++) {
    sval += analogRead(pinSensor2);
  }
  sval = sval / 10;
  return sval;
}

void calibration() {
  outIndex = outIndexOpen; // Setting current index servo value to max
  servoIndex.write(outIndexOpen); // Writing max value to servo
  servoOther.write(outOtherClose); // Writing max value to servo
  servoThumb.write(outThumbOpen); // Writing max value to servo
  position = positionMin; //position = 0
  prePosition = positionMin; //prePosition = 0

  delay(200);
  if (onSerial) Serial.println("======calibration start======");

  sensorMax = readSensor(); // reading sensor 1
  sensorMin = sensorMax - 50; // setting lower bound 1
  sensorMax2 = readSensor2(); // reading sensor 2
  sensorMin2 = sensorMax2 - 50; // setting lower bound 2
  unsigned long time = millis(); // setting millis() variable
  while ( millis() < time + 4000 ) // 4 second while loop
    {
    sensorValue = readSensor(); 
    sensorValue2 = readSensor2();
    delay(25);
    //calibrating max and min of sensor 1 & 2
    if ( sensorValue < sensorMin ) sensorMin = sensorValue; 
    else if ( sensorValue > sensorMax )sensorMax = sensorValue;
    if ( sensorValue2 < sensorMin2 ) sensorMin2 = sensorValue2;
    else if ( sensorValue2 > sensorMax2 )sensorMax2 = sensorValue2;
    
    //logic to determine how far the index finger should extend, then calls function map() below
    if (sensorValue > ((sensorMax - sensorMin) / 2 + sensorMin) ) position = prePosition + speed; 
    else if (sensorValue2 > ((sensorMax2 - sensorMin2) / 2 + sensorMin2) ) position = prePosition - speed;
    else position = prePosition;
    if (position < positionMin) position = positionMin; 
    if (position > positionMax) position = positionMax;
    prePosition = position;
    outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose); //dependent on sensorValue and sensorValue2
    
    servoIndex.write(outIndex); // extends index finger servo
      
    if (onSerial) serialMonitor(); //opens serialMonitor?
  }
  if (onSerial)  Serial.println("======calibration finish======");
}

void serialMonitor() {
  Serial.print("Min="); Serial.print(sensorMin);
  Serial.print(",Max="); Serial.print(sensorMax);
  Serial.print(",sensor="); Serial.print(sensorValue);
  Serial.print(",speed="); Serial.print(speed);
  Serial.print(",position="); Serial.print(position);
  Serial.print(",outIndex="); Serial.print(outIndex);
  Serial.print(",isThumbOpen="); Serial.print(isThumbOpen);
  Serial.print(",isOtherLock="); Serial.println(isOtherLock);
}

//アナログ入力をデジタル入力として使う関数　
boolean DigitalRead(const int pin) {
  if (analogRead(pin) > 512)return 1;
  else return 0;
}
