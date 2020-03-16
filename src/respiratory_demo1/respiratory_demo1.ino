/*Copyright (c) 2020, Dynamic Motion SA (Switzerland)
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of the University of California, Berkeley nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Principes
 *  
 *  The software is part of an open source respiratory assisting system, also called CPAP / PEEP / BiPAP
 *  An hardware containing a pump, a pressure detector (columb of water) and LEDs
 *  
 *  The Arduino (UNO or Nano) controls an ESC (RC brushless controller) for the pump motor.
 *  The brushless pumps air with a pressure of few cm H2O, and a debit of few dozen of liters per minute.
 *  The pressure measurement is made with a column of water and simple level detectors (conduction between imerged wires).
 *  Pressure setting is made with the position of imerged wires (future)
 *  
 *  Future evolution ideas are:
 *  Adding a valve on exhaust
 *  Adding a simple way for setting time
 * 
 */

#include <EEPROM.h>
#include <Servo.h>
Servo myservo;  // create servo object to control a servo

// Hardware definitione
#define LED_CYCLE_PIN 13
#define RED_LED_PIN 2
#define BLUE_LED_PIN 3
#define MOTOR_ESC_PIN 9
#define PIN_RED_WATER_DETECTION A1
#define PIN_BLUE_WATER_DETECTION A0

// Globals
uint32_t mem_millis;
#define TICK_TIME 25 // every 25s, the process is checked
#define INSPIRATE_TIME_SEC  4 // time in seconds, duration of halph breath cycle
#define EXPIRATE_TIME_SEC  4

uint16_t cycleProgressTime; // time in "tick"
uint16_t cycleStep; // right now, only 2 steps: inspirate and expirate
byte waterLevel; // 2 bits logic, the level detectors 
uint16_t lev1,lev2; // reading on analog pins detecting flow level
byte motorTargetValue;
#define ACCEL_VAL 3

void delayClignote(int32_t ms){ // delay with blinking LED (used during init of ESC motor controller)
  while (ms>0){
    digitalWrite(LED_CYCLE_PIN,1);
    ms-=30;
    delay(30);
    digitalWrite(LED_CYCLE_PIN,0);
    ms-=30;
    delay(30);
  }
}

void manange_motor(void){ // smooth motor speed change
  static uint8_t currentMotorOutVal;
  if (currentMotorOutVal<motorTargetValue){
      currentMotorOutVal+=ACCEL_VAL;
      if (currentMotorOutVal>motorTargetValue) currentMotorOutVal=motorTargetValue;
  }else   if (currentMotorOutVal>motorTargetValue){
      if (currentMotorOutVal<=ACCEL_VAL)currentMotorOutVal=0 ;
      else   currentMotorOutVal-=ACCEL_VAL;
      if (currentMotorOutVal<motorTargetValue) currentMotorOutVal=motorTargetValue;
  }
  myservo.write( currentMotorOutVal );
}

// Standard functions
void setup() {
  Serial.begin(57600);
  pinMode(LED_CYCLE_PIN, OUTPUT);
  digitalWrite(PIN_BLUE_WATER_DETECTION,1); // activate pull-up
  digitalWrite(PIN_RED_WATER_DETECTION,1); // activate pull-up
  pinMode(BLUE_LED_PIN,OUTPUT);
  pinMode(RED_LED_PIN,OUTPUT);
  Serial.println("Respirator");
  myservo.attach(MOTOR_ESC_PIN);  // attaches the servo on pin 9 to the servo object

  myservo.write( 44 ); // ESC start sequence: throttle at 0 position (44/255) during long time
  delayClignote(6000); // wait 6 seconds
  myservo.write( 240 ); // throttle at full speed (240/255)
  delayClignote(1000);
  myservo.write( 44 );// throttle back to zero (44/255)
  delayClignote(1000);
}

void loop() {
  if ( (millis()-mem_millis)>=TICK_TIME){
    mem_millis+=TICK_TIME;
    manange_motor();
    cycleProgressTime++; 
    if (cycleProgressTime >= (INSPIRATE_TIME_SEC * (1000/TICK_TIME) ) )cycleStep=1;
    else cycleStep=0;
    if (cycleProgressTime >= ((INSPIRATE_TIME_SEC +EXPIRATE_TIME_SEC) * (1000/TICK_TIME) ) )cycleProgressTime=0;
  }

  lev1=analogRead(PIN_BLUE_WATER_DETECTION);
  lev2=analogRead(PIN_RED_WATER_DETECTION);
  waterLevel=0;
  if (lev1<950){ // 
    digitalWrite(RED_LED_PIN,1);
    waterLevel|=0x1;
  }  else {
    digitalWrite(RED_LED_PIN,0);
  }
  if (lev2<950){
    digitalWrite(BLUE_LED_PIN,0);
    waterLevel|=0x02;
  }  else {
    digitalWrite(BLUE_LED_PIN,1);
  }
  
  switch(cycleStep){
    default:cycleStep=0; // in case of cycle error
    case 0:
      digitalWrite(LED_CYCLE_PIN,1);
      motorTargetValue=50;
    break;
    case 1: 
      digitalWrite(LED_CYCLE_PIN,0);
      motorTargetValue=160;
    break;
  }
}
