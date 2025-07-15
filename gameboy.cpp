#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

// Flag Masks
#define FLAG_Z 0x80 // Zero Flag
#define FLAG_N 0x40 // Subtraction Flag
#define FLAG_H 0x20 // Half Carry flag
#define FLAG_C 0x10 // Carry Flag
#define LOW4 0x0F    // Lower 4 bits
#define LOW12 0x0FFF // Lower 12 bits

class Memory {
public:
    uint16_t address_space = 0xFFFF;
    uint8_t ROM_bank_00[0x4000];
    uint8_t ROM_bank_01_NN[0x4000];
    uint8_t VRAM[0x2000];
    uint8_t ERAM[0x2000];
    uint8_t WRAM_1[0x1000];
    uint8_t WRAM_2[0x1000];
    uint8_t echo_RAM[0x1E00];
    uint8_t OAM[0xA0];
    uint8_t not_usable[0x5F];
    uint8_t IO[0x80];
    uint8_t HRAM[0x7F];
    uint8_t interrupt;

    uint8_t &operator[] (uint16_t address) {
        if (address < 0x4000) {
            return ROM_bank_00[address];
        } else if (address < 0x8000) {
            return ROM_bank_01_NN[address - 0x4000];
        } else if (address < 0xA000) {
            return VRAM[address - 0x8000];
        } else if (address < 0xC000) {
            return ERAM[address - 0xA000];
        } else if (address < 0xD000) {
            return WRAM_1[address - 0xC000];            
        } else if (address < 0xE000) {
            return WRAM_2[address - 0xD000];
        } else if (address < 0xFE00) {
            return echo_RAM[address - 0xE000];
        } else if (address < 0xFEA0) {
            return OAM[address - 0xFE00];
        } else if (address < 0xFF00) {
            return not_usable[address - 0xFEA0];
        } else if (address < 0xFF80) {
            return IO[address - 0xFF00];
        } else if (address < 0xFFFF) {
            return HRAM[address - 0xFF80];
        } else if (address == 0xFFFF) {
            return interrupt;
        } 
        // return exception
        return interrupt;
    }
};

class CPU {
private:
    Memory *memory;

    uint8_t A = 0x01; // Accumulator
    uint8_t B = 0x00;
    uint8_t C = 0x13;
    uint8_t D = 0x00;
    uint8_t E = 0xD8;
    uint8_t H = 0x01;
    uint8_t L = 0x4D;
    uint8_t F = 0xB0; // Flags

    uint16_t SP = 0xFFFE; // Stack Pointer
    uint16_t PC = 0x0100; // Program Counter

    std::string filename;
    std::vector<uint8_t> cart;

    unsigned int cycles = 0;

    // NOTES
    // d8 -> 8-bit immediate value (unsigned)
    // s8 -> 8-bit immediate value (signed)
    // d16 -> 16-bit immediate value (unsigned little endian)
    // a16 -> 16-bit address

    // 0x00
    void NOP() {
        PC++;
        cycles++;
    }

    // 0x01
    void LD_BC_d16() {
        C = get_byte();
        B = get_byte();
        cycles += 3;
    }

    // 0x02
    void LD_BC_mem_A() {
        uint16_t BC = get_register_pair(B, C);       
        (*memory)[BC] = A;
        PC++;
        cycles += 2;
    }

    // 0x03
    void INC_BC() {
        INC_reg_pair(B, C);
        PC++;
        cycles += 2;
    }

    // 0x04 
    void INC_B() {
        INC_reg(B);
        PC++;
        cycles++;
    }

    // 0x05
    void DEC_B() {
        DEC_reg(B);
        PC++;
        cycles++;
    }

    // 0x06
    void LD_B_d8() {
        B = get_byte();
        cycles += 2;
    }

    // 0x07
    void RLCA() {
        uint8_t bit7 = (A >> 7) & 0x01;
        A = ((A << 1) | bit7) & 0xFF;
        clear_flags();
        if (bit7) set_flag_c(1);
        PC++;
        cycles++;
    }

    // 0x08
    void LD_a16_mem_SP() {
        uint8_t byte_lo = SP;
        uint8_t byte_hi = SP >> 8;
        uint16_t address = get_2_bytes();
        (*memory)[address] = byte_lo;
        (*memory)[address + 1] = byte_hi;
        PC++;
        cycles += 5;
    }

