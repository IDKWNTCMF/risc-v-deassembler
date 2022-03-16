#include "rv32im.hpp"
#include "rvc.hpp"

#include <iostream>
#include <map>

class FileNotFoundException : public std::exception {
private:
    std::string error;
public:
    explicit FileNotFoundException(std::string err) : error(std::move(err)) {}
    const char * what() const noexcept override {
        return error.c_str();
    }
};

class FileFormatException : public std::exception {
private:
    std::string error;
public:
    explicit FileFormatException(std::string err) : error(std::move(err)) {}
    const char * what() const noexcept override {
        return error.c_str();
    }
};

#pragma pack(push, 1)

struct ELF32_File_Header {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ELF32_Section_Header {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

struct ELF32_Symbol {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
};

#pragma pack(pop)

std::string get_section_name(const ELF32_Section_Header &section_header, const uint8_t shstrtab[], size_t sz) {
    std::string name = "";
    size_t cur = section_header.sh_name;
    while (cur < sz && shstrtab[cur] != 0) {
        name += char(shstrtab[cur]);
        cur++;
    }
    return name;
}

std::string get_symbol_name(const ELF32_Symbol &symbol, const uint8_t strtab[], size_t sz) {
    std::string name = "";
    size_t cur = symbol.st_name;
    while (cur < sz && strtab[cur] != 0) {
        name += char(strtab[cur]);
        cur++;
    }
    return name;
}

std::string get_type(uint8_t type) {
    if (type == 0) return "NOTYPE";
    if (type == 1) return "OBJECT";
    if (type == 2) return "FUNC";
    if (type == 3) return "SECTION";
    if (type == 4) return "FILE";
    if (type == 5) return "COMMON";
    if (type == 6) return "TLS";
    if (type == 10) return "LOOS";    ///NOT SURE
    if (type == 12) return "HIOS";    ///NOT SURE
    if (type == 13) return "LOPROC";  ///NOT SURE
    if (type == 15) return "HIPROC";  ///NOT SURE
    return "";
}

std::string get_bind(uint8_t bind) {
    if (bind == 0) return "LOCAL";
    if (bind == 1) return "GLOBAL";
    if (bind == 2) return "WEAK";
    if (bind == 10) return "LOOS";    ///NOT SURE
    if (bind == 12) return "HIOS";    ///NOT SURE
    if (bind == 13) return "LOPROC";  ///NOT SURE
    if (bind == 15) return "HIPROC";  ///NOT SURE
    return "";
}

std::string get_vis(uint8_t vis) {
    if (vis == 0) return "DEFAULT";
    if (vis == 1) return "INTERNAL";
    if (vis == 2) return "HIDDEN";
    if (vis == 3) return "PROTECTED";
    return "";
}

std::string get_index(uint16_t index) {
    if (index == 0x0000) return "UNDEF";
    if (index == 0xff00) return "BEFORE"; ///NOT SURE
    if (index == 0xff01) return "AFTER";  ///NOT SURE
    if (index == 0xff20) return "LOOS";   ///NOT SURE
    if (index == 0xff3f) return "HIOS";   ///NOT SURE
    if (index == 0xfff1) return "ABS";
    if (index == 0xfff2) return "COMMON"; ///NOT SURE
    if (index == 0xffff) return "XINDEX"; ///NOT SURE
    return std::to_string(index);
}

void print_symbol_info(const ELF32_Symbol &symbol, size_t idx, const std::string &name, FILE *output_file) {
    uint8_t type = (symbol.st_info & 0xf), bind = (symbol.st_info >> 4), vis = (symbol.st_other & 0b11);
    fprintf(output_file, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", (int)idx, symbol.st_value, symbol.st_size, get_type(type).c_str(), get_bind(bind).c_str(), get_vis(vis).c_str(), get_index(symbol.st_shndx).c_str(), name.c_str());
}

void disasm(FILE *input_file, FILE *output_file) {
    ELF32_File_Header file_header{};
    if (fread(&file_header, sizeof file_header, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
    if (file_header.e_ident[0] != 0x7f || file_header.e_ident[1] != 0x45 || file_header.e_ident[2] != 0x4c || file_header.e_ident[3] != 0x46) throw FileFormatException("Wrong format of input file!");

    ELF32_Section_Header shstrtab_header{};
    fseek(input_file, file_header.e_shoff + file_header.e_shstrndx * file_header.e_shentsize, SEEK_SET);
    if (fread(&shstrtab_header, sizeof shstrtab_header, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");;

    uint8_t shstrtab[shstrtab_header.sh_size];
    fseek(input_file, shstrtab_header.sh_offset, SEEK_SET);
    if (fread(&shstrtab, shstrtab_header.sh_size, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
    ELF32_Section_Header text_header{}, symtab_header{}, strtab_header{};

    for (size_t i = 0; i < file_header.e_shnum; i++) {
        ELF32_Section_Header section_header{};

        fseek(input_file, file_header.e_shoff + i * file_header.e_shentsize, SEEK_SET);
        if (fread(&section_header, sizeof section_header, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");

        if (section_header.sh_name != 0) {
            std::string name = get_section_name(section_header, shstrtab, shstrtab_header.sh_size);
            if (name == ".text") std::swap(text_header, section_header);
            if (name == ".symtab") std::swap(symtab_header, section_header);
            if (name == ".strtab") std::swap(strtab_header, section_header);
        }
    }

    uint8_t strtab[strtab_header.sh_size];
    fseek(input_file, strtab_header.sh_offset, SEEK_SET);
    if (fread(&strtab, strtab_header.sh_size, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");

    ELF32_Symbol tmp{};
    ELF32_Symbol symbols[symtab_header.sh_size / sizeof tmp];
    std::map<uint32_t, std::string> marks;

    size_t cur = 0;
    for (size_t i = 0; i < symtab_header.sh_size; i += sizeof tmp) {
        fseek(input_file, symtab_header.sh_offset + cur * sizeof tmp, SEEK_SET);
        if (fread(&symbols[cur], sizeof tmp, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");

        if (symbols[cur].st_name != 0) {
            marks[symbols[cur].st_value] = get_symbol_name(symbols[cur], strtab, strtab_header.sh_size);;
        }
        cur++;
    }

    cur = 0;
    uint32_t cur_address = text_header.sh_addr;
    fseek(input_file, text_header.sh_offset, SEEK_SET);
    while (cur < text_header.sh_size) {
        uint16_t part1;
        if (fread(&part1, sizeof part1, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
        if ((part1 & 0b11) != 0b11) {
            uint16_t command = part1;
            if ((command & 0b11) == 0b01 && (((command >> 13) & 0b111) == 0b001 || ((command >> 13) & 0b111) == 0b101)) {
                int16_t offset = (((command >> 12) & 0b1) << 11) + (((command >> 8) & 0b1) << 10) + (((command >> 9) & 0b11) << 8) + (((command >> 6) & 0b1) << 7) + (((command >> 7) & 0b1) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 11) & 0b1) << 4) + (((command >> 3) & 0b111) << 1);
                if ((offset & (1 << 11)) != 0) offset = (offset | 0xf000);
                if (marks[cur_address + offset].empty()) {
                    char buffer[15];
                    sprintf(buffer, "LOC_%05x", cur_address + offset);
                    marks[cur_address + offset] = buffer;
                }
            }
            if ((command & 0b11) == 0b01 && (((command >> 13) & 0b111) == 0b110 || ((command >> 13) & 0b111) == 0b111)) {
                int16_t offset = (((command >> 12) & 0b1) << 8) + (((command >> 5) & 0b11) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 10) & 0b11) << 3) + (((command >> 3) & 0b11) << 1);
                if ((offset & (1 << 8)) != 0) {
                    offset = (offset | 0xff00);
                }
                if (marks[cur_address + offset].empty()) {
                    char buffer[15];
                    sprintf(buffer, "LOC_%05x", cur_address + offset);
                    marks[cur_address + offset] = buffer;
                }
            }
            cur_address += 2;
            cur += 2;
        } else {
            uint16_t part2;
            if (fread(&part2, sizeof part2, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
            uint32_t command = (part2 << 16) + part1;
            int32_t offset;
            if ((command & 0b1111111) == 0b1101111) {
                offset = (((command >> 31) & 0b1) << 20) + (((command >> 12) & 0xff) << 12) + (((command >> 20) & 0b1) << 11) + (((command >> 21) & 0x3ff) << 1);
                if ((offset & (1 << 20)) != 0) {
                    offset = (offset | 0xffe00000);
                }
                if (marks[cur_address + offset].empty()) {
                    char buffer[15];
                    sprintf(buffer, "LOC_%05x", cur_address + offset);
                    marks[cur_address + offset] = buffer;
                }
            }
            if ((command & 0b1111111) == 0b1100011) {
                offset = (((command >> 31) & 0b1) << 12) + (((command >> 7) & 0b1) << 11) + (((command >> 25) & 0b111111) << 5) + (((command >> 8) & 0b1111) << 1);
                if ((offset & (1 << 12)) != 0) {
                    offset = (offset | 0xffffe000);
                }
                if (marks[cur_address + offset].empty()) {
                    char buffer[15];
                    sprintf(buffer, "LOC_%05x", cur_address + offset);
                    marks[cur_address + offset] = buffer;
                }
            }
            cur_address += 4;
            cur += 4;
        }
    }

    fprintf(output_file, ".text\n");

    cur = 0;
    cur_address = text_header.sh_addr;
    fseek(input_file, text_header.sh_offset, SEEK_SET);
    while (cur < text_header.sh_size) {
        uint16_t part1;
        if (fread(&part1, sizeof part1, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
        if ((part1 & 0b11) != 0b11) {
            uint16_t command = part1;
            std::string mark_offset;
            if ((command & 0b11) == 0b01 && (((command >> 13) & 0b111) == 0b001 || ((command >> 13) & 0b111) == 0b101)) {
                int16_t offset = (((command >> 12) & 0b1) << 11) + (((command >> 8) & 0b1) << 10) + (((command >> 9) & 0b11) << 8) + (((command >> 6) & 0b1) << 7) + (((command >> 7) & 0b1) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 11) & 0b1) << 4) + (((command >> 3) & 0b111) << 1);
                if ((offset & (1 << 11)) != 0) offset = (offset | 0xf000);
                mark_offset = marks[cur_address + offset];
            }
            if ((command & 0b11) == 0b01 && (((command >> 13) & 0b111) == 0b110 || ((command >> 13) & 0b111) == 0b111)) {
                int16_t offset = (((command >> 12) & 0b1) << 8) + (((command >> 5) & 0b11) << 6) + (((command >> 2) & 0b1) << 5) + (((command >> 10) & 0b11) << 3) + (((command >> 3) & 0b11) << 1);
                if ((offset & (1 << 8)) != 0) {
                    offset = (offset | 0xff00);
                }
                mark_offset = marks[cur_address + offset];
            }
            rvc(part1, cur_address, marks[cur_address], mark_offset, output_file);
            cur_address += 2;
            cur += 2;
        } else {
            uint16_t part2;
            if (fread(&part2, sizeof part2, 1, input_file) != 1) throw FileFormatException("An error occurred while reading!");
            uint32_t command = (part2 << 16) + part1;
            int32_t offset;
            std::string mark_offset;
            if ((command & 0b1111111) == 0b1101111) {
                offset = (((command >> 31) & 0b1) << 20) + (((command >> 12) & 0xff) << 12) + (((command >> 20) & 0b1) << 11) + (((command >> 21) & 0x3ff) << 1);
                if ((offset & (1 << 20)) != 0) {
                    offset = (offset | 0xffe00000);
                }
                mark_offset = marks[cur_address + offset];
            }
            if ((command & 0b1111111) == 0b1100011) {
                offset = (((command >> 31) & 0b1) << 12) + (((command >> 7) & 0b1) << 11) + (((command >> 25) & 0b111111) << 5) + (((command >> 8) & 0b1111) << 1);
                if ((offset & (1 << 12)) != 0) {
                    offset = (offset | 0xffffe000);
                }
                mark_offset = marks[cur_address + offset];
            }
            rv32im(command, cur_address, marks[cur_address], mark_offset, output_file);
            cur_address += 4;
            cur += 4;
        }
    }

    fprintf(output_file, "\n.symtab\n");
    fprintf(output_file, "%s %-15s %7s %-8s %-8s %-8s %6s %s\n", "Symbol", "Value", "Size", "Type", "Bind", "Vis", "Index", "Name");

    cur = 0;
    for (size_t i = 0; i < symtab_header.sh_size; i += sizeof tmp) {
        if (symbols[cur].st_name != 0) {
            print_symbol_info(symbols[cur], cur, marks[symbols[cur].st_value], output_file);
        } else {
            print_symbol_info(symbols[cur], cur, "", output_file);
        }
        cur++;
    }
}

int main(int argc, char *argv[]) {
    try {
        if (argc != 3) throw std::invalid_argument("Invalid number of arguments!");
        FILE *input_file = fopen(argv[1], "rb"), *output_file = fopen(argv[2], "w");
        if (input_file == nullptr) throw FileNotFoundException("Unable to open input file!");
        if (output_file == nullptr) throw FileNotFoundException("Unable to open output file!");
        disasm(input_file, output_file);
        fclose(input_file);
        fclose(output_file);
    } catch (std::invalid_argument &e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (FileNotFoundException &e) {
        std::cerr << e.what() << '\n';
        return 2;
    } catch (FileFormatException &e) {
        std::cerr << e.what() << '\n';
        return 3;
    }
    return 0;
}
