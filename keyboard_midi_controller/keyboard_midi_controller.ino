// Using arduino as a midi controller
#include <MIDIUSB.h>

// Pins based on Arduino Pro Micro
#define CONTROL_MUX(n)  (14 + (n))
#define READ_KEY(n)     (2 + (n))
#define PITCH_SHIFT_MAX 4
#define CHANNEL_MAX     4

// LAST_BLOCK_CONTROL:
// Decide whether to use the last block (5 keys) as control for the midi
// interface or not (normal keys)
// Setting this value to 'true' will reduce the range of the keyboard, but allows for pitch shifting
// and channel selection:
//
// ##########################
// ##########################
// . .   |  |#| |#|  |   |###
// . .   |  |#| |#|  |   |###
// . .   |  |#| |#|  |   |###
// . .   |  | | | |  |   |###
// . .   |  |A| |C|  |   |###
// . .   |   |   |   |   |###
// . .___|___|_B_|_D_|_E_|###
// ##########################
// ##########################
//
// A: * Shift keys 1 octave lower
// B: X
// C: * Shift keys 1 octave higher
// D: ** Move to previous MIDI channel
// E: ** Move to next MIDI channel
//
// *:  Defaults to C3 at beginning of the keyboard, cycles when reaching upper/lower,
//     upper can be set using PITCH_SHIFT_MAX, lower = 0
//
// **: Defaults to 0, cycles when reaching upper/lower
//     upper can be set using CHANNEL_MAX, lower = 0

#define LAST_BLOCK_CONTROL  true

// Handle key status
char curr[8];
char prev[8];
// Handle keyboard control status
#if LAST_BLOCK_CONTROL
  char channel = 0;
  char pitch_shift = 3;
#else
  #define channel     0
  #define pitch_shift 0
#endif

void setup() {
  
  // Control multiplexer
  pinMode(CONTROL_MUX(0), OUTPUT);
  pinMode(CONTROL_MUX(1), OUTPUT);
  pinMode(CONTROL_MUX(2), OUTPUT);

  // Read key status
  pinMode(READ_KEY(0), INPUT_PULLUP);
  pinMode(READ_KEY(1), INPUT_PULLUP);
  pinMode(READ_KEY(2), INPUT_PULLUP);
  pinMode(READ_KEY(3), INPUT_PULLUP);
  pinMode(READ_KEY(4), INPUT_PULLUP);
  pinMode(READ_KEY(5), INPUT_PULLUP);
  pinMode(READ_KEY(6), INPUT_PULLUP);
  pinMode(READ_KEY(7), INPUT_PULLUP);

  // Initialize keys as not pressed
  for (uint8_t i=0; i<8; i++) {
    curr[i] = 0b00000000;
    prev[i] = 0b00000000;
  }
}

// Enable keyboar block for reading (mapping is done by wires)
void setMuxAddr(char number) {
  digitalWrite(CONTROL_MUX(0), number & 0b00000001);
  digitalWrite(CONTROL_MUX(1), number & 0b00000010);
  digitalWrite(CONTROL_MUX(2), number & 0b00000100);
}

  // Read keys status, in reverse order so they appear as "highest note = msb"
char scanLine() {
  uint8_t value = 0;
  for(uint8_t i = 7; i<8; i--) {
    value = (value << 1) | digitalRead(READ_KEY(i));
  }
  // Since is using pullup logic, negate values to have the "Pressed == HIGH" logic
  return ~value;
}

// Check the status of a single key in the register
#define TEST_KEY(x)         ((x) & __mask)

// Compute the actual pitch from pitch_shift, block and key index
#define MIDI_C0         ((uint8_t)12)
#define MIDI_PITCH(block, index) ((uint8_t)MIDI_C0 + pitch_shift * 12\
    + (uint8_t)((block) << 3)\
    + (uint8_t) (index))

// Checks single notes in the block and send MIDI packets based on their state
void checkNoteBlock(uint8_t s) {
  uint8_t c = curr[s];
  uint8_t p = prev[s];
  for (uint8_t i = 0; i<8; i++) {
    uint8_t __mask = (0b00000001 << i);
    if (TEST_KEY(c) && !TEST_KEY(p)) {
      MidiUSB.sendMIDI((midiEventPacket_t){0x09, 0x90 | channel, MIDI_PITCH(s,i), 64}); // noteon
    } else if (TEST_KEY(p) && !TEST_KEY(c)) {
      MidiUSB.sendMIDI((midiEventPacket_t){0x08, 0x80 | channel, MIDI_PITCH(s,i), 64}); // noteoff
    }
  }
}

// Chek if the key with specified index is pressed in the active block
#define KEY_PRESSED(n) ((__pressed) & (0b00000001 << (n)))

#if LAST_BLOCK_CONTROL
// Handle the keypressing on last block
void handleControlBlock() {
  uint8_t __pressed = curr[7] & (curr[7] ^ prev[7]); // flipped state and now pressed
  for (uint8_t i = 0; i < 5; i++) {
    if(KEY_PRESSED(0)) {
      pitch_shift = (pitch_shift + (PITCH_SHIFT_MAX - 1)) % PITCH_SHIFT_MAX;
    }else if (KEY_PRESSED(2)) {
      pitch_shift = (pitch_shift + 1) % PITCH_SHIFT_MAX;
    }
    if(KEY_PRESSED(3)) {
      channel = (channel + (CHANNELS_MAX -1)) % CHANNEL_MAX;
    }else if (KEY_PRESSED(4)) {
      channel = (channel + 1) % CHANNEL_MAX;
    }
  }
}
#endif

void loop() {
  // Handle last blok separately
#if LAST_BLOCK_CONTROL
  setMuxAddr(7);
  prev[7] = curr[7];
  curr[7] = scanLine();
  if(curr[7] ^ prev[7]) handleControlBlock();

  for(uint8_t i = 0; i<7; i++) {
#else
  for(uint8_t i = 0; i<8; i++) {
#endif
    setMuxAddr(i);
    prev[i] = curr[i];
    curr[i] = scanLine();
    if (curr[i] ^ prev[i]) {  // block 7 (8th) is used for special commands (change channel and shift pitch)
      checkNoteBlock(i);
    }
  }
  MidiUSB.flush();  // Ensures to send MIDI packets if available
  delay(1);
}
