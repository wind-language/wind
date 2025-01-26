@include "../string/string.wi"
@include "#unix.wi"

@const ARG_BYTE = 0x01;
@const ARG_WORD = 0x02;
@const ARG_DWORD = 0x03;
@const ARG_QWORD = 0x04;
@const ARG_PTR = 0x07;    // string

namespace __wind_internal {
    func typed2str(val: u64, type_desc: byte): ptr<char> {
        var sign: bool = type_desc & 0x08;
        var type_id: byte = type_desc & 0x07;

        branch [
            type_id == ARG_PTR: return guard![val] as ptr<char>;
            sign: return std::str::fmt::ntoa(val as s64);
            else: return std::str::fmt::untoa(val as u64);
        ]
    }

    func write_typed(s: ptr<char>, type_desc: byte): void {
        s = __wind_internal::typed2str(s as u64, type_desc);
        unix::write(STDOUT, s, std::str::len(s));
    }
    func writenewline(): void {
        unix::write(STDOUT, "\n", 1);
    }

    namespace __printer {
        func c2_print(): void { // callee of original @argpush caller function, 1 return address on rsp
            var arg_len: size_t;
            var type_desc: u64;
            asm { mov ?arg_len, rax; }
            asm { mov ?type_desc, rbx; }
            var i: size_t = 0;
            var arg: ptr<char>;
            var type_entry: byte;
            loop [i < arg_len] {
                asm {
                    mov rax, ?i;
                    mov rdi, [rbp+rax*8+24];   // 8 (size of ptr) + 8*(2 rets) 
                    mov ?arg, rdi;
                    // extract the arg corresponding type_entry
                    mov rdi, ?type_desc;
                    lea rcx, [rax*4];
                    shr rdi, cl;
                    and rdi, 0x0F;
                    mov ?type_entry, dil;
                }
                __wind_internal::write_typed(arg, type_entry);
                i++;
            }
        }
    }
}

@pub func std::io::puts(s: ptr<char>): void {
    unix::write(STDOUT, s, std::str::len(s));
    unix::write(STDOUT, "\n", 1);
}

@pub @argpush func std::io::print(): void {
    __wind_internal::__printer::c2_print();
}

@pub @argpush func std::io::println(): void {
    __wind_internal::__printer::c2_print();
    __wind_internal::writenewline();
}