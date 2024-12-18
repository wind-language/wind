import suite as wtsuite

LITERAL_TESTS = [
    {
        "name": "Simple Literal test",
        "type": "lexer",
        "args": [
            "a = 1",
            [
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "a"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.ASSIGN, "="),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "1"),
            ]
        ]
    },
    {
        "name": "Medium Literal test",
        "type": "lexer",
        "args": [
            "@a += {1, 2, 3}",
            [
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.AT, "@"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "a"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.PLUS, "+"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.ASSIGN, "="),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.LBRACE, "{"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "1"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.COMMA, ","),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "2"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.COMMA, ","),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "3"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.RBRACE, "}"),
            ]
        ],
    },
    {
        "name": "Complex Literal test",
        "type": "lexer",
        "args": [
            "// This is a comment\n c + 1 = 50\n // a - b = 0\n c = a - b + 5 * 2",
            [
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "c"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.PLUS, "+"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "1"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.ASSIGN, "="),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "50"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "c"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.ASSIGN, "="),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "a"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.MINUS, "-"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.IDENTIFIER, "b"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.PLUS, "+"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "5"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.MULTIPLY, "*"),
                wtsuite.WAPI.Token(wtsuite.WAPI.TOKEN_TYPE.INTEGER, "2"),
            ]
        ]
    }
]