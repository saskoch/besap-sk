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

// Flattened array sizes (literals)
#define FLAG_COUNT 4
#define INSTR_COUNT 16
#define STEP_COUNT 8

uint16_t UCODE_TEMPLATE[128];  // 16*8
uint16_t ucode[512];           // 4*16*8

// Build base template
void buildTemplate() {
    for(int i=0;i<128;i++) UCODE_TEMPLATE[i] = 0;

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

    // 1001–1101
    for(int i=9;i<=13;i++){
        UCODE_TEMPLATE[i*STEP_COUNT+0] = MI | CO;
        UCODE_TEMPLATE[i*STEP_COUNT+1] = RO | II | CE;
    }

    // 1110 OUT
    UCODE_TEMPLATE[14*STEP_COUNT+0] = MI | CO;
    UCODE_TEMPLATE[14*STEP_COUNT+1] = RO | II | CE;
    UCODE_TEMPLATE[14*STEP_COUNT+2] = AO | OI;

    // 1111 HLT
    UCODE_TEMPLATE[15*STEP_COUNT+0] = MI | CO;
    UCODE_TEMPLATE[15*STEP_COUNT+1] = RO | II | CE;
    UCODE_TEMPLATE[15*STEP_COUNT+2] = HLT;
}

// Initialize full microcode with flags
void initUCode() {
    for(int f=0;f<FLAG_COUNT;f++){
        for(int instr=0;instr<INSTR_COUNT;instr++){
            for(int step=0;step<STEP_COUNT;step++){
                ucode[f*INSTR_COUNT*STEP_COUNT + instr*STEP_COUNT + step] =
                    UCODE_TEMPLATE[instr*STEP_COUNT + step];
            }
        }
    }

    // Conditional jumps
    ucode[1*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J; // JC CF=1, ZF=0
    ucode[3*INSTR_COUNT*STEP_COUNT + 7*STEP_COUNT + 2] = IO | J; // JC CF=1, ZF=1
    ucode[2*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J; // JZ ZF=1, CF=0
    ucode[3*INSTR_COUNT*STEP_COUNT + 8*STEP_COUNT + 2] = IO | J; // JZ ZF=1, CF=1
}

// EEPROM functions
void AddressShiftOut(int dataPin,int clockPin,int bitOrder,int val,int bits){
    for(int i=0;i<bits;i++){
        if(bitOrder==LSBFIRST) digitalWrite(dataPin,!!(val&(1<<i)));
        else digitalWrite(dataPin,!!(val&(1<<(bits-1-i))));
        delay(1);
        digitalWrite(clockPin,HIGH); delay(1);
        digitalWrite(clockPin,LOW); delay(1);
    }
}

void SetDataOut(int val){
    for(int i=0;i<8;i++) digitalWrite(dataPins[i],bitRead(val,i));
    delay(1);
    digitalWrite(write,LOW); delay(1);
    digitalWrite(write,HIGH); delay(10);
}

void writeEEPROM(int address,int val){
    AddressShiftOut(SER,SRCLK,MSBFIRST,address,11);
    SetDataOut(val);
}

// Program microcode into EEPROM
void ProgramCPUEEPROM(){
    for(int address=0;address<1024;address++){
        int flags       = (address & 0x0300) >> 8;
        int byte_sel    = (address & 0x0080) >> 7;
        int instruction = (address & 0x0078) >> 3;
        int step        = (address & 0x0007);

        int idx = flags*INSTR_COUNT*STEP_COUNT + instruction*STEP_COUNT + step;

        if(byte_sel) writeEEPROM(address, ucode[idx]);
        else writeEEPROM(address, ucode[idx] >> 8);
    }
}

void setup(){
    buildTemplate();
    initUCode();

    pinMode(SER, OUTPUT); pinMode(RCLK, OUTPUT);
    pinMode(SRCLK, OUTPUT); pinMode(_SRCLR, OUTPUT); digitalWrite(_SRCLR,HIGH);
    pinMode(write, OUTPUT); digitalWrite(write,HIGH);
    for(int i=0;i<8;i++) pinMode(dataPins[i],OUTPUT);

    ProgramCPUEEPROM();
}

void loop(){}
