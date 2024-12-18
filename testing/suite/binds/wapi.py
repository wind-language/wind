import libpyapi
class WAPI:
    TOKEN_TYPE = libpyapi.TokenType
    def file2tok(file:str) -> list:
        lexer = libpyapi.TokenizeFile(file)
        return WAPI.stream2array(lexer.get())

    def lit2tok(lit:str) -> list:
        lexer = libpyapi.TokenizeLiteral(lit)
        return WAPI.stream2array(lexer.get())
    
    def parseFile(file:str) -> list:
        pass
    
    def parseLiteral(lit:str) -> list:
        pass
    
    def stream2array(stream: libpyapi.TokenStream):
        tokens = []
        token = stream.pop()
        while (token):
            tokens.append(token)
            token = stream.pop()
        return tokens

    def Token(type:int, value:str):
        return libpyapi.Token(
            value,
            type,
            "",
            ((0,0), (0,0)),
            0
        );