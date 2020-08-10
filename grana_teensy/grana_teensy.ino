// grana
// v0.0.1
// august 2020

// this is a project started by aaron montoya-moraga
// research assistant at mit media lab
// opera of the future + future sketches

// this version is a draft and remixes code
// found on the Teensy + Arduino examples

// this version runs on teensy 4.0
// with the teensy audio shield

// current functionalities include:
// 0. mic input
// 1. sd card reading and writing
// 2. record button
// 3. play button
// 4. knob for recording in different filename
// 5. knob for playback of different filename

// import libraries
#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// measured in milliseconds
int bounceTime = 8;

// bounce for buttons
Bounce buttonRecord = Bounce(0, bounceTime);
Bounce buttonPlay = Bounce(2, bounceTime);

int mode = 0;
// 0 is stopped
// 1 is recording
// 2 is playing

// variables for Teensy Audio Shield on Teensy 4.0
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

AudioInputI2S         i2s2;
AudioRecordQueue      queue1;
AudioPlaySdRaw        playRaw1;
AudioOutputI2S        i2s1;
AudioConnection       patchCord1(i2s2, 0, queue1, 0);
AudioConnection       patchCord2(playRaw1, 0, i2s1, 0);
AudioConnection       patchCord3(playRaw1, 0, i2s1, 1);
AudioControlSGTL5000  sgtl5000_1;
// GUItool: end automatically generated code

// four filenames, their names in binary notation
String fileNames[] = {"00.RAW", "01.RAW", "10.RAW", "11.RAW"};

// file to record data to
File bufferFile;

// potentiometer values
int knobFileRecordRaw;
int knobFilePlayRaw;

// select files to play and record
int fileRecordIndex;
int filePlayIndex;

String fileRecordString;
String filePlayString;

char fileRecord[6];
char filePlay[6];

void setup() {

  // setup buttons
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  // reserve memory for audios
  AudioMemory(60);

  // turn on audio
  sgtl5000_1.enable();

  // select electret mic as input
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);

  // set output volume
  sgtl5000_1.volume(0.5);

  // configure spi
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  // start SD card
  SD.begin(SDCARD_CS_PIN);

  // start serial communication
  Serial.begin(9600);
}

void loop() {

  // read the potentiometers and map their values
  knobFileRecordRaw = analogRead(A3);
  knobFilePlayRaw = analogRead(A2);

  // map their values to the corresponding index
  fileRecordIndex = map(knobFileRecordRaw, 0, 1023, 0, 3);
  filePlayIndex = map(knobFilePlayRaw, 0, 1023, 0, 3);

  // retrieve strings
  fileRecordString = String(fileNames[fileRecordIndex]);
  filePlayString = String(fileNames[filePlayIndex]);

  // convert to arrays of char
  fileRecordString.toCharArray(fileRecord, 7);
  filePlayString.toCharArray(filePlay, 7);

  // read the buttons
  buttonRecord.update();
  buttonPlay.update();

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("button rec down");
    if (mode == 2) {
      stopPlaying();
    }
    if (mode == 0) {
      startRecording();
    }
  }

  if (buttonRecord.risingEdge()) {
    Serial.println("button rec up");
    if (mode == 1) {
      stopRecording();
    }
  }
  
  if (buttonPlay.fallingEdge()) {
    Serial.println("button play down");
    if (mode == 1) {
      stopRecording();
    }
    if (mode == 0) {
      startPlaying();
    }
  }

  if (buttonPlay.risingEdge()) {
    Serial.println("button play up");
    if (mode == 2) {
      stopPlaying();
    }
  }

  // If we're playing or recording, carry on
  if (mode == 1) {
    continueRecording();
  }
  if (mode == 2) {
    continuePlaying();
  }

}

void startRecording() {
  Serial.println("startRecording");



  if (SD.exists(fileRecord)) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove(fileRecord);
  }
  bufferFile = SD.open(fileRecord, FILE_WRITE);
  if (bufferFile) {
    queue1.begin();
    mode = 1;
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    //elapsedMicros usec = 0;
    bufferFile.write(buffer, 512);
  }
}

void stopRecording() {
  Serial.println("stopRecording");
  queue1.end();
  if (mode == 1) {
    while (queue1.available() > 0) {
      bufferFile.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    bufferFile.close();
  }
  mode = 0;
}

void startPlaying() {

  Serial.println("startPlaying");

  if (SD.exists(filePlay)) {
    playRaw1.play(filePlay);
    mode = 2;
  }
}

void continuePlaying() {
  if (!playRaw1.isPlaying()) {
    playRaw1.stop();
    mode = 0;
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == 2) playRaw1.stop();
  mode = 0;
}
