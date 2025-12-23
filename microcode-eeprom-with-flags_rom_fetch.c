#include <Arduino.h>

// Pins
int SER = 0;
int RCLK = 4;
int SRCLK = 3;
int _SRCLR = 2;
int dataPins[] = {5, 6, 7, 8, 9, 10, 11, 12};
int write = 13;

// Control Signals
const uint16_t HLT = 0x8000;
const uint16_t MI  = 0x4000;
const uint16_t RI  = 0x2000;
const uint16_t RO  = 0x1000;
const uint16_t IO  = 0x0800;
const uint16_t II  = 0x0400;
const uint16_t AI  = 0x0200;
const uint16_t AO  = 0x0100;
const uint16_t SO  = 0x0080;
const uint16_t SU  = 0x0040;
const uint16_t BI  = 0x0020;
const uint16_t OI  = 0x0010;
const uint16_t CE  = 0x0008;
const uint16_t CO  = 0x0004;
const uint16_t J   = 0x0002;
const uint16_t FI  = 0x0001;

#define FLAG_COUNT 4
#define INSTR_COUNT 16
#define STEP_COUNT 8

uint16_t UCODE_TEMPLATE[128];
uint16_t UCODE_ROMFETCH_TEMPLATE[128];
uint16_t ucode[512];
uint16_t ucode_romfetch[512];

