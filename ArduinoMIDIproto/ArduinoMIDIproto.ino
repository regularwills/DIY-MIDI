/*---------------------------------------------------------------------
  BY: Will May

  DIY MIDI Controller : For use of an Arduino with USB capabilities 
  to act as a MIDI controller for most audio/ music production programs

  REQUIREMENTS: Arduino Microcontroller board and compatabile USB connector
                Arduino IDE to upload code and "MIDIUSB" lib installed
                16 push buttons connected to the corresponding I/O pins below
                2 Potentiomenter knobs connected to corresponding ANALOG pins
                Any software that a regular MIDI controller can use
----------------------------------------------------------------------*/

#include "MIDIUSB.h"

const bool debug = false;

//Buttons and corresponding pins
const int TOTAL_BUTTONS = 16;                              
const int BUTTONS_PIN[TOTAL_BUTTONS] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,A4,A5};          
                                                            
// Starting pitches realted to each button pin
const int BUTTONS_PITCH[TOTAL_BUTTONS] = {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
                                                          
// Temporary input reads to check against current state.
int currentRead[TOTAL_BUTTONS];                            
int tempRead;


//Potentiometers and corresponding pins (slide or rotary)
const int TOTAL_POTS = 2;    
const int POT_PIN[TOTAL_POTS] = {A0,A1};                    //WARNING: Make sure these are ANALOG pins               

int potCState[TOTAL_POTS] = {0};                            // Current state of pot knob
int potPState[TOTAL_POTS] = {0};                            // Previous state of pot knob
int potStateDifference = 0;                                 // Difference between the current and previous states

int midiCState[TOTAL_POTS] = {};                            // Current state of midi value
int midiPState[TOTAL_POTS] = {0};                           // Previous state of midi value

const int TIMEOUT = 300;                                    // Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 10;                                // * Threshold for the potentiometer signal variation
boolean potMoving = true;                                   
unsigned long PTime[TOTAL_POTS] = {0};                      // Previously stored time
unsigned long timer[TOTAL_POTS] = {0};                      // Stores the time that has elapsed since the timer was reset


// Install firmware for a reset/new Arduino board
void setup() {
  // Initialize all the pins as a pull-up input.
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    pinMode(BUTTONS_PIN[i], INPUT_PULLUP);
  }
}

// Running firmware perpetually on Arduino board
void loop() {
  buttons();
  potentiometers();
}

//***** BUTTONS ************
void buttons(){
  //Ge the digital state from a button, store that state
   for (int i = 0; i < TOTAL_BUTTONS; i++) {           
     int buttonState = digitalRead(BUTTONS_PIN[i]);     // In pull-up inputs the button logic is inverted (HIGH is not pressed, LOW is pressed).
     tempRead = buttonState;                            
  
    // Continue if the last recorded state is different to current state
     if (currentRead[i] != tempRead) {                   
       // Delay for switch bounce  -----> See https://www.arduino.cc/en/pmwiki.php?n=Tutorial/Debounce
       delay(2);

       //Get the mapped pitch for this pin and save the new input state
       int pitch = BUTTONS_PITCH[i];                             
       currentRead[i] = tempRead;                        
      
       // Play or stop playing the note based on the digital read
       if (buttonState == LOW) {                        
         noteOn(pitch);                   // Button pressed                
       } 
       else {
         noteOff(pitch);                  // Button not pressed
       }
     }
   }
}

// Send a midi note
void noteOn(int pitch) {
  MidiUSB.sendMIDI({0x09, 0x90, pitch, 127});
  MidiUSB.flush();
}

// Terminate a midi note
void noteOff(int pitch) {
  MidiUSB.sendMIDI({0x08, 0x80, pitch, 0});
  MidiUSB.flush();
}

//********* POTENTIOMETERS ***********
void potentiometers() {
  // Read in the anaolg pins for pot knobs. 
  // Grab the abs value of the difference in pot states
  for (int i = 0; i < TOTAL_POTS; i++) {                  
    potCState[i] = analogRead(POT_PIN[i]);               

    // Maps the reading of the potCState to a value usable in midi
    midiCState[i] = map(potCState[i], 0, 1023, 0, 127);   
    potStateDifference = abs(potCState[i] - potPState[i]);

    // Opens gate if the pot variation is greater than the threshold, 
    // stores the previous time                              
    if (potStateDifference > varThreshold) {              
      PTime[i] = millis();                                
    }
    
    // Reset timer to 0ms
    timer[i] = millis() - PTime[i];                       

    // If the timer is less than max time, then the pot knob is still moving
    if (timer[i] < TIMEOUT) {                             
      potMoving = true;                                   
    }
    else {  
      potMoving = false;
    }

  // If the potentiometer is still moving, send the change control
    if (potMoving == true) {                              
      if (midiPState[i] != midiCState[i]) {

        controlChange(1, 1 + i, midiCState[i]);           // (MIDI channel to be used, lowest MIDI CC number,  CC value)
        MidiUSB.flush();
        
        // **** FOR DEBUGGING POTENTIOMETER ISSUES ****
        if (debug){
          Serial.print("Pot: ");                         
          Serial.print(i);                               
          Serial.print(" ");
          Serial.println(midiCState[i]);
        }

        // Stores the current reading of the potentiometer to compare with the next
        potPState[i] = potCState[i];                      
        midiPState[i] = midiCState[i];
      }
    }
  }
}

// Convert the pot state to a MIDI control change. (Should also be included with MIDIUSB.h) 
void controlChange(int channel, int control, int value){
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
