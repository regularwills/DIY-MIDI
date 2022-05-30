#include "MIDIUSB.h"

/*Set up for an arduino Leonardo as a midi controller corresponding with the MIDIUSB arduino library*/

//Global variables related to buttons and corresponding pins
const int TOTAL_BUTTONS = 16;                               // All the Arduino pins used for buttons, in order.
const int BUTTONS_PIN[TOTAL_BUTTONS] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,A4,A5};          
                                                            // ^Every pitch corresponding to every Arduino pin. Each note has an associated numeric pitch (frequency scale).

const int BUTTONS_PITCH[TOTAL_BUTTONS] = {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
                                                           // ^Current state of the pressed buttons.
int currentRead[TOTAL_BUTTONS];                            // Temporary input reads to check against current state.
int tempRead;


//Global variables related to potentiometers and corresponding pins
const int TOTAL_POTS = 2;                                   // All potentiometers (slide or rotary)
const int POT_PIN[TOTAL_POTS] = {A0,A1};                    // All potentiometers corresponding to every Arduino pin

int potCState[TOTAL_POTS] = {0};                            // Current state of pot knob
int potPState[TOTAL_POTS] = {0};                            // Previous state of pot knob
int potStateDifference = 0;                                 // Difference between the current and previous state of the pot

int midiCState[TOTAL_POTS] = {};                            // Current state of midi value
int midiPState[TOTAL_POTS] = {0};                            // Previous stae of midi value

const int TIMEOUT = 300;                                     // Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 10;                                 // * Threshold for the potentiometer signal variation
boolean potMoving = true;                                   
unsigned long PTime[TOTAL_POTS] = {0};                       // Previously stored time
unsigned long timer[TOTAL_POTS] = {0};                       // Stores the time that has elapsed since the timer was reset


// The setup function runs once when you press reset or power the board
void setup() {
  // Initialize all the pins as a pull-up input.
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    pinMode(BUTTONS_PIN[i], INPUT_PULLUP);
  }
}

void loop() {
  buttons();
  potentiometers();
}


//*********************** BUTTONS *****************************************************
void buttons(){
   for (int i = 0; i < TOTAL_BUTTONS; i++) {            // Get the digital state from the button pin.
     int buttonState = digitalRead(BUTTONS_PIN[i]);     // In pull-up inputs the button logic is inverted (HIGH is not pressed, LOW is pressed).
     tempRead = buttonState;                             // Temporarily store the digital state.
  
     if (currentRead[i] != tempRead) {                   // Continue only if the last state is different to the current state.
       // See https://www.arduino.cc/en/pmwiki.php?n=Tutorial/Debounce
       delay(2);
       int pitch = BUTTONS_PITCH[i];                    // Get the pitch mapped to the pressed button.             
       currentRead[i] = tempRead;                        // Save the new input state.
      
       if (buttonState == LOW) {                         // Execute note on or noted off depending on the button state.
         noteOn(pitch);                                  // Low equals button is being/has been pressed
       } 
       else {
         noteOff(pitch);
       }
     }
   }
}

void noteOn(int pitch) {
  MidiUSB.sendMIDI({0x09, 0x90, pitch, 127});
  MidiUSB.flush();
}

void noteOff(int pitch) {
  MidiUSB.sendMIDI({0x08, 0x80, pitch, 0});
  MidiUSB.flush();
}

//********************** POTENTIOMETERS ***********************************************
void potentiometers() {
  for (int i = 0; i < TOTAL_POTS; i++) {                  // Loops through all the potentiometers
    potCState[i] = analogRead(POT_PIN[i]);                // Read in pins from the arduino board
    midiCState[i] = map(potCState[i], 0, 1023, 0, 127);   // Maps the reading of the potCState to a value usable in midi
    potStateDifference = abs(potCState[i] - potPState[i]);// Calculates the absolute value between the difference 
                                                          // between the current and previous state of the pot knob

    if (potStateDifference > varThreshold) {              // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis();                                // Stores the previous time
    }

    timer[i] = millis() - PTime[i];                       // Resets the timer to 0ms

    if (timer[i] < TIMEOUT) {                             // If the timer is less than the maximum allowed time
      potMoving = true;                                   // then the potentiometer is still moving
    }
    else {  
      potMoving = false;
    }

    if (potMoving == true) {                              // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        controlChange(1, 1 + i, midiCState[i]);           // (MIDI channel to be used, lowest MIDI CC number,  CC value)
        MidiUSB.flush();
        Serial.print("Pot: ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(midiCState[i]);
        potPState[i] = potCState[i];                      // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }
}

void controlChange(int channel, int control, int value){
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
