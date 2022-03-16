#ifndef LAB3_RV32IM_HPP
#define LAB3_RV32IM_HPP

#include <iostream>

std::string get_register(uint8_t reg) {
    static std::string regs[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
    return regs[reg];
}

uint8_t get_rd(uint32_t command) {
    return (command >> 7) & 0b11111;
}

uint8_t get_func3(uint32_t command) {
    return (command >> 12) & 0b111;
}

uint8_t get_rs1(uint32_t command) {
    return (command >> 15) & 0b11111;
}

uint8_t get_rs2(uint32_t command) {
    return (command >> 20) & 0b11111;
}

uint8_t get_func7(uint32_t command) {
    return (command >> 25);
}

uint8_t get_func5(uint32_t command) {
    return (command >> 27);
}

uint8_t get_func2(uint32_t command) {
    return ((command >> 25) & 0b11);
}

int16_t get_shamt(uint32_t command) {
    return (command >> 20) & 0b11111;
}

uint8_t get_opcode(uint32_t command) {
    return (command & 0x7f);
}

bool type_u(uint32_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    std::string op;
    int32_t offset = (command & 0xfffff000);
    uint8_t rd = get_rd(command);
    if (get_opcode(command) == 0b0110111) op = "lui";
    if (get_opcode(command) == 0b0010111) op = "auipc";
    if (op.empty()) return false;
    fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), offset);
    return true;
}

bool type_uj(uint32_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    std::string op;
    uint32_t offset = (((command >> 31) & 0b1) << 20) + (((command >> 12) & 0xff) << 12) + (((command >> 20) & 0b1) << 11) + (((command >> 21) & 0x3ff) << 1);
    if ((offset & (1 << 20)) != 0) {
        offset = (offset | 0xffe00000);
    }
    uint8_t rd = get_rd(command);
    if (get_opcode(command) == 0b1101111) op = "jal";
    if (op.empty()) return false;
    fprintf(output_file, "%08x %10s: %s %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), mark_offset.c_str());
    return true;
}

bool type_i(uint32_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    std::string op;
    int16_t offset = command >> 20;
    if ((offset & (1 << 11)) != 0) {
        offset = (offset | 0xf000);
    }
    if (get_opcode(command) == 0b0000011) {
        uint8_t rs1 = get_rs1(command), func3 = get_func3(command), rd = get_rd(command);
        if (func3 == 0b000) op = "lb";
        if (func3 == 0b001) op = "lh";
        if (func3 == 0b010) op = "lw";
        if (func3 == 0b100) op = "lbu";
        if (func3 == 0b101) op = "lhu";
        if (op.empty()) return false;
        fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), offset, get_register(rs1).c_str());
        return true;
    }
    uint8_t rs1 = get_rs1(command), func3 = get_func3(command), rd = get_rd(command);
    if (get_opcode(command) == 0b1100111) {
        if (func3 == 0b000) op = "jalr"; else return false;
        fprintf(output_file, "%08x %10s: %s %s, %s, %d\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), get_register(rs1).c_str(), offset);
        return true;
    }
    if (get_opcode(command) == 0b0010011) {
        int16_t imm = command >> 20;
        if ((imm & (1 << 11)) != 0) {
            imm = (imm | 0xf000);
        }
        if (func3 == 0b000) op = "addi";
        if (func3 == 0b010) op = "slti";
        if (func3 == 0b011) op = "sltiu";
        if (func3 == 0b100) op = "xori";
        if (func3 == 0b110) op = "ori";
        if (func3 == 0b111) op = "andi";
        if (func3 == 0b001 && get_func7(command) == 0b0000000) {
            imm = get_shamt(command);
            op = "slli";
        }
        if (func3 == 0b101) {
            imm = get_shamt(command);
            uint8_t func7 = get_func7(command);
            if (func7 == 0b0000000) op = "srli";
            if (func7 == 0b0100000) op = "srai";
        }
        if (op.empty()) return false;
        fprintf(output_file, "%08x %10s: %s %s, %s, %d\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), get_register(rs1).c_str(), imm);
        return true;
    }
    return false;
}

bool type_sb(uint32_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    if (get_opcode(command) == 0b1100011) {
        std::string op;
        int16_t offset = (((command >> 31) & 0b1) << 12) + (((command >> 7) & 0b1) << 11) + (((command >> 25) & 0b111111) << 5) + (((command >> 8) & 0b1111) << 1);
        if ((offset & (1 << 12)) != 0) {
            offset = (offset | 0xe000);
        }
        uint8_t rs2 = get_rs2(command), rs1 = get_rs1(command), func3 = get_func3(command);
        if (func3 == 0b000) op = "beq";
        if (func3 == 0b001) op = "bne";
        if (func3 == 0b100) op = "blt";
        if (func3 == 0b101) op = "bge";
        if (func3 == 0b110) op = "bltu";
        if (func3 == 0b111) op = "bgeu";
        if (op.empty()) return false;
        fprintf(output_file, "%08x %10s: %s %s, %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_register(rs1).c_str(), get_register(rs2).c_str(), mark_offset.c_str());
        return true;
    }
    return false;
}

bool type_s(uint32_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b0100011) {
        std::string op;
        int16_t offset = ((command >> 25) << 5) + ((command >> 7) & 31);
        if ((offset & (1 << 11)) != 0) {
            offset = (offset | 0xf000);
        }
        uint8_t rs2 = get_rs2(command), rs1 = get_rs1(command), func3 = get_func3(command);
        if (func3 == 0b000) op = "sb";
        if (func3 == 0b001) op = "sh";
        if (func3 == 0b010) op = "sw";
        if (op.empty()) return false;
        fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), op.c_str(), get_register(rs2).c_str(), offset, get_register(rs1).c_str());
        return true;
    }
    return false;
}