// ------------------------------------------------------------
// Build normal (RAM-based) microcode template
// ------------------------------------------------------------
void buildTemplate() {
  for (int i = 0; i < 128; i++) UCODE_TEMPLATE[i] = 0;

  // 0000 NOP
  UCODE_TEMPLATE[0*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[0*STEP_COUNT+1] = RO | II | CE;

  // 0001 LDA
  UCODE_TEMPLATE[1*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[1*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[1*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[1*STEP_COUNT+3] = RO | AI;

  // 0010 ADD
  UCODE_TEMPLATE[2*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[2*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[2*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[2*STEP_COUNT+3] = RO | BI;
  UCODE_TEMPLATE[2*STEP_COUNT+4] = SO | AI | FI;

  // 0011 SUB
  UCODE_TEMPLATE[3*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[3*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[3*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[3*STEP_COUNT+3] = RO | BI;
  UCODE_TEMPLATE[3*STEP_COUNT+4] = SO | AI | SU | FI;

  // 0100 STA
  UCODE_TEMPLATE[4*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[4*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[4*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[4*STEP_COUNT+3] = AO | RI;

  // 0101 LDI
  UCODE_TEMPLATE[5*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[5*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[5*STEP_COUNT+2] = IO | AI;

  // 0110 JMP
  UCODE_TEMPLATE[6*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[6*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[6*STEP_COUNT+2] = IO | J;

  // 0111 JC
  UCODE_TEMPLATE[7*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[7*STEP_COUNT+1] = RO | II | CE;

  // 1000 JZ
  UCODE_TEMPLATE[8*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[8*STEP_COUNT+1] = RO | II | CE;

  // 1001–1101 unused instructions
  for (int i = 9; i <= 10; i++) {
    UCODE_TEMPLATE[i*STEP_COUNT+0] = MI | CO;
    UCODE_TEMPLATE[i*STEP_COUNT+1] = RO | II | CE;
  }

  // 1011 CMP
  UCODE_TEMPLATE[11*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[11*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[11*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[11*STEP_COUNT+3] = RO | BI;
  UCODE_TEMPLATE[11*STEP_COUNT+4] = SU | FI;

  // 1100 CMR
  UCODE_TEMPLATE[12*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[12*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[12*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[12*STEP_COUNT+3] = BI;
  UCODE_TEMPLATE[12*STEP_COUNT+4] = SU | FI;

  // 1101 IN
  UCODE_TEMPLATE[13*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[13*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[13*STEP_COUNT+2] = IO | MI;
  UCODE_TEMPLATE[13*STEP_COUNT+3] = RI | AI;

  // 1110 OUT
  UCODE_TEMPLATE[14*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[14*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[14*STEP_COUNT+2] = AO | OI;

  // 1111 HLT
  UCODE_TEMPLATE[15*STEP_COUNT+0] = MI | CO;
  UCODE_TEMPLATE[15*STEP_COUNT+1] = RO | II | CE;
  UCODE_TEMPLATE[15*STEP_COUNT+2] = HLT;
}

// ------------------------------------------------------------
// Build ROM-fetch variant template
// ------------------------------------------------------------
void buildRomFetchTemplate() {
  for (int instr = 0; instr < INSTR_COUNT; instr++) {
    // Copy original
    for (int step = 0; step < STEP_COUNT; step++) {
      UCODE_ROMFETCH_TEMPLATE[instr * STEP_COUNT + step] =
        UCODE_TEMPLATE[instr * STEP_COUNT + step];
    }

    // Replace the first two fetch microsteps:
    UCODE_ROMFETCH_TEMPLATE[instr * STEP_COUNT + 1] = II | CE;  // increment PC and instruction in (ROM drives bus)
  }

// ------------------------------------------------------------
// Set up LDR (opcode 0x9) microcode explicitly
// ------------------------------------------------------------
int LDR_OPCODE = 9;

UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+0] = MI | CO;      // PC → MAR
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+1] = II | CE;      // Fetch opcode (ROM drives bus, increment PC)
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+2] = IO | MI;      // Operand → MAR
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+3] = AI;      // ROM[MAR] → A (no RO)
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+4] = 0;            // padding        
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+5] = 0;
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+6] = 0;
UCODE_ROMFETCH_TEMPLATE[LDR_OPCODE*STEP_COUNT+7] = 0;

// ------------------------------------------------------------
// Set up JF (opcode 0xA) microcode explicitly for far jumps
// ------------------------------------------------------------
int JF_OPCODE = 10;

UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+0] = MI | CO;      // PC → MAR
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+1] = II | CE;      // Fetch opcode (ROM drives bus, increment PC)
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+2] = IO | MI;      
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+3] = J;      
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+4] = 0;           
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+5] = 0;
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+6] = 0;
UCODE_ROMFETCH_TEMPLATE[JF_OPCODE*STEP_COUNT+7] = 0;
}

// ------------------------------------------------------------
// Initialize full microcode arrays (RAM mode + ROM mode)
// ------------------------------------------------------------
void initUCode() {
  for (int f = 0; f < FLAG_COUNT; f++) {
    for (int instr = 0; instr < INSTR_COUNT; instr++) {
      for (int step = 0; step < STEP_COUNT; step++) {
        ucode[f*INSTR_COUNT*STEP_COUNT + instr*STEP_COUNT + step] =
          UCODE_TEMPLATE[instr*STEP_COUNT + step];
        ucode_romfetch[f*INSTR_COUNT*STEP_COUNT + instr*STEP_COUNT + step] =
          UCODE_ROMFETCH_TEMPLATE[instr*STEP_COUNT + step];
      }
    }
  }

  // Conditional jumps (JC, JZ)
  ucode[1*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J; // JC CF=1, ZF=0
  ucode[3*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J; // JC CF=1, ZF=1
  ucode[2*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J; // JZ ZF=1, CF=0
  ucode[3*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J; // JZ ZF=1, CF=1

  ucode_romfetch[1*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J;
  ucode_romfetch[3*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J;
  ucode_romfetch[2*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J;
  ucode_romfetch[3*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J;
}

// ------------------------------------------------------------
// EEPROM Writing Functions
// ------------------------------------------------------------
void AddressShiftOut(int dataPin,int clockPin,int bitOrder,int val,int bits){
  for (int i = 0; i < bits; i++) {
    if (bitOrder == LSBFIRST)
      digitalWrite(dataPin, !!(val & (1 << i)));
    else
      digitalWrite(dataPin, !!(val & (1 << (bits - 1 - i))));
    delayMicroseconds(1);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(clockPin, LOW);
    delayMicroseconds(1);
  }
}

void SetDataOut(int val){
  for (int i = 0; i < 8; i++) digitalWrite(dataPins[i], bitRead(val, i));
  delayMicroseconds(1);
  digitalWrite(write, LOW);
  delayMicroseconds(1);
  digitalWrite(write, HIGH);
  delay(5);
}

void writeEEPROM(int address, int val){
  AddressShiftOut(SER, SRCLK, MSBFIRST, address, 11);
  SetDataOut(val);
}

// ------------------------------------------------------------
// Program EEPROMs (first normal, then ROM-fetch variant)
// ------------------------------------------------------------
void ProgramCPUEEPROM(){
  // First half (normal)
  for (int address = 0; address < 1024; address++) {
    int flags       = (address & 0x0300) >> 8;
    int byte_sel    = (address & 0x0080) >> 7;
    int instruction = (address & 0x0078) >> 3;
    int step        = (address & 0x0007);

    int idx = flags*INSTR_COUNT*STEP_COUNT + instruction*STEP_COUNT + step;
    int value = byte_sel ? ucode[idx] : (ucode[idx] >> 8);
    writeEEPROM(address, value);
  }

  // Second half (ROM-fetch variant)
  for (int address = 0; address < 1024; address++) {
    int flags       = (address & 0x0300) >> 8;
    int byte_sel    = (address & 0x0080) >> 7;
    int instruction = (address & 0x0078) >> 3;
    int step        = (address & 0x0007);

    int idx = flags*INSTR_COUNT*STEP_COUNT + instruction*STEP_COUNT + step;
    int value = byte_sel ? ucode_romfetch[idx] : (ucode_romfetch[idx] >> 8);
    writeEEPROM(address + 1024, value); // upper half
  }
}

// ------------------------------------------------------------
void setup() {
  buildTemplate();
  buildRomFetchTemplate();
  initUCode();

  pinMode(SER, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(_SRCLR, OUTPUT);
  digitalWrite(_SRCLR, HIGH);
  pinMode(write, OUTPUT);
  digitalWrite(write, HIGH);
  for (int i = 0; i < 8; i++) pinMode(dataPins[i], OUTPUT);

  ProgramCPUEEPROM();
}

void loop(){}