    // 0x09
    void ADD_HL_BC() {
        uint16_t HL = get_register_pair(H, L);
        uint16_t BC = get_register_pair(B, C);
        set_flag_h_16_add(HL, BC);
        set_flag_c_16_add(HL, BC);
        set_flag_n(0);
        HL += BC;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x0A
    void LD_A_BC_mem() {
        uint16_t BC = get_register_pair(B, C);
        A = (*memory)[BC];
        PC++;
        cycles += 2;
    }

    // 0x0B
    void DEC_BC() {
        DEC_reg_pair(B, C);
        PC++;
        cycles += 2;
    }

    // 0x0C
    void INC_C() {
        INC_reg(C);
        PC++;
        cycles++;
    }

    // 0x0D
    void DEC_C() {
        DEC_reg(C);
        PC++;
        cycles++;
    }

    // 0x0E
    void LD_C_d8() {
        C = get_byte();
        cycles += 2;
    }

    // 0x0F
    void RRCA() {
        uint8_t bit0 = A & 0x01;
        A = (A >> 1) | (bit0 << 7);
        clear_flags();
        if (bit0) set_flag_c(1);
        PC++;
        cycles++;
    }

    // 0x10 UNFINISHED
    void STOP() {
        // oof
        cycles++;
    }

    // 0x11
    void LD_DE_d16() {
        E = get_byte();
        D = get_byte();
        cycles += 3;
    }

    // 0x12
    void LD_DE_mem_A() {
        int16_t DE = get_register_pair(D, E);
        (*memory)[DE] = A;
        PC++;
        cycles += 2;
    }

    // 0x13
    void INC_DE() {
        INC_reg_pair(D, E);
        PC++;
        cycles += 2;
    }

    // 0x14
    void INC_D() {
        INC_reg(D);
        PC++;
        cycles++;
    }

    // 0x15
    void DEC_D() {
        DEC_reg(D);
        PC++;
        cycles++;
    }

    // 0x16
    void LD_D_d8() {
        D = get_byte();
        cycles += 2;
    }

    // 0x17
    void RLA() {
        uint8_t bit7 = A >> 7;
        uint8_t bit_c = (F & FLAG_C) >> 4;
        clear_flags();
        A = (A << 1) | bit_c;
        if (bit7) set_flag_c(1);
        PC++;
        cycles++;
    }

    // 0x18
    void JR_s8() {
        PC += static_cast<int8_t>(get_byte());
        cycles += 3;
    }

    // 0x19
    void ADD_HL_DE() {
        uint16_t HL = get_register_pair(H, L);
        uint16_t DE = get_register_pair(D, E);
        set_flag_h_16_add(HL, DE);
        set_flag_c_16_add(HL, DE);
        set_flag_n(0);
        HL += DE;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x1A
    void LD_A_DE_mem() {
        uint16_t DE = get_register_pair(D, E);
        A = (*memory)[DE];
        PC++;
        cycles += 2;
    }

    // 0x1B
    void DEC_DE() {
        DEC_reg_pair(D, E);
        PC++;
        cycles += 2;
    }

    // 0x1C
    void INC_E() {
        INC_reg(E);
        PC++;
        cycles++;
    }

    // 0x1D
    void DEC_E() {
        DEC_reg(E);
        PC++;
        cycles++;
    }

    // 0x1E
    void LD_E_d8() {
        E = get_byte();
        cycles += 2;
    }

    // 0x1F
    void RRA() {
        uint8_t bit0 = A & 0x01;
        A = (A >> 1) | ((F & FLAG_C) << 3);
        clear_flags();
        if (bit0) set_flag_c(1);
        PC++;
        cycles++;
    }

    // 0x20
    void JR_NZ_s8() {
        int8_t offset = static_cast<int8_t>(get_byte());
        if (F & FLAG_Z) {
            cycles += 2;
        } else {
            PC += offset;
            cycles += 3;
        }
    }

    // 0x21
    void LD_HL_d16() {
        L = get_byte();
        H = get_byte();
        cycles += 3;
    }

    // 0x22
    void LD_HL_mem_plus_A() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL++] = A;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x23
    void INC_HL() {
        INC_reg_pair(H, L);
        PC++;
        cycles += 2;
    }

    // 0x24
    void INC_H() {
        INC_reg(H);
        PC++;
        cycles++;
    }

    // 0x25
    void DEC_H() {
        DEC_reg(H);
        PC++;
        cycles++;
    }

    // 0x26
    void LD_H_d8() {
        H = get_byte();
        cycles += 2;
    }

    // 0x27
    void DAA() {
        bool setC = false;

        if (!(F & FLAG_N)) {
            if ((A > 0x99) || (F & FLAG_C)) {
                A += 0x60;
                setC = true;
            }
            if ((A & 0x0F) > 0x09 || (F & FLAG_H)) {
                A += 0x06;
            }
        } else {
            if (F & FLAG_C) A -= 0x60;
            if (F & FLAG_H) A -= 0x06;
        }

        set_flag_z(A);
        set_flag_h(0);
        if (setC)               set_flag_c(1);
        else if (!(F & FLAG_N)) set_flag_c(0);
        PC++;

        cycles++;
    }

    // 0x28
    void JR_Z_s8() {
        int8_t offset = static_cast<int8_t>(get_byte());
        if (F & FLAG_Z) {
            PC += offset;
            cycles += 3;
        } else {
            cycles += 2;
        }
    }

    // 0x29
    void ADD_HL_HL() {
        uint16_t HL = get_register_pair(H, L);
        set_flag_c_16_add(HL, HL);
        set_flag_h_16_add(HL, HL);
        set_flag_n(0);
        HL += HL;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x2A
    void LD_A_HL_mem_plus() {
        uint16_t HL = get_register_pair(H, L);
        A = (*memory)[HL];
        HL++;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x2B
    void DEC_HL() {
        DEC_reg_pair(H, L);
        PC++;
        cycles += 2;
    }

    // 0x2C
    void INC_L() {
        INC_reg(L);
        PC++;
        cycles++;
    }

    // 0x2D
    void DEC_L() {
        DEC_reg(L);
        PC++;
        cycles++;
    }

    // 0x2E
    void LD_L_d8() {
        L = get_byte();
        PC++;
        cycles += 2;
    }

    // 0x2F
    void CPL() {
        A = ~A;
        set_flag_n(1);
        set_flag_h(1);
        PC++;
        cycles++;
    }

    // 0x30
    void JR_NC_s8() {
        int8_t offset = static_cast<int8_t>(get_byte());
        if (F & FLAG_C) {
            PC += offset;
            cycles += 3;
        }
        else {
            cycles += 2;
        } 
    }

    // 0x31
    void LD_SP_d16() {
        SP = get_2_bytes();
        cycles += 3;
    }

    // 0x32
    void LD_HL_mem_minus_A() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL--] = A;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x33
    void INC_SP() {
        SP++;
        PC++;
        cycles += 2;
    }

    // 0x34
    void INC_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        set_flag_h_8_add((*memory)[HL], 1);        
        (*memory)[HL]++;
        set_flag_z((*memory)[HL]);
        set_flag_n(0);
        PC++;
        cycles += 3;
    }

    // 0x35
    void DEC_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        set_flag_h_8_add((*memory)[HL], 1);        
        (*memory)[HL]--;
        set_flag_z((*memory)[HL]);
        set_flag_n(0);
        PC++;
        cycles += 3;
    }

