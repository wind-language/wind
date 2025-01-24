@include "#types.wi"
@include "#libc.wi"

@pub func std::mem::copy(dst: ptr<char>, src: ptr<char>, len: size_t): void {
    var i: size_t = 0;
    loop[i<len] {
        dst[i] = src[i];
        i++;
    }
}

@pub func std::mem::set(dst: ptr<char>, val: byte, len: size_t): void {
    var i: size_t = 0;
    loop[i<len] {
        dst[i] = val;
        i++;
    }
}

@pub func std::mem::zero(dst: ptr<char>, len: size_t): void {
    std::mem::set(dst, 0, len);
}

@pub func std::mem::alloc(size: size_t): ptr<char> {
    return malloc(size);
}

@pub func std::mem::free(ptr: ptr<char>): void {
    free(ptr);
}

@pub @pure[stchk] func std::mem::reverse(s: ptr<char>, len: size_t): void {
    var l: size_t = 0;
    var j: size_t = len - 1;
    loop [l<j] {
        asm {
            mov rax, ?s;
            add rax, ?l;
            mov al, byte [rax];

            mov rbx, ?s;
            add rbx, ?j;
            mov bl, byte [rbx];

            mov rax, ?s;
            add rax, ?l;
            mov byte [rax], bl;

            mov rbx, ?s;
            add rbx, ?j;
            mov byte [rbx], al;
        }

        l++;
        j--;
    }
}