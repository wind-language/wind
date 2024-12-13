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
#include <assert.h>

int main(int argc, char **argv) {
  assert(argc == 2);
  const char* input_file = argv[1]; 
  WindLexer *lexer = TokenizeFile(input_file);
  if (lexer == nullptr) {
    return 1;
  }
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
  std::string file_path = backend->emit();

  delete ir;
  delete opt;
  delete backend;


  std::cerr << file_path << std::endl;
  
  return 0;
}
