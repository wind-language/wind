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
    var buf: ptr<char> = std::mem::alloc(32);
    var i: size_t = 0;
    loop[n > 0] {
        buf[i] = n % 10 + '0';
        n = n/10;
        i++;
    }
    std::mem::reverse(buf, i);
    return buf;
}

@pub func std::str::fmt::ntoa(n: s64): ptr<char> {
    // signed integer to string
    var buf: ptr<char> = std::mem::alloc(32);
    var i: size_t = 0;
    var sign: bool = n < 0;
    branch [
        sign: {
            n = -n;
            i++;
        }
    ]
    loop[n > 0] {
        buf[i] = n % 10 + '0';
        n = n/10;
        i++;
    }
    std::mem::reverse(buf, i-(sign as size_t));
    branch[
        sign: {
            buf[0] = '-';
        }
    ]
    return buf;
}
