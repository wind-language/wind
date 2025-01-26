@include "#types.wi"
@include "../mem/mem.wi"

@pub func std::str::len(s: ptr<char>): size_t {
    var len: size_t = 0;
    loop[s[len] != 0] {
        len++;
    }
    return len;
}

@pub func std::str::fmt::untoa(n: u64): ptr<char> {
    var buf: ptr<char> = guard![std::mem::alloc(24)]; // max u64 is 20 digits
    var i: int = 0;
    branch [
        n == 0: {
            buf[0] = '0';
            buf[1] = 0;
            return buf;
        }
    ]
    var ch: char;
    loop [n > 0] {
        asm {
            mov rax, ?n;
            xor rdx, rdx;
            mov rcx, 10;
            div rcx;
            add dl, '0';
            mov ?ch, dl;

            mov rax, ?buf;
            mov ecx, ?i;
            mov [rax+rcx], dl;
        }
        i++;
        asm {
            mov rax, ?n;
            xor rdx, rdx;
            mov rcx, 10;
            div rcx;
            mov ?n, rax;
        }
    }
    std::mem::reverse(buf, i as size_t);
    buf[i] = 0;
    return buf;
}

@pub func std::str::fmt::ntoa(n: s64): ptr<char> {
    var buf: ptr<char> = guard![std::mem::alloc(24)]; // max s64 is 20 digits
    var i: int = 0;
    var sign: bool = n < 0;
    branch [
        sign: n = -n;
    ]
    branch [
        n == 0: {
            buf[0] = '0';
            buf[1] = 0;
            return buf;
        }
    ]
    var ch: char;
    loop [n > 0] {
        asm {
            mov rax, ?n;
            xor rdx, rdx;
            mov rcx, 10;
            div rcx;
            add dl, '0';
            mov ?ch, dl;

            mov rax, ?buf;
            mov ecx, ?i;
            mov [rax+rcx], dl;
        }
        i++;
        asm {
            mov rax, ?n;
            xor rdx, rdx;
            mov rcx, 10;
            div rcx;
            mov ?n, rax;
        }
    }
    branch [
        sign: { buf[i]='-'; i++; }
    ]
    std::mem::reverse(buf, i as size_t);
    buf[i] = 0;
    return buf;
}