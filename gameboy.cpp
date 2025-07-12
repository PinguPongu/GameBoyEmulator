#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Flag Masks
#define FLAG_Z 0x80 // Zero Flag
#define FLAG_N 0x40 // Subtraction Flag
#define FLAG_H 0x20 // Half Carry flag
#define FLAG_C 0x10 // Carry Flag

#define LOW4 0x0F    // Lower 4 bits
#define LOW12 0x0FFF // Lower 12 bits

class CPU {
public:
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

    std::string filename = "Tetris_(USA)_(Rev-A).gb";
    std::vector<uint8_t> cart;

    // 0x00
    void NOP() {
        PC++;
    }

    // 0x01
    void LD_BC_d16() {
        C = get_byte();
        B = get_byte();
    }

    // 0x02 unfinished
    void LD_BC_A() {
        // oof
    }

    // 0x03
    void INC_BC() {
        uint16_t BC = get_register_pair(B, C);
        BC++; 
        store_register_pair(BC, B, C);
    }

    // 0x04 
    void INC_B() {
        set_flag_h_8_add(B, 1);
        B++;
        set_flag_n(0);
        set_flag_z(B);
    }

    // 0x05
    void DEC_B() {
        set_flag_h_8_sub(B, 1);
        B--;
        set_flag_n(1);
        set_flag_z(B);
    }

    // 0x06
    void LD_B_d8() {
        B = get_byte();
    }

    // 0x07
    void RLCA() {
        uint8_t bit7 = (A >> 7) & 0x01;
        A = ((A << 1) | bit7) & 0xFF;
        clear_flags();
        if (bit7)
            F |= FLAG_C;
    }

    // 0x08 unfinished
    void LD_a16_SP() {
        //oofff
    }

    // 0x09
    void ADD_HL_BC() {
        uint16_t BC = get_register_pair(B, C);
        uint16_t HL = get_register_pair(H, L);
        set_flag_h_16_add(HL, BC);
        set_flag_c_16_add(HL, BC);
        set_flag_n(0);
        HL += BC;
        store_register_pair(HL, H, L);
    }



    // ---------
    // UTILITIES 
    // ---------

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
    
    // H FLAG - USE BEFORE EDIT IS MADE

    void set_flag_h_8_add(uint8_t reg, uint8_t addition) {
        if (((reg & LOW4) + (addition & LOW4)) > LOW4)
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_h_8_sub(uint8_t reg, uint8_t addition) {
        if ((reg & LOW4) < (addition & LOW4))
            F |= FLAG_H;
        else
            F &= ~FLAG_H;
    }

    void set_flag_h_16_add(uint16_t reg, uint16_t subtraction) {
        if (((reg & LOW12) + (subtraction & LOW12)) > LOW12)
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

    uint16_t get_2_bytes(std::ifstream &file) {
        uint8_t low, high;
        low = cart[PC++];
        high = cart[PC++];

        return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
    }

    void load_game() {
        std::ifstream rom(filename, std::ios::binary);
        if (!rom.is_open())
        return;
        
        cart.reserve(32768);
        char byte;
        while (rom.read(&byte, 1)) {
           cart.push_back(static_cast<uint8_t>(byte));
        }
    }
};

int main() {

}
