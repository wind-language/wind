#include <wind/backend/x86_64/backend.h>
#include <wind/generation/IR.h>

enum TypeID {
    BYTE = 0,
    WORD = 1,
    DWORD = 2,
    QWORD = 3,
    FLOAT = 4,
    DOUBLE = 5,
    LONG_DOUBLE = 6,
    POINTER = 7
};

uint8_t WindEmitter::typeIdentify(DataType *type) {
    uint8_t base = 0;
    if (type->isArray() || type->isPointer()) {
        base = TypeID::POINTER; // std will treat as a string no matter what
    } else {
        switch (type->moveSize()) {
            case 1:
                base = TypeID::BYTE;
                break;
            case 2:
                base = TypeID::WORD;
                break;
            case 4:
                base = TypeID::DWORD;
                break;
            case 8:
                base = TypeID::QWORD;
                break;
            default:
                base = TypeID::QWORD;
        }
    }
    if (type->isSigned()) {
        base |= 0x8;
    }
    return base;
}

uint64_t WindEmitter::setDesc(uint64_t desc, uint8_t index, DataType *type) {
    uint8_t base = typeIdentify(type);
    desc |= base << (index * 4);
    return desc;
}

uint64_t WindEmitter::get64bitDesc(std::vector<DataType*> &args) {
    /*
    the idea is of having a 64-bit descriptor with each entry having 4 bits
         1 for the sign and 3 for the identification
    this will result in a 16-entry descriptor
    the identification is done by the following:
        0 - byte
        1 - word
        2 - dword
        3 - qword
        4 - float
        5 - double
        6 - long double
        7 - pointer
    -- The std will take care of the rest:
    */

   if (args.size()>16)
       throw std::runtime_error("Too many arguments, cannot fit in descriptor");

    uint64_t desc = 0;
    for (int i=0;i<args.size();i++) {
        int index = args.size()-1-i; // reverse order
        desc = setDesc(desc, index, args[i]);
    }
    return desc;
}