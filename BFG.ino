    The Binary Function Generator is a bytebeat-based, 8-bit oscillator for
    modular systems, hosted on the Arduino microcontroller platform.
    
    Copyright (C) 2013  Erik Källman (vurma)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "wiring_private.h"
#undef round
#undef abs

// The speaker is on Arduino pin 11, which is port B pin 3, pin 17 on the PDIP.
// The signal on this pin is OC2A.

//Each comparator value has an old and a new state. Even index = new states, odd indexes = old. See setComp() for use. Increment this value by two if more comparator values are added.
int compState[4];

// Set mode values for the variable inputs of dI 0 and 1:
int m1val = 1;
int m2val = 2;
const int mDflt = 0; //Default mode
const int led0 =  4;
const int led1 =  5;
const int led2 =  6;
const int led3 =  7;

int modes[3]; //The selectable modes contains two switchable and one default id.
int rst = 0; //Function reset value

//Define the mode-specific constants:
long c0[4] = { 1, 2, 3, 4 };
long c1[4] = { 5, 6, 7, 8 };
long c2[4] = { 9, 10, 11, 12 };
long c3[4] = { 13, 14, 15, 16 };

//Set default clock value
static int clkVal = 0;

long t = 0;
int tCounter = 0;
unsigned long clkTime;

int i = 0;

long* C[4]; //All mode constants are stored in this 2D array. Accessed as: C[mode index][constant index]

static int maxPeriod = 64; //Maximum iteration steps for one period of the wave.
unsigned long waveFun[64]; //A vector to store one period of the wavefunction. Used for the comparator training mode.

int tModeFlag = 0;
int lastTModeFlag = 0;

int compAvg = 0;
int compMax = 0;

void setup()  {
  // set bit 3 in data direction register B to 1 to configure Port B pin 3 for output:
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, INPUT);
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  DDRB |= 1 << 3;
  // I forget what this does!
  TCCR2A |= 1 << COM2A1;
  // use 0x01 for divisor of 1, i.e. 31.25kHz
  // or 0x02 for divisor of 8, or 0x03 for divisor of 64 (default)
  TCCR2B = TCCR2B & 0xf8 | 0x01;

  //Store the constants defined for each mode:
  C[0] = c0;
  C[1] = c1;
  C[2] = c2;
  C[3] = c3;

  //Store the mode identifiers:
  modes[0] = mDflt;
  modes[1] = m1val;
  modes[2] = m2val;

  //Serial.begin(9600);
}

int getMode(){

  byte modeSwitch[2];
  int m = mDflt;

  //Grab the logic values from input 0 and 1:
  modeSwitch[0] = digitalRead(0);
  modeSwitch[1] = digitalRead(1);

  if((modeSwitch[0] == HIGH) && (modeSwitch[1] == HIGH)){
    //Randomly select either the default mode, m1 or m2.
    //int r = rand() % 3;
    //m = modes[r];
    m = mDflt;

  } else if(modeSwitch[0] == HIGH) {
    m = m1val;

  } else{
    m = m2val;

  }
  return m;
}

void setLEDs(int state){
  int j;

  if(state == 0){
    for(j=4;j<=8;j++){
      digitalWrite(j,LOW);
    }

  } else if(state == 1){
    for(j=4;j<=8;j++){
      digitalWrite(j,HIGH);
    }
  }
}

void blinkLED(int id, int n, int s) {
  //Toggles digital output @id from LOW to HIGH n times. Ends in state @s.
  int j;
  for(j=0;j<=n,j++){
    digitalWrite(id, HIGH);
    delay(250);
    digitalWrite(id, LOW);
  }
  digitalWrite(id,s);
}


void setClock(){

  unsigned long ms = millis();
  clkTime += ms;

  if(digitalRead(9) == HIGH){
    tCounter += tCounter;
  }

  if(tCounter == 8){
    //We have recieved 8 measures of clock. Derive a tempo:
    clkVal = clkTime / 2;
    clkTime = 0;
    tCounter = 0;
  }
}


long setFunction() {
  long fun;
  long* k;
  switch (modeId) {
  case 0:
    k = C[0];
    //OCR2A = k[0]*t&t>>k[1]*t+((t>>k[2]&5))*t&t>>k[3];
    //OCR2A = ((t<<1)^((t<<1)+(t>>7)&t>>12))|t>>(4-(1^7&(t>>19)))|t>>7;
    fun = k[0]*t&t>>8;
    break;
  }

  return fun;
}

void setTMode(long f) {
  //setTMode stores one complete period of the wavefunction and sets comparator levels according to the maximum values in order to derive triggers based on the wavefunction.

  if((lastTModeFlag == 0) && (tMode == HIGH)){
    //Reinitialize the iteration variables to start "recording" a new period of the wavefunction:
    tModeFlag = 1;
    digitalWrite(12,HIGH);
    t = 0;
    i = 0;
    delay(3000);
    blinkLED(13,8,HIGH);
    //Set the flag to start recording the wavefunction:
    lastTModeFlag = 1;
  }

  if(tModeFlag == 1){
    waveFun[i] = f;
  }

  if((i+1) == maxPeriod){
    //An entire period of the wavefunction has been captured in waveFun. Find its maximum value and arithmetic mean:

    unsigned long currMax = 0;
    unsigned int avg = 0;
    int j=0;

    for(j=0;j<=maxPeriod;j++){
      if(waveFun[j] > currMax){
        currMax = waveFun[j];
      }
      avg += waveFun[j];
    }

    //Set the comparator variables:
    compAvg = avg/maxPeriod;
    compMax = currMax;

    //Exit training mode:
    t = 0;
    i = 0;
    lastTModeFlag = 0;
    tModeFlag = 0;
    blinkLED(13,8,LOW);
    delay(2000);
  }
  return;
}

void setComp(){
  //A function that, based on compAvg and compMax, creates gate-out signals

  int compVals[2];
  compVals[1] = compMax;
  compVals[2] = compAvg;
  int j;

  for(j=0;j<2;j++) {
    int i = j*2;
    int tmpVal = compVals[j];

    if(fOut>=tmpVal){
      compState[i] = 1;
      if(compState[i] != compState[i+1]){
        //Set the gate signal to high, if its not already high:
        //analogWrite(0,255);
        digitalWrite(10,HIGH);
        compState[i+1] = 1;
      }
    } else {
      compState[i] = 0;
      //Was the previous state HIGH?
      if(compState[i] != compState[i+1]){
        //Set the gate signal to low, if its not already low:
        //analogWrite(0,0);
        digitalWrite(10,LOW);
        compState[i+1] = 0;
      }
    }
  }
}

  //TODO : remove all led-defining variables and set digital states just as numbers.
void loop() {

  //if ((++i != 64) || (digitalRead(13) == HIGH)) return;

  if (++i != maxPeriod) return;
  i = 0;
  t++;

  int tMode = digitalRead(12);
  int modeId = getMode();
  long fOut = setFunction(modeId);

  if(tMode == HIGH){
    setTMode(fOut);

  } else {
    OCR2A = fOut;
    //setClock();
    delay(clkVal);
  }
  setComp();
  return;
}
