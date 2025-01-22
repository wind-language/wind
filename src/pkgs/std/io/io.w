@include "../string/string.wi"
@include "#unix.wi"

@pub func std::io::puts(s: ptr<char>): void {
    unix::write(STDOUT, s, std::str::len(s));
    unix::write(STDOUT, "\n", 1);
}