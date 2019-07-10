//MOZZI SYNTH by Tomás Hurrell, Juan Manuel Baez, Nicolas Fernandez

#include <MozziGuts.h>
#include <Oscil.h>
#include <AutoMap.h>
#include <tables/cos2048_int8.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <LowPassFilter.h>


// PARAMETERS
// Rate (Hz) of calling updateControl(), powers of 2 please.
#define CONTROL_RATE 64

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> sinewave(SIN2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> squarewave(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SAW2048_NUM_CELLS, AUDIO_RATE> sawwave(SAW2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> trianglewave(TRIANGLE2048_DATA);

//Waveforms Array + freq range
const Oscil<COS2048_NUM_CELLS, AUDIO_RATE> WAVEFORMS[4] = {sinewave, squarewave, sawwave, trianglewave};
const int MIN_FREQ = 18;
const int MAX_FREQ = 5000;

//WAVEFORM SELECTOR
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> selectedWaveform = sinewave;
int selectedWaveformIndex = 0;

//Octaves
int octaves[9][7] = {
  {16,18,21,23,25,28,31},//octave 0
  {33,37,41,44,49,55,62},//octave 1
  {65,73,82,87,98,110,123}, //octave 2
  {131,145,165,175,196,220,247}, //octave 3
  {262,294,330,349,392,440,494}, //octave 4
  {523,587,659,698,784,880,988}, //octave 5
  {1046,1175,1319,1397,1568,1760,1976}, //octave 6
  {2093,2349,2637,2794,3136,3520,3951}, //octave 7
  {4186,4699,5274,5588,6272,7040,7902}, //octave 8
};

//LOW PASS FILTER + LFO
Oscil <COS2048_NUM_CELLS, AUDIO_RATE> cutoffLFO(COS2048_DATA);
LowPassFilter lpf = LowPassFilter();

//Mapping values from pots to the desired oscilator frequencies.
AutoMap kMapFreq(0,1023, MIN_FREQ, MAX_FREQ);
AutoMap kMapOctave(0,1023, 0, 8);
AutoMap kMapWaveform(0,1023, 0, 3);
AutoMap kMapLPFCutoff(0,1023, 0, 180);
AutoMap kMapLPFCutoffLFO(0,1023,0, 3000);

//KEYS - Digital input
int cKey = 8;
int dKey = 7;
int eKey = 6;
int fKey = 5;
int gKey = 4;
int aKey = 3;
int bKey = 2;

//---------------------------------------------------------
// METHODS
//---------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(cKey, INPUT);
  pinMode(dKey, INPUT);
  pinMode(eKey, INPUT);
  pinMode(fKey, INPUT);
  pinMode(gKey, INPUT);
  pinMode(aKey, INPUT);
  pinMode(bKey, INPUT);
  startMozzi(CONTROL_RATE);
}

void loop() {
  audioHook();
}

// MOZZI METHODS

void updateControl(){
  //Pots read
  int freqKnobValue = mozziAnalogRead(0);
  int waveformKnobValue = mozziAnalogRead(1);
  int lpfCutoffKnobValue = mozziAnalogRead(3);
  int lfoKnobValue = mozziAnalogRead(5); 

  //Value Mapping
  int carrierFreq = kMapFreq(freqKnobValue);
  int octave = kMapOctave(freqKnobValue);
  int waveform = kMapWaveform(waveformKnobValue);
  int lpfCutoff = kMapLPFCutoff(lpfCutoffKnobValue);
  int lfo = kMapLPFCutoffLFO(lfoKnobValue);

  //Keyboard or frequency knob mode
  bool synthMode = digitalRead(13);
  Serial.print(synthMode);
  //IF KEYBOARD MODE
  if (synthMode == HIGH) {
    carrierFreq = 0; //stops freq and waits for key values
    
    //Digital reading + Value setting
    int cKeyState = digitalRead(cKey);
    int dKeyState = digitalRead(dKey);
    int eKeyState = digitalRead(eKey);
    int fKeyState = digitalRead(fKey);
    int gKeyState = digitalRead(gKey);
    int aKeyState = digitalRead(aKey);
    int bKeyState = digitalRead(bKey);

    int selectedOctave = octave;
    
    if (cKeyState == HIGH) carrierFreq = octaves[octave][0];
    if (dKeyState == HIGH) carrierFreq = octaves[octave][1];
    if (eKeyState == HIGH) carrierFreq = octaves[octave][2];
    if (fKeyState == HIGH) carrierFreq = octaves[octave][3];
    if (gKeyState == HIGH) carrierFreq = octaves[octave][4];
    if (aKeyState == HIGH) carrierFreq = octaves[octave][5];
    if (bKeyState == HIGH) carrierFreq = octaves[octave][6];

    //Printing key values
//    keySerialPrint(cKeyState, dKeyState, eKeyState, fKeyState, gKeyState, aKeyState, bKeyState, octave);
  }
  
  //Sound Settings
  if (waveform != selectedWaveformIndex) {
    selectedWaveform = WAVEFORMS[waveform];
    selectedWaveformIndex = waveform;  
  }

  cutoffLFO.setFreq(lfo);
  int lfoVol;
  lfoVol = 128u+cutoffLFO.next();//Set Lfo to 0 to 255 instead of -127 to 127
  lpf.setCutoffFreq((lpfCutoff*lfoVol)>>8);
  selectedWaveform.setFreq(carrierFreq);
}

void serialPrint(int carrierFreq, int waveform, int lpfCutoff) {
  
  Serial.print("FREQ = "); 
  Serial.print(carrierFreq);
  Serial.print("\t"); // prints a tab
  
  Serial.print("WAVEFORM = "); 
  Serial.print(waveform);
  Serial.print("\t"); // prints a tab

  Serial.print("LPF CUTOFF = "); 
  Serial.print(lpfCutoff);
  Serial.print("\t"); // prints a tab
  
  Serial.print("\n"); // prints a new line 
  
}

void keySerialPrint(int cKeyState, int dKeyState, int eKeyState, int fKeyState, int gKeyState, int aKeyState, int bKeyState, int octave) {
        
    Serial.print("OCTAVE "); 
    Serial.print(octave);
    Serial.print("\t"); // prints a tab
    Serial.print("\t"); // prints a tab
    Serial.print("C"); 
    Serial.print(cKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("D"); 
    Serial.print(dKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("E"); 
    Serial.print(eKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("F"); 
    Serial.print(fKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("G"); 
    Serial.print(gKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("A"); 
    Serial.print(aKeyState);
    Serial.print("\t"); // prints a tab
    Serial.print("B"); 
    Serial.print(bKeyState);
    Serial.print("\t"); // prints a tab 
    
    Serial.print("\n"); // prints a new line 
}

int updateAudio(){
  return lpf.next(selectedWaveform.next());
}
