#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/bridge/ast.h>
#include <wind/bridge/ast_printer.h>
#include <wind/generation/compiler.h>
#include <wind/generation/optimizer.h>
#include <wind/generation/ir_printer.h>
#include <wind/generation/backend.h>
#include <wind/bridge/opt_flags.h>

#include <iostream>

int main() {
  WindLexer *lexer = TokenizeFile("grammar/wind.0.w");
  WindParser *parser = new WindParser(lexer->get(), lexer->source());
  Body *ast = parser->parse();
  std::cout << "AST:" << std::endl;
  ASTPrinter *printer = new ASTPrinter();
  ast->accept(*printer);
  std::cout << "\n\n";

  std::cout << "IR:" << std::endl;
  WindCompiler *ir = new WindCompiler(ast);
  WindOptimizer *opt = new WindOptimizer(ir->get());
  IRBody *optimized = opt->get();
  IRPrinter *ir_printer = new IRPrinter(optimized);
  ir_printer->print();
  WindEmitter *backend = new WindEmitter(optimized);
  backend->emit();

  delete ir;
  delete opt;
  delete backend;

  
  return 0;
}
