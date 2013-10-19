    /* The Binary Function Computer is a bytebeat-based, 8-bit oscillator for */
    /* modular systems, hosted on the Arduino microcontroller platform. */

    /* Copyright (C) 2013  Erik Källman (vurma) */

    /* This program is free software: you can redistribute it and/or modify */
    /* it under the terms of the GNU General Public License as published by */
    /* the Free Software Foundation, either version 3 of the License, or */
    /* (at your option) any later version. */

    /* This program is distributed in the hope that it will be useful, */
    /* but WITHOUT ANY WARRANTY; without even the implied warranty of */
    /* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
    /* GNU General Public License for more details. */

    /* You should have received a copy of the GNU General Public License */
    /* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "wiring_private.h"
#undef round
#undef abs

// The speaker is on Arduino pin 11, which is port B pin 3, pin 17 on the PDIP.
// The signal on this pin is OC2A.

//Each comparator value has an old and a new state. Even index = new states, odd indexes = old. See setComp() for use. Increment this value by two if more comparator values are added.
int compState[4];
unsigned long compVals[2];
int compClk[2];
unsigned long compAvg = 0;
unsigned long compMax = 0;


// Set mode values for the variable inputs of dI 0 and 1:
int m1val = 1;
int m2val = 2;
const int mDflt = 0; //Default mode
/* const int led0 =  4; */
/* const int led1 =  5; */
/* const int led2 =  6; */
/* const int led3 =  7; */

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

int l;

long* C[4]; //All mode constants are stored in this 2D array. Accessed as: C[mode index][constant index]

static int maxIt = 16384; //Maximum iteration steps for one period of the wave.
int maxWf = 450;
unsigned long waveFun[450]; //A vector to store one period of the wavefunction. Used for the comparator training mode.

int tModeFlag = 0;
int lastTModeFlag = 0;

void setup()  {

  //OUTPUTs
    // set bit 3 in data direction register B to 1 to configure Port B pin 3 for output:
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  //INPUTs
  pinMode(3, INPUT);

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

int funMode(){

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

void blinkLED(int id, int n, int s, int d) {
  //Toggles digital output @id from LOW to HIGH @n times with a @d ms delay in between blinks. Ends in state @s.
  int j = 0;
  for(j=0;j<n;j++) {
    digitalWrite(id, HIGH);
    delay(d);
    digitalWrite(id, LOW);
    //Serial.print(78);
    delay(d);
  }
  digitalWrite(id, s);
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

long setFunction(int id) {
  long fun;
  long* k;
  switch (id){
  case 0:
    k = C[0];
    //fun = k[0]*t&t>>k[1]*t+((t>>k[2]&5))*t&t>>k[3];
    fun = ((t<<1)^((t<<1)+(t>>7)&t>>12))|t>>(4-(1^7&(t>>19)))|t>>7;
    //fun = k[0]*t&t>>8;
    break;
  }
  return fun;
}

void setTMode(long f) {
  //setTMode stores one complete period of the wavefunction and sets comparator levels according to the maximum values in order to derive triggers based on the wavefunction.

  if(lastTModeFlag == 0){
    //Reinitialize the iteration variables to start "recording" a new period of the wavefunction:

    //digitalWrite(12,HIGH);
    t = 0;
    l = 0;
    delay(1000);
    blinkLED(13,5,HIGH,50);
    //Set the flag to start recording the wavefunction:
    lastTModeFlag = 1;
  }

  if(tModeFlag == 1){
    waveFun[l] = f;
    /* Serial.print("WF value:"); */
    /* Serial.println(waveFun[l]); */
  }

  if((l) == maxWf){
    //An entire period of the wavefunction has been captured in waveFun. Find its maximum value and arithmetic mean:
    //Serial.println("==Calculating max/avg==");
    unsigned long avg = 0;
    unsigned long navg = 0;
    int j=0;

    for(j=0;j<=maxWf;j++){
      if(waveFun[j] > compMax){
        compMax = waveFun[j];
        /* Serial.println("==max=="); */
        /* Serial.println(compMax); */
      }
      if(waveFun[j] != 0) {
        avg += waveFun[j];
        navg += 1;
      }
    }

    //Set the comparator variables:

    compAvg = avg/navg;
    /* Serial.println("==avg=="); */
    /* Serial.println(compAvg); */
    //compMax = currMax;
    //Exit training mode:
    t = 0;
    l = 0;
    lastTModeFlag = 0;
    tModeFlag = 0;

    blinkLED(13,2,LOW,250);
    blinkLED(13,3,LOW,50);
    blinkLED(13,5,LOW,25);

  }
  return;
}

void toggleLED(int id){
  digitalWrite(id,not(digitalRead(id)));
}

void setComp(long f){
  //A function that, based on compAvg and compMax, creates gate-out signals

  int j;
  int i;
  unsigned long tmpVal;
  unsigned long funval;
  compVals[1] = compMax;
  compVals[2] = compAvg;


  for(j=0;j<2;j++) {
    i = j*2;

    tmpVal = compVals[j];

    if(j == 1){
      funval = f*0.5;
    } else {
      funval = f;
    }

    if(funval >=tmpVal){
      compState[i] = 1;
      if(compState[i] != compState[i+1]){
        //Set the gate signal to high, if its not already high:
        //analogWrite(0,255);
        compClk[j] += 1;
        compState[i+1] = 1;
        if(not(maxIt % (maxIt/4))){
          digitalWrite((j+9), HIGH);
        }

      }
    } else {
      compState[i] = 0;
      //Was the previous state HIGH?
      if(compState[i] != compState[i+1]){
        //Set the gate signal to low, if its not already low:
        //analogWrite(0,0);
        compClk[j] -= 1;
        compState[i+1] = 0;
        if(not(maxIt % (maxIt/4))){
          digitalWrite((j+9), LOW);
        }
      }
    }
  }

  if(l == maxIt/4) {
    for(j=0;j<2;j++) {
      if(compClk[j] > 0){
        toggleLED(j+9);
      } else {
        toggleLED(j+9);
      }
      //Reinitialize the comparator values.
      compClk[2];
    }
  }
  //Serial.print(compClk[1]);
  //Serial.println(compClk[2]);
}

  //TODO : remove all led-defining variables and set digital states just as numbers.
void loop() {

  int funId;
  long fOut;
  int tMode;

  //digitalWrite(12, LOW);
  //  digitalWrite(10, LOW);
  if(l > maxIt){
    l = 0;
    t = 0;
  }

  t++;
  l++;
  //Serial.print("==");
  //  Serial.print(l);
  //Serial.print(":");

  //if (++i != maxIt) return;
  //i = 0;

  funId = funMode();
  fOut = setFunction(funId);
  setComp(fOut);
  //setClock();


  tMode = digitalRead(3);

  if(tMode == HIGH) {
    tModeFlag = 1;
  }

  if(tModeFlag == 1) {
    setTMode(fOut);
  } else {
    OCR2A = fOut;
  }
}
