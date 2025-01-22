@include "#types.wi"

@pub func std::str::len(s: ptr<char>): size_t {
    var len: size_t = 0;
    loop[s[len] != 0] {
        len++;
    }
    return len;
}