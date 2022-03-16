#ifndef LAB3_RVC_HPP
#define LAB3_RVC_HPP

#include <iostream>

std::string get_reg(uint8_t reg) {
    static std::string regs[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
    return regs[reg];
}

std::string get_reg_(uint8_t reg) {
    static std::string regs[8] = {"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5"};
    return regs[reg];
}

uint8_t get_opcode(uint16_t command) {
    return (command & 0b11);
}

uint8_t get_funct3(uint16_t command) {
    return (command >> 13);
}

uint8_t get_rd(uint16_t command) {
    return ((command >> 7) & 0b11111);
}

uint8_t get_rs2(uint16_t command) {
    return ((command >> 2) & 0b11111);
}

uint8_t get_rs1(uint16_t command) {
    return ((command >> 7) & 0b11111);
}

uint8_t get_rd_(uint16_t command) {
    return ((command >> 2) & 0b111);
}

uint8_t get_rs2_(uint16_t command) {
    return ((command >> 2) & 0b111);
}

uint8_t get_rs1_(uint16_t command) {
    return ((command >> 7) & 0b111);
}

uint8_t get_imm3(uint16_t command) {
    return ((command >> 10) & 0b111);
}

uint8_t get_imm2(uint16_t command) {
    return ((command >> 5) & 0b11);
}

bool type_ciw(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b00) {
        if (get_funct3(command) == 0b000) {
            uint8_t rd_ = get_rd_(command);
            uint16_t imm = (((command >> 7) & 0b1111) << 6) + (((command >> 11) & 0b11) << 4) + (((command >> 5) & 0b1) << 3) + (((command >> 6) & 0b1) << 2);
            if (imm != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %s, %d\n", cur_address, mark.c_str(), "c.addi4spn", get_reg_(rd_).c_str(), "sp", imm);
                return true;
            }
        }
    }
    return false;
}

bool type_cl(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b00) {
        if (get_funct3(command) == 0b010) {
            uint8_t rd_ = get_rd_(command), rs1_ = get_rs1_(command), offset = (((command >> 5) & 0b1) << 6) + (((command >> 10) & 0b111) << 3) + (((command >> 6) & 0b1) << 2);
            fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), "c.lw", get_reg_(rd_).c_str(), offset, get_reg_(rs1_).c_str());
            return true;
        }
        if (get_funct3(command) == 0b110) {
            uint8_t rs2_ = get_rs2_(command), rs1_ = get_rs1_(command), offset = (((command >> 5) & 0b1) << 6) + (((command >> 10) & 0b111) << 3) + (((command >> 6) & 0b1) << 2);
            fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), "c.sw", get_reg_(rs2_).c_str(), offset, get_reg_(rs1_).c_str());
            return true;
        }
    }
    return false;
}

bool type_cs(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b01) {
        if (get_funct3(command) == 0b100) {
            std::string op;
            uint8_t rs1_ = get_rs1_(command), rs2_ = get_rs2_(command);
            if (get_imm3(command) == 0b011 && get_imm2(command) == 0b00) op = "c.sub";
            if (get_imm3(command) == 0b011 && get_imm2(command) == 0b01) op = "c.xor";
            if (get_imm3(command) == 0b011 && get_imm2(command) == 0b10) op = "c.or";
            if (get_imm3(command) == 0b011 && get_imm2(command) == 0b11) op = "c.and";
            if (!op.empty()) {
                fprintf(output_file, "%08x %10s: %s %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_reg_(rs1_).c_str(), get_reg_(rs2_).c_str());
                return true;
            }
        }
    }
    return false;
}

bool type_ci(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (command == 0x0001) {
        fprintf(output_file, "%08x %10s: %s\n", cur_address, mark.c_str(), "c.nop");
        return true;
    }
    if (get_opcode(command) == 0b01) {
        uint8_t rd = get_rd(command);
        if (get_funct3(command) == 0b000) {
            int8_t imm = (((command >> 12) & 0b1) << 5) + ((command >> 2) & 0b11111);
            if ((imm & (1 << 5)) != 0) {
                imm = (imm | 0xc0);
            }
            if (rd != 0 && imm != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %s, %d\n", cur_address, mark.c_str(), "c.addi", get_reg(rd).c_str(), get_reg(rd).c_str(), imm);
                return true;
            }
        }
        if (get_funct3(command) == 0b010) {
            int8_t imm = (((command >> 12) & 0b1) << 5) + ((command >> 2) & 0b11111);
            if ((imm & (1 << 5)) != 0) {
                imm = (imm | 0xc0);
            }
            if (rd != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.li", get_reg(rd).c_str(), imm);
                return true;
            }
        }
        if (get_funct3(command) == 0b011) {
            int32_t imm = (((command >> 12) & 0b1) << 17) + (((command >> 2) & 0b11111) << 12);
            int16_t imm_ = (((command >> 12) & 0b1) << 9) + (((command >> 3) & 0b11) << 7) + (((command >> 5) & 0b1) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 6) & 0b1) << 4);
            if ((imm & (1 << 17)) != 0) imm = (imm | 0xfffc0000);
            if ((imm_ & (1 << 9)) != 0) imm_ = (imm_ | 0xfc00);
            if (rd != 0 && rd != 2 && imm != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.lui", get_reg(rd).c_str(), imm);
                return true;
            }
            if (rd == 2 && imm_ != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %s, %d\n", cur_address, mark.c_str(), "c.addi16sp", get_reg(rd).c_str(), get_reg(rd).c_str(), imm_);
                return true;
            }
        }
        if (get_funct3(command) == 0b100) {
            uint8_t rd_ = get_rd_(command);
            if (((command >> 10) & 0b11) == 0b10) {
                int8_t imm = (((command >> 12) & 0b1) << 5) + ((command >> 2) & 0b11111);
                if ((imm & (1 << 5)) != 0) {
                    imm = (imm | 0xc0);
                }
                if (imm != 0) {
                    fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.andi", get_reg_(rd_).c_str(), imm);
                    return true;
                }
            }
            if (((command >> 10) & 0b111) == 0b000) {
                uint8_t imm = ((command >> 2) & 0b11111);
                if (imm != 0) {
                    fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.srli", get_reg_(rd_).c_str(), imm);
                    return true;
                }
            }
            if (((command >> 10) & 0b111) == 0b001) {
                uint8_t imm = ((command >> 2) & 0b11111);
                if (imm != 0) {
                    fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.srai", get_reg_(rd_).c_str(), imm);
                    return true;
                }
            }
        }
    }
    if (get_opcode(command) == 0b10) {
        if (get_funct3(command) == 0b000) {
            uint8_t rd = get_rd(command);
            uint8_t shamt = ((command >> 2) & 0b11111);
            if (rd != 0 && shamt != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %d\n", cur_address, mark.c_str(), "c.slli", get_reg(rd).c_str(), shamt);
                return true;
            }
        }
        if (get_funct3(command) == 0b010) {
            uint8_t rd = get_rd(command), offset = (((command >> 2) & 0b11) << 6) + (((command >> 12) & 0b1) << 5) + (((command >> 4) & 0b111) << 2);
            if (rd != 0) {
                fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), "c.lwsp", get_reg(rd).c_str(), offset, "sp");
                return true;
            }
        }
    }
    return false;
}

