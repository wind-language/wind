#include <string>
#include <stdint.h>
#include <vector>

#ifndef WRITER_MANAGE_H
#define WRITER_MANAGE_H

struct Reg {
    uint8_t id;
    uint8_t size;
    enum RegType {
        GPR,
        SEG
    } type;
    bool signed_value=true;
};

struct RegOffset {
    Reg reg;
    int64_t offset;
};

class Mem {
public:
    Reg base;
    Reg index;
    int64_t offset;
    uint16_t size;
    uint16_t id = 90;
    std::string label;
    enum BaseType {
        BASE,
        LABEL
    } base_type;
    enum OffsetType {
        IMM,
        REG,
        REG_IMM
    } offset_type;

    Mem(Reg base, int64_t offset, uint16_t size) : base(base), offset(offset), size(size) { base_type = BASE; offset_type = IMM; }
    Mem(std::string label, int64_t offset, uint16_t size) : label(label), offset(offset), size(size) { base_type = LABEL; offset_type = IMM; }
    Mem(Reg base, Reg index, uint16_t size) : base(base), index(index), size(size) { base_type = BASE; offset_type = REG; }
    Mem(std::string label, Reg index, int64_t offset, uint16_t size) : label(label), index(index), offset(offset), size(size) { base_type = LABEL; offset_type = REG; }
    Mem(Reg base, Reg index, int64_t offset, uint16_t size) : base(base), index(index), offset(offset), size(size) { base_type = BASE; offset_type = REG_IMM; }
};

struct Label {
    std::string name;
    std::string content;
};
struct Section {
    std::string name;
    std::vector<Label> labels;
    std::string header;
};

class WindWriter {
    class Content {
    public:
        std::vector<Section> sections;
        uint16_t cs_id;
        uint16_t cl_id;
        
        uint16_t NewSection(std::string name);
        void BindSection(uint16_t id);
        void WriteHdr(std::string content);
        
        uint16_t NewLabel(std::string name);
        void BindLabel(uint16_t id);
        void WriteLabel(std::string content);

        std::string LabelById(uint16_t id) { return sections[cs_id].labels[id].name; }
    } content;

public:
    // Section management
    uint16_t NewSection(std::string name) { return content.NewSection(name); }
    void BindSection(uint16_t id) { content.BindSection(id); }
    void WriteHdr(std::string content) { this->content.WriteHdr(content+"\n"); }

    // Label management
    uint16_t NewLabel(std::string name) { return content.NewLabel(name); }
    void BindLabel(uint16_t id) { content.BindLabel(id); }
    void Write(std::string content) { this->content.WriteLabel(content + "\n"); }
    std::string LabelById(uint16_t id) { return content.LabelById(id); }

    void Global(std::string name) { this->WriteHdr(".global " + name); }
    void Extern(std::string name) { this->WriteHdr(".extern " + name); }
    void Align(uint16_t size) { this->Write(".align " + std::to_string(size)); }

    virtual std::string ResolveReg(Reg &reg) { return ""; }
    virtual std::string ResolveMem(Mem &mem) { return ""; }
    virtual std::string ResolveRegOff(RegOffset &offs) { return ""; }
    virtual std::string ResolveWord(uint16_t size) { return ""; }

    void Write(std::string instr, Reg dst, Reg src) { this->Write(instr + " " + ResolveReg(dst) + ", " + ResolveReg(src)); }
    void Write(std::string instr, Reg dst, Mem src) { this->Write(instr + " " + ResolveReg(dst) + ", " + ResolveMem(src)); }
    void Write(std::string instr, Mem dst, Reg src) { this->Write(instr + " " + ResolveMem(dst) + ", " + ResolveReg(src)); }
    void Write(std::string instr, Mem dst, int64_t imm) { this->Write(instr + " " + ResolveMem(dst) + ", " + std::to_string(imm)); }
    void Write(std::string instr, Reg dst, int64_t imm) { this->Write(instr + " " + ResolveReg(dst) + ", " + std::to_string(imm)); }

    void Write(std::string instr, int64_t imm, Reg src) { this->Write(instr + " " + std::to_string(imm) + ", " + ResolveReg(src)); }
    void Write(std::string instr, int64_t imm, Mem src) { this->Write(instr + " " + std::to_string(imm) + ", " + ResolveMem(src)); }

    void Write(std::string instr, Reg dst) { this->Write(instr + " " + ResolveReg(dst)); }
    void Write(std::string instr, Mem dst) { this->Write(instr + " " + ResolveMem(dst)); }
    void Write(std::string instr, std::string label) { this->Write(instr + " " + label); }

    void Write(std::string instr, int64_t imm) { this->Write(instr + " " + std::to_string(imm)); }

    void Write(std::string instr, Reg src, RegOffset offs) { this->Write(instr + " " + ResolveReg(src) + ", " + ResolveRegOff(offs)); }

    void Write(std::string instr, Mem dst, Mem src) { this->Write(instr + " " + ResolveMem(dst) + ", " + ResolveMem(src)); }

    // Memory
    Mem ptr(Reg base, int64_t offset, uint16_t size) { return Mem(base, offset, size); }
    Mem ptr(std::string label, int64_t offset, uint16_t size) { return Mem(label, offset, size); }
    Mem ptr(Reg base, Reg index, uint16_t size) { return Mem(base, index, size); }
    Mem ptr(std::string label, Reg index, int64_t offset, uint16_t size) { return Mem(label, index, offset, size); }
    Mem ptr(Reg base, Reg index, int64_t offset, uint16_t size) { return Mem(base, index, offset, size); }

    RegOffset roff(Reg reg, int64_t offset) { return {reg, offset}; }

    // Data
    void String(std::string content) { this->Write(".string \"" + content + "\""); }
    void Byte(long value) { this->Write(".byte " + std::to_string(value)); }
    void Word(long value) { this->Write(".short " + std::to_string(value)); }
    void Dword(long value) { this->Write(".long " + std::to_string(value)); }
    void Qword(long value) { this->Write(".quad " + std::to_string(value)); }
    void Reserve(long size) { this->Write(".space " + std::to_string(size)); }

    // Emission
    std::string Emit();
};

#endif