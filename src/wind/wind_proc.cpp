#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>

int main() {
  WindLexer *lexer = TokenizeFile("grammar/wind.0.w");
  WindParser *parser = new WindParser(lexer->get(), lexer->source());
  parser->parse();
  return 0;
}