bool type_cj(uint16_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    if (get_opcode(command) == 0b01) {
        std::string op;
        int16_t offset = (((command >> 12) & 0b1) << 11) + (((command >> 8) & 0b1) << 10) + (((command >> 9) & 0b11) << 8) + (((command >> 6) & 0b1) << 7) + (((command >> 7) & 0b1) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 11) & 0b1) << 4) + (((command >> 3) & 0b111) << 1);
        if ((offset & (1 << 11)) != 0) offset = (offset | 0xf000);
        if (get_funct3(command) == 0b001) op = "c.jal";
        if (get_funct3(command) == 0b101) op = "c.j";
        if (!op.empty()) {
            fprintf(output_file, "%08x %10s: %s %s\n", cur_address, mark.c_str(), op.c_str(), mark_offset.c_str());
            return true;
        }
    }
    return false;
}

bool type_cb(uint16_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    if (get_opcode(command) == 0b01) {
        std::string op;
        uint8_t rs1_ = get_rs1_(command);
        int16_t offset = (((command >> 12) & 0b1) << 8) + (((command >> 5) & 0b11) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 10) & 0b11) << 3) + (((command >> 3) & 0b11) << 1);
        if ((offset & (1 << 8)) != 0) {
            offset = (offset | 0xff00);
        }
        if (get_funct3(command) == 0b110) op = "c.beqz";
        if (get_funct3(command) == 0b111) op = "c.bnez";
        if (!op.empty()) {
            fprintf(output_file, "%08x %10s: %s %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_reg_(rs1_).c_str(), mark_offset.c_str());
            return true;
        }
    }
    return false;
}

bool type_cr(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (command == 0x9002) {
        fprintf(output_file, "%08x %10s: %s\n", cur_address, mark.c_str(), "c.ebreak");
        return true;
    }
    if (get_opcode(command) == 0b10) {
        if (get_funct3(command) == 0b100) {
            std::string op;
            uint8_t rs1 = get_rs1(command), rs2 = get_rs2(command), rd;
            if (rs1 != 0 && rs2 == 0) {
                if ((command & (1 << 12)) == 0) op = "c.jr"; else op = "c.jalr";
                fprintf(output_file, "%08x %10s: %s %s\n", cur_address, mark.c_str(), op.c_str(), get_reg(rs1).c_str());
                return true;
            }
            if (rs1 != 0) {
                rd = rs1;
                if ((command & (1 << 12)) == 0) op = "c.mv"; else op = "c.add";
                fprintf(output_file, "%08x %10s: %s %s, %s\n", cur_address, mark.c_str(), op.c_str(), get_reg(rd).c_str(), get_reg(rs2).c_str());
                return true;
            }
        }
    }
    return false;
}

bool type_css(uint16_t command, uint32_t cur_address, const std::string &mark, FILE *output_file) {
    if (get_opcode(command) == 0b10) {
        if (get_funct3(command) == 0b110) {
            int8_t rs2 = get_rs2(command), offset = (((command >> 7) & 0b11) << 6) + (((command >> 9) & 0b1111) << 2);
            fprintf(output_file, "%08x %10s: %s %s, %d(%s)\n", cur_address, mark.c_str(), "c.swsp", get_reg(rs2).c_str(), offset, "sp");
            return true;
        }
    }
    return false;
}

void rvc(uint16_t command, uint32_t cur_address, const std::string &mark, const std::string &mark_offset, FILE *output_file) {
    if (type_ciw(command, cur_address, mark, output_file)) return;
    if (type_cl(command, cur_address, mark, output_file)) return;
    if (type_cs(command, cur_address, mark, output_file)) return;
    if (type_ci(command, cur_address, mark, output_file)) return;
    if (type_cj(command, cur_address, mark, mark_offset, output_file)) return;
    if (type_cb(command, cur_address, mark, mark_offset, output_file)) return;
    if (type_cr(command, cur_address, mark, output_file)) return;
    if (type_css(command, cur_address, mark, output_file)) return;
    fprintf(output_file, "%08x %10s: %s\n", cur_address, mark.c_str(), "unknown_command");
}

#endif //LAB3_RVC_HPP
