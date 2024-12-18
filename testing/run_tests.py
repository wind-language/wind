import suite as wtsuite

from tests.lexer import LEXER_TESTS

# Static
runner = wtsuite.WTest()
print("[LEXER TESTS]")
for test_args in LEXER_TESTS:
    runner.runTests(*test_args)