    // 0x36
    void LD_HL_mem_d8() {
        uint16_t HL = get_register_pair(H, L);
        uint8_t data = get_byte();
        (*memory)[HL] = data;
        cycles += 3;
    }

    // 0x37
    void SCF() {
        set_flag_n(0);
        set_flag_h(0);
        set_flag_c(1);
        PC++;
        cycles++;
    }

    // 0x38
    void JR_C_s8() {
        int8_t offset = static_cast<int8_t>(get_byte());
        if (F & FLAG_C) {
            PC += offset;
            cycles += 3;
        } else {
            cycles += 2;
        }
    }

    // 0x39
    void ADD_HL_SP() {
        uint16_t HL = get_register_pair(H, L);
        set_flag_c_16_add(HL, SP);
        set_flag_h_16_add(HL, SP);
        set_flag_n(0);
        HL += SP;
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x3A
    void LD_A_HL_mem_minus() {
        uint16_t HL = get_register_pair(H, L);
        A = (*memory)[HL--];
        store_register_pair(HL, H, L);
        PC++;
        cycles += 2;
    }

    // 0x3B
    void DEC_SP() {
        SP--;
        PC++;
        cycles += 2;
    }

    // 0x3C
    void INC_A() {
        INC_reg(A);
        PC++;
        cycles++;
    }

    // 0x3D
    void DEC_A() {
        DEC_reg(A);
        PC++;
        cycles++;
    }

    // 0x3E
    void LD_A_d8() {
        A = get_byte();
        cycles += 2;
    }

    // 0x3F
    void CCF() {
        set_flag_n(0);
        set_flag_h(0);
        set_flag_c(!(F & FLAG_C));
        PC++;
        cycles++;
    }

    // 0x40
    void LD_B_B() {
        B = B;
        PC++;
        cycles++;
    }

    // 0x41
    void LD_B_C() {
        B = C;
        PC++;
        cycles++;
    }

    // 0x42
    void LD_B_D() {
        B = D;
        PC++;
        cycles++;
    }

    // 0x43
    void LD_B_E() {
        B = E;
        PC++;
        cycles++;
    }

    // 0x44
    void LD_B_H() {
        B = H;
        PC++;
        cycles++;
    }

    // 0x45
    void LD_B_L() {
        B = L;
        PC++;
        cycles++;
    }

    // 0x46
    void LD_B_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        B = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x47
    void LD_B_A() {
        B = A;
        PC++;
        cycles++;
    }

    // 0x48
    void LD_C_B() {
        C = B;
        PC++;
        cycles++;
    }

    // 0x49
    void LD_C_C() {
        C = C;
        PC++;
        cycles++;
    }

    // 0x4A
    void LD_C_D() {
        C = D;
        PC++;
        cycles++;
    }

    // 0x4B
    void LD_C_E() {
        C = E;
        PC++;
        cycles++;
    }

    // 0x4C
    void LD_C_H() {
        C = H;
        PC++;
        cycles++;
    }

    // 0x4D
    void LD_C_L() {
        C = L;
        PC++;
        cycles++;
    }

    // 0x4E
    void LD_C_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        C = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x4F
    void LD_C_A() {
        C = A;
        PC++;
        cycles++;
    }

    // 0x50
    void LD_D_B() {
        D = B;
        PC++;
        cycles++;
    }

    // 0x51
    void LD_D_C() {
        D = C;
        PC++;
        cycles++;
    }

    // 0x52
    void LD_D_D() {
        D = D;
        PC++;
        cycles++;
    }

    // 0x53
    void LD_D_E() {
        D = E;
        PC++;
        cycles++;
    }

    // 0x54
    void LD_D_H() {
        D = H;
        PC++;
        cycles++;
    }

    // 0x55
    void LD_D_L() {
        D = L;
        PC++;
        cycles++;
    }

    // 0x56
    void LD_D_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        D = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x57
    void LD_D_A() {
        D = A;
        PC++;
        cycles++;
    }

    // 0x58
    void LD_E_B() {
        E = B;
        PC++;
        cycles++;
    }

    // 0x59
    void LD_E_C() {
        E = C;
        PC++;
        cycles++;
    }

    // 0x5A
    void LD_E_D() {
        E = D;
        PC++;
        cycles++;
    }

    // 0x5B
    void LD_E_E() {
        E = E;
        PC++;
        cycles++;
    }

    // 0x5C
    void LD_E_H() {
        E = H;
        PC++;
        cycles++;
    }

    // 0x5D
    void LD_E_L() {
        E = L;
        PC++;
        cycles++;
    }

    // 0x5E
    void LD_E_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        E = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x5F
    void LD_E_A() {
        E = A;
        PC++;
        cycles++;
    }

    // 0x60
    void LD_H_B() {
        H = B;
        PC++;
        cycles++;
    }

    // 0x61
    void LD_H_C() {
        H = C;
        PC++;
        cycles++;
    }

    // 0x62
    void LD_H_D() {
        H = D;
        PC++;
        cycles++;
    }

    // 0x63
    void LD_H_E() {
        H = E;
        PC++;
        cycles++;
    }

    // 0x64
    void LD_H_H() {
        H = H;
        PC++;
        cycles++;
    }

    // 0x65
    void LD_H_L() {
        H = L;
        PC++;
        cycles++;
    }

    // 0x66
    void LD_H_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        H = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x67
    void LD_H_A() {
        H = A;
        PC++;
        cycles++;
    }

    // 0x68
    void LD_L_B() {
        L = B;
        PC++;
        cycles++;
    }

    // 0x69
    void LD_L_C() {
        L = C;
        PC++;
        cycles++;
    }

    // 0x6A
    void LD_L_D() {
        L = D;
        PC++;
        cycles++;
    }

    // 0x6B
    void LD_L_E() {
        L = E;
        PC++;
        cycles++;
    }

    // 0x6C
    void LD_L_H() {
        L = H;
        PC++;
        cycles++;
    }

    // 0x6D
    void LD_L_L() {
        L = L;
        PC++;
        cycles++;
    }

    // 0x6E
    void LD_L_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        L = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x6F
    void LD_L_A() {
        L = A;
        PC++;
        cycles++;
    }

    // 0x70
    void LD_HL_mem_B() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = B;
        PC++;
        cycles += 2;
    }

