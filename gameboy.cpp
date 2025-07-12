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

class CPU {
public:

    void play_game() {
        filename = "Tetris_(USA)_(Rev-A).gb";
        load_game();
        while(1) {
            uint8_t byte = cart[PC++];
            select_op(byte);
            print_registers();
        }
    }

private:
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
        INC_reg_pair(B, C);
    }

    // 0x04 
    void INC_B() {
        INC_reg(B);
    }

    // 0x05
    void DEC_B() {
        DEC_reg(B);
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

    // 0x0A // unfinished
    void LD_A_BC() {

    }

    // 0x0B
    void DEC_BC() {
        DEC_reg_pair(B, C);
    }

    // 0x0C
    void INC_C() {
        INC_reg(C);
    }

    // 0x0D
    void DEC_C() {
        DEC_reg(C);
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
        uint16_t reg_comb = get_register_pair(reg_hi, reg_lo);
        reg_comb++;
        store_register_pair(reg_comb, reg_hi, reg_lo);
    }

    void DEC_reg_pair(uint8_t &reg_hi, uint8_t &reg_lo) {
        uint16_t reg_comb = get_register_pair(reg_hi, reg_lo);
        reg_comb--;
        store_register_pair(reg_comb, reg_hi, reg_lo);
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


    void select_op(uint8_t byte) {
        switch(byte) {
            case 0x00: NOP();       break;
            case 0x01: LD_BC_d16(); break;
            case 0x02: LD_BC_A();   break;
            case 0x03: INC_BC();    break;
            case 0x04: INC_B();     break;
            case 0x05: DEC_B();     break;
            case 0x06: LD_B_d8();   break;
            case 0x07: RLCA();      break;
            case 0x08: LD_a16_SP(); break;
            case 0x09: ADD_HL_BC(); break;
            case 0x0A: LD_A_BC();   break;
            case 0x0B: DEC_BC();    break;
            case 0x0C: INC_C();     break;
            case 0x0D: DEC_C();     break;
        }
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


    void print_registers() {
        uint16_t AF = get_register_pair(A, F);
        uint16_t BC = get_register_pair(B, C);
        uint16_t DE = get_register_pair(D, E);
        uint16_t HL = get_register_pair(H, L);

        std::cout << std::hex << std::uppercase; // Hex formatting, uppercase A-F
        std::cout << "╔═════════════════╗\n";
        std::cout << "║ Curr.Byte: 0x" << std::setw(2) << std::setfill('0') << +cart[PC] << " ║\n";
        std::cout << "╚═════════════════╝\n";
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
};

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

int main() {
    CPU cpu;
    // cpu.play_game();
    Memory mem;

    mem[0x02] = 5;

    std::cout << static_cast<int>(mem[0x02]) << '\n';

}