bool type_r(uint32_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b0110011) {
        std::string op;
        uint8_t func5 = get_func5(command), func2 = get_func2(command), rs2 = get_rs2(command), rs1 = get_rs1(command), func3 = get_func3(command), rd = get_rd(command);
        if (func5 == 0b00000 && func2 == 0b00) {
            if (func3 == 0b000) op = "add";
            if (func3 == 0b001) op = "sll";
            if (func3 == 0b010) op = "slt";
            if (func3 == 0b011) op = "sltu";
            if (func3 == 0b100) op = "xor";
            if (func3 == 0b101) op = "srl";
            if (func3 == 0b110) op = "or";
            if (func3 == 0b111) op = "and";
        }
        if (func5 == 0b01000 && func2 == 0b00) {
            if (func3 == 0b000) op = "sub";
            if (func3 == 0b101) op = "sra";
        }
        if (func5 == 0b00000 && func2 == 0b01) {
            if (func3 == 0b000) op = "mul";
            if (func3 == 0b001) op = "mulh";
            if (func3 == 0b010) op = "mulhsu";
            if (func3 == 0b011) op = "mulhu";
            if (func3 == 0b100) op = "div";
            if (func3 == 0b101) op = "divu";
            if (func3 == 0b110) op = "rem";
            if (func3 == 0b111) op = "remu";
        }
        if (op.empty()) return false;
        fprintf(output_file, "%08x %10s: %s %s, %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_register(rd).c_str(), get_register(rs1).c_str(), get_register(rs2).c_str());
        return true;
    }
    return false;
}

void rv32im(uint32_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    if (type_u(command, cur_address, mark, output_file)) return;
    if (type_uj(command, cur_address, mark, mark_offset, output_file)) return;
    if (type_i(command, cur_address, mark, mark_offset, output_file)) return;
    if (type_sb(command, cur_address, mark, mark_offset, output_file)) return;
    if (type_s(command, cur_address, mark, output_file)) return;
    if (type_r(command, cur_address, mark, output_file)) return;
    fprintf(output_file, "%08x %10s: %s\n", cur_address, mark.c_str(), "unknown_command");
}

#endif //LAB3_RV32IM_HPP