    // 0x71
    void LD_HL_mem_C() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = C;
        PC++;
        cycles += 2;
    }

    // 0x72
    void LD_HL_mem_D() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = D;
        PC++;
        cycles += 2;
    }

    // 0x73
    void LD_HL_mem_E() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = E;
        PC++;
        cycles += 2;
    }

    // 0x74
    void LD_HL_mem_H() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = H;
        PC++;
        cycles += 2;
    }

    // 0x75
    void LD_HL_mem_L() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = L;
        PC++;
        cycles += 2;
    }

    // 0x76 UNFINISHED
    void HALT() {
        cycles++;
    }

    // 0x77
    void LD_HL_mem_A() {
        uint16_t HL = get_register_pair(H, L);
        (*memory)[HL] = A;
        PC++;
        cycles += 2;
    }

    // 0x78
    void LD_A_B() {
        A = B;
        PC++;
        cycles++;
    }

    // 0x79
    void LD_A_C() {
        A = C;
        PC++;
        cycles++;
    }

    // 0x7A
    void LD_A_D() {
        A = D;
        PC++;
        cycles++;
    }

    // 0x7B
    void LD_A_E() {
        A = E;
        PC++;
        cycles++;
    }

    // 0x7C
    void LD_A_H() {
        A = H;
        PC++;
        cycles++;
    }

    // 0x7D
    void LD_A_L() {
        A = L;
        PC++;
        cycles++;
    }

    // 0x7E
    void LD_A_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        A = (*memory)[HL];
        PC++;
        cycles += 2;
    }

    // 0x7F
    void LD_A_A() {
        A = A;
        PC++;
        cycles++;
    }

    // 0x80
    void ADD_A_B() {
        ADD(A, B);
        PC++;
        cycles++;
    }

    // 0x81
    void ADD_A_C() {
        ADD(A, C);
        PC++;
        cycles++;
    }

    // 0x82
    void ADD_A_D() {
        ADD(A, D);
        PC++;
        cycles++;
    }

    // 0x83
    void ADD_A_E() {
        ADD(A, E);
        PC++;
        cycles++;
    }

    // 0x84
    void ADD_A_H() {
        ADD(A, H);
        PC++;
        cycles++;
    }

    // 0x85
    void ADD_A_L() {
        ADD(A, L);
        PC++;
        cycles++;
    }

    // 0x86
    void ADD_A_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        ADD(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0x87
    void ADD_A_A() {
        ADD(A, A);
        PC++;
        cycles++;
    }

    // 0x88
    void ADC_A_B() {
        ADC(A, B);
        PC++;
        cycles++;
    }

    // 0x89
    void ADC_A_C() {
        ADC(A, C);
        PC++;
        cycles++;
    }

    // 0x8A
    void ADC_A_D() {
        ADC(A, D);
        PC++;
        cycles++;
    }

    // 0x8B
    void ADC_A_E() {
        ADC(A, E);
        PC++;
        cycles++;
    }

    // 0x8C
    void ADC_A_H() {
        ADC(A, H);
        PC++;
        cycles++;
    }

    // 0x8D
    void ADC_A_L() {
        ADC(A, L);
        PC++;
        cycles++;
    }

    // 0x8E
    void ADC_A_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        ADC(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0x8F
    void ADC_A_A() {
        ADC(A, A);
        PC++;
        cycles++;
    }

    // 0x90
    void SUB_B() {
        SUB(A, B);
        PC++;
        cycles++;
    }

    // 0x91
    void SUB_C() {
        SUB(A, C);
        PC++;
        cycles++;
    }

    // 0x92
    void SUB_D() {
        SUB(A, D);
        PC++;
        cycles++;
    }

    // 0x93
    void SUB_E() {
        SUB(A, E);
        PC++;
        cycles++;
    }

    // 0x94
    void SUB_H() {
        SUB(A, H);
        PC++;
        cycles++;
    }

    // 0x95
    void SUB_L() {
        SUB(A, L);
        PC++;
        cycles++;
    }

    // 0x96
    void SUB_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        SUB(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0x97
    void SUB_A() {
        SUB(A, A);
        PC++;
        cycles++;
    }

    // 0x98
    void SBC_A_B() {
        SBC(A, B);
        PC++;
        cycles++;
    }

    // 0x99
    void SBC_A_C() {
        SBC(A, C);
        PC++;
        cycles++;
    }

    // 0x9A
    void SBC_A_D() {
        SBC(A, D);
        PC++;
        cycles++;
    }

    // 0x9B
    void SBC_A_E() {
        SBC(A, E);
        PC++;
        cycles++;
    }

    // 0x9C
    void SBC_A_H() {
        SBC(A, H);
        PC++;
        cycles++;
    }

    // 0x9D
    void SBC_A_L() {
        SBC(A, L);
        PC++;
        cycles++;
    }

    // 0x9E
    void SBC_A_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        SBC(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0x9F
    void SBC_A_A() {
        SBC(A, A);
        PC++;
        cycles++;
    }

    // 0xA0
    void AND_B() {
        AND(A, B);
        PC++;
        cycles++;
    }

    // 0xA1
    void AND_C() {
        AND(A, C);
        PC++;
        cycles++;
    }

    // 0xA2
    void AND_D() {
        AND(A, D);
        PC++;
        cycles++;
    }

    // 0xA3
    void AND_E() {
        AND(A, E);
        PC++;
        cycles++;
    }

    // 0xA4
    void AND_H() {
        AND(A, H);
        PC++;
        cycles++;
    }

    // 0xA5
    void AND_L() {
        AND(A, L);
        PC++;
        cycles++;
    }

    // 0xA6
    void AND_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        AND(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0xA7
    void AND_A() {
        AND(A, A);
        PC++;
        cycles++;
    }

    // 0xA8
    void XOR_B() {
        XOR(A, B);
        PC++;
        cycles++;
    }

    // 0xA9
    void XOR_C() {
        XOR(A, C);
        PC++;
        cycles++;
    }

    // 0xAA
    void XOR_D() {
        XOR(A, D);
        PC++;
        cycles++;
    }

    // 0xAB
    void XOR_E() {
        XOR(A, E);
        PC++;
        cycles++;
    }

    // 0xAC
    void XOR_H() {
        XOR(A, H);
        PC++;
        cycles++;
    }

    // 0xAD
    void XOR_L() {
        XOR(A, L);
        PC++;
        cycles++;
    }

    // 0xAE
    void XOR_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        XOR(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0xAF
    void XOR_A() {
        XOR(A, A);
        PC++;
        cycles++;
    }

    // 0xB0
    void OR_B() {
        OR(A, B);
        PC++;
        cycles++;
    }

    // 0xB1
    void OR_C() {
        OR(A, C);
        PC++;
        cycles++;
    }

    // 0xB2
    void OR_D() {
        OR(A, D);
        PC++;
        cycles++;
    }

    // 0xB3
    void OR_E() {
        OR(A, E);
        PC++;
        cycles++;
    }

    // 0xB4
    void OR_H() {
        OR(A, H);
        PC++;
        cycles++;
    }

    // 0xB5
    void OR_L() {
        OR(A, L);
        PC++;
        cycles++;
    }

    // 0xB6
    void AND_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        OR(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0xB7
    void OR_A() {
        OR(A, A);
        PC++;
        cycles++;
    }

    // 0xB8
    void CP_B() {
        CP(A, B);
        PC++;
        cycles++;
    }

    // 0xB9
    void CP_C() {
        CP(A, C);
        PC++;
        cycles++;
    }

    // 0xBA
    void CP_D() {
        CP(A, D);
        PC++;
        cycles++;
    }

    // 0xBB
    void CP_E() {
        CP(A, E);
        PC++;
        cycles++;
    }

    // 0xBC
    void CP_H() {
        CP(A, H);
        PC++;
        cycles++;
    }

    // 0xBD
    void CP_L() {
        CP(A, L);
        PC++;
        cycles++;
    }

    // 0xBE
    void CP_HL_mem() {
        uint16_t HL = get_register_pair(H, L);
        CP(A, (*memory)[HL]);
        PC++;
        cycles += 2;
    }

    // 0xBF
    void CP_A() {
        CP(A, A);
        PC++;
        cycles++;
    }

    // 0xC0
    void RET_NZ() {
        if (!(F & FLAG_Z)) {
            uint8_t lo_byte = (*memory)[SP++];
            uint8_t hi_byte = (*memory)[SP++];
            PC = get_register_pair(hi_byte, lo_byte);
            cycles += 5;
        } else {
            cycles += 2;
        }
    }

    // 0xC1

    void select_op(uint8_t byte) {
        switch(byte) {
            case 0x00: NOP();           break;
            case 0x01: LD_BC_d16();     break;
            case 0x02: LD_BC_mem_A();   break;
            case 0x03: INC_BC();        break;
            case 0x04: INC_B();         break;
            case 0x05: DEC_B();         break;
            case 0x06: LD_B_d8();       break;
            case 0x07: RLCA();          break;
            case 0x08: LD_a16_mem_SP(); break;
            case 0x09: ADD_HL_BC();     break;
            case 0x0A: LD_A_BC_mem();   break;
            case 0x0B: DEC_BC();        break;
            case 0x0C: INC_C();         break;
            case 0x0D: DEC_C();         break;
            case 0x0E: LD_C_d8();       break;
            case 0x0F: RRCA();          break;
            case 0x10: STOP();          break; // UNFINISHED
            case 0x11: LD_DE_d16();     break;
            case 0x12: LD_DE_mem_A();   break;
            case 0x13: INC_DE();        break;
            case 0x14: INC_D();         break;
            case 0x15: DEC_D();         break;
            case 0x16: LD_D_d8();       break;
            case 0x17: RLA();           break;
            case 0x18: JR_s8();         break;
            case 0x19: ADD_HL_DE();     break;
            case 0x1A: LD_A_DE_mem();   break;
            case 0x1B: DEC_DE();        break;
            case 0x1C: INC_E();         break;
            case 0x1D: DEC_E();         break;
            case 0x1E: LD_E_d8();       break;
            case 0x1F: RRA();           break;
            case 0x20: JR_NZ_s8();      break;
            case 0x21: LD_HL_d16();     break;
            case 0x22: LD_HL_mem_plus_A();  break;
            case 0x23: INC_HL();        break;
            case 0x24: INC_H();         break;
            case 0x25: DEC_H();         break;
            case 0x26: LD_H_d8();       break;
            case 0x27: DAA();           break;
        }
    }

    // ---------
    // UTILITIES 
    // ---------

    // -------------------
    // OPERATION TEMPLATES
    // -------------------

    void INC_reg(uint8_t &reg) {
        set_flag_h_8_add(reg, 1);
        reg++;
        set_flag_n(0);
        set_flag_z(reg);
    }

    void DEC_reg(uint8_t &reg) {
        set_flag_h_8_sub(reg, 1);
        reg--;
        set_flag_n(1);
        set_flag_z(reg);
    }

    void INC_reg_pair(uint8_t &reg_hi, uint8_t &reg_lo) {
        uint16_t reg_pair = get_register_pair(reg_hi, reg_lo);
        reg_pair++;
        store_register_pair(reg_pair, reg_hi, reg_lo);
    }

    void DEC_reg_pair(uint8_t &reg_hi, uint8_t &reg_lo) {
        uint16_t reg_pair = get_register_pair(reg_hi, reg_lo);
        reg_pair--;
        store_register_pair(reg_pair, reg_hi, reg_lo);
    }

    void ADD(uint8_t &reg, uint8_t val) {
        uint8_t result = reg + val;
        set_flag_z(result);
        set_flag_n(0);
        set_flag_h_8_add(reg, val);
        set_flag_c_8_add(reg, val);
        reg = result;
    }

    void ADC(uint8_t &reg, uint8_t val) {
        if (F & FLAG_C) ADD(reg, val + 1);
        else            ADD(reg, val);
    }

    void SUB(uint8_t &reg, uint8_t val) {
        uint8_t result = reg - val;
        set_flag_z(result);
        set_flag_n(1);
        set_flag_h_8_sub(reg, val);
        set_flag_c_8_sub(reg, val);
        reg = result;
    }

    void SBC(uint8_t &reg, uint8_t val) {
        if (F & FLAG_C) SUB(reg, val + 1);
        else            SUB(reg, val);
    }

    void AND(uint8_t &reg_1, uint8_t reg_2) {
        reg_1 &= reg_2;
        set_flag_z(reg_1);
        set_flag_n(0);
        set_flag_h(1);
        set_flag_c(0);
    }

    void XOR(uint8_t &reg_1, uint8_t reg_2) {
        reg_1 ^= reg_2;
        set_flag_z(reg_1);
        set_flag_n(0);
        set_flag_h(0);
        set_flag_c(0);
    }

    void OR(uint8_t &reg_1, uint8_t reg_2) {
        reg_1 |= reg_2;
        set_flag_z(reg_1);
        set_flag_n(0);
        set_flag_h(0);
        set_flag_c(0);
    }

    void CP(uint8_t reg_1, uint8_t reg_2) {
        uint8_t result = reg_1 - reg_2;
        set_flag_z(result);
        set_flag_n(1);
        set_flag_h_8_sub(reg_1, reg_2);
        set_flag_c_8_sub(reg_1, reg_2);
    }

    // -------------
    // F FLAGS UTILS
    // -------------

    void clear_flags() {
        F = 0;
    }

    void set_flag_z(uint8_t reg) {
        if (reg == 0) 
            F |= FLAG_Z; 
        else
            F &= ~FLAG_Z;
    }
    
    void set_flag_n(char bit) {
        if (bit)
            F |= FLAG_N;
        else
            F &= ~FLAG_N;
    }

    void set_flag_h(char bit) {
        if (bit)
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_c(char bit) {
        if (bit)
            F |= FLAG_C;
        else
            F &= ~FLAG_C;
    }
    
    // H FLAG - USE BEFORE EDIT IS MADE

    void set_flag_h_8_add(uint8_t reg, uint8_t addition) {
        if (((reg & LOW4) + (addition & LOW4)) > LOW4)
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_h_8_sub(uint8_t reg, uint8_t subtraction) {
        if ((reg & LOW4) < (subtraction & LOW4))
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_h_16_add(uint16_t reg, uint16_t addition) {
        if (((reg & LOW12) + (addition & LOW12)) > LOW12)
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_h_16_sub(uint16_t reg, uint16_t subtraction) {
        if ((reg & LOW12) < (subtraction & LOW12))
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    // C FLAG - USE BEFORE EDIT IS MADE

    void set_flag_c_8_add(uint8_t reg, uint8_t addition) {
        if ((reg > (reg + addition)))
            F |= FLAG_C;
        else
            F &= ~FLAG_C;
    }

    void set_flag_c_8_sub(uint8_t reg, uint8_t subtraction) {
        if ((reg < (reg - subtraction)))
            F |= FLAG_C;
        else
            F &= ~FLAG_C;
    }

    void set_flag_c_16_add(uint16_t reg, uint16_t addition) {
        if ((reg > (reg + addition)))
            F |= FLAG_C;
        else
            F &= ~FLAG_C;
    }

    void set_flag_c_16_sub(uint16_t reg, uint16_t subtraction) {
        if ((reg < (reg - subtraction)))
            F |= FLAG_C;
        else
            F &= ~FLAG_C;
    }

    // ------------------------------------
    // GETTING AND STORING 16 BIT REGISTERS
    // ------------------------------------

    uint16_t get_register_pair(uint8_t reg_a, uint8_t reg_b) {
        return (reg_a << 8) | reg_b;
    }

    void store_register_pair(uint16_t reg_ab, uint8_t &reg_a, uint8_t &reg_b) {
        reg_b = reg_ab;
        reg_a = reg_ab >> 8;
    }

    uint8_t get_byte() {
        return cart[PC++];
    }

    uint16_t get_2_bytes() {
        uint8_t byte_lo, byte_hi;
        byte_lo = cart[PC++];
        byte_hi = cart[PC++];

        return static_cast<uint16_t>(byte_lo) | (static_cast<uint16_t>(byte_hi) << 8);
    }

    void load_game() {
        std::ifstream rom(filename, std::ios::binary);
        if (!rom.is_open())
            // return custom exception    
            return;
        
        cart.reserve(32768);
        char byte;
        while (rom.read(&byte, 1)) {
           cart.push_back(static_cast<uint8_t>(byte));
        }
    }

    // -----
    // DEBUG
    // -----

    void print_registers() {
        uint16_t AF = get_register_pair(A, F);
        uint16_t BC = get_register_pair(B, C);
        uint16_t DE = get_register_pair(D, E);
        uint16_t HL = get_register_pair(H, L);

        std::cout << std::hex << std::uppercase;
        std::cout << "╔══════════════════\n";
        std::cout << "║ Curr.Byte: 0x" << std::setw(2) << std::setfill('0') << +cart[PC] << "\n";
        std::cout << "║ Cycles: " << cycles << "\n";
        std::cout << "╚═════════════════. ..\n";
        std::cout << "╔════════╦════════╗\n";
        std::cout << "║ Reg    ║ Value  ║\n";
        std::cout << "╠════════╬════════╣\n";
        std::cout << "║ A      ║ 0x" << std::setw(2) << std::setfill('0') << +A << "   ║\n";
        std::cout << "║ F      ║ 0x" << std::setw(2) << std::setfill('0') << +F << "   ║\n";
        std::cout << "║ B      ║ 0x" << std::setw(2) << std::setfill('0') << +B << "   ║\n";
        std::cout << "║ C      ║ 0x" << std::setw(2) << std::setfill('0') << +C << "   ║\n";
        std::cout << "║ D      ║ 0x" << std::setw(2) << std::setfill('0') << +D << "   ║\n";
        std::cout << "║ E      ║ 0x" << std::setw(2) << std::setfill('0') << +E << "   ║\n";
        std::cout << "║ H      ║ 0x" << std::setw(2) << std::setfill('0') << +H << "   ║\n";
        std::cout << "║ L      ║ 0x" << std::setw(2) << std::setfill('0') << +L << "   ║\n";
        std::cout << "╠════════╬════════╣\n";
        std::cout << "║ AF     ║ 0x" << std::setw(4) << std::setfill('0') << AF << " ║\n";
        std::cout << "║ BC     ║ 0x" << std::setw(4) << std::setfill('0') << BC << " ║\n";
        std::cout << "║ DE     ║ 0x" << std::setw(4) << std::setfill('0') << DE << " ║\n";
        std::cout << "║ HL     ║ 0x" << std::setw(4) << std::setfill('0') << HL << " ║\n";
        std::cout << "║ SP     ║ 0x" << std::setw(4) << std::setfill('0') << SP << " ║\n";
        std::cout << "║ PC     ║ 0x" << std::setw(4) << std::setfill('0') << PC << " ║\n";
        std::cout << "╚════════╩════════╝\n";
    }

    void reset_cycles() {
        cycles = 0;
    }

public:
    CPU(Memory *_memory) : memory(_memory) {}

    void play_game() {
        filename = "Tetris_(USA)_(Rev-A).gb";
        load_game();

        while(1) {
            print_registers();
            uint8_t opcode = cart[PC];
            select_op(opcode);
        }
    }
};


int main() {
    Memory mem;
    CPU cpu(&mem);
    cpu.play_game();
}
