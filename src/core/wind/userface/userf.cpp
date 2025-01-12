/**
 * @file userf.cpp
 * @brief Implementation of the WindUserInterface class.
 */

#include <wind/userface/userf.h>

#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/bridge/ast.h>
#include <wind/bridge/ast_printer.h>
#include <wind/generation/compiler.h>
#include <wind/generation/optimizer.h>
#include <wind/generation/ir_printer.h>
#include <wind/processing/utils.h>
#include <wind/isc/isc.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/backend/interface/ld.h>

#include <filesystem>
#include <iostream>

#ifndef WIND_RUNTIME_PATH
#warning "WIND_RUNTIME_PATH not defined"
#define WIND_RUNTIME_PATH ""
#endif

const char HELP[] = "Usage: wind [options] [files]\n"
                    "Options:\n"
                    "  -ej  Emit object file\n"
                    "  -o   Output file path\n"
                    "  -sa  Show AST\n"
                    "  -si  Show IR\n"
                    "  -ss"
                    "  -h   Display this help message\n";

/**
 * @brief Constructor for WindUserInterface.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 */
WindUserInterface::WindUserInterface(int argc, char **argv) {
  this->flags = 0;
  this->argv = argv;
  for (int i = 1; i < argc; i++) {
    parseArgument(std::string(argv[i]), i);
  }
}

/**
 * @brief Destructor for WindUserInterface.
 */
WindUserInterface::~WindUserInterface() {
  for (std::string obj : this->objects) {
    std::filesystem::remove(obj);
  }
}

/**
 * @brief Parses a command-line argument.
 * @param arg The argument to parse.
 * @param i The index of the argument.
 */
void WindUserInterface::parseArgument(std::string arg, int &i) {
  if (arg == "-ej") {
    this->flags |= EMIT_OBJECT;
  }
  else if (arg == "-o") {
    this->output = std::string(argv[++i]);
  }
  else if (arg == "-sa") {
    this->flags |= SHOW_AST;
  }
  else if (arg == "-si") {
    this->flags |= SHOW_IR;
  }
  else if (arg == "-ss") {
    this->flags |= SHOW_ASM;
  }
  else if (arg == "-h") {
    std::cout << HELP;
    _Exit(0);
  }
  else {
    files.push_back(arg);
  }
}

/**
 * @brief Emits an object file from the given path.
 * @param path The path to the source file.
 */
void WindUserInterface::emitObject(std::string path) {
  global_isc->tabulaRasa();
  WindLexer *lexer = TokenizeFile(path.c_str());
  if (lexer == nullptr) {
    std::cerr << "File not found: " << path << std::endl;
    _Exit(1);
  }
  WindParser *parser = new WindParser(lexer->get(), path);
  Body *ast = parser->parse();
  if (flags & SHOW_AST) {
    std::cout << "[" << path << "] AST:" << std::endl;
    ASTPrinter *printer = new ASTPrinter();
    ast->accept(*printer);
    std::cout << "\n\n";
  }

  WindCompiler *ir = new WindCompiler(ast);
  WindOptimizer *opt = new WindOptimizer(ir->get());
  IRBody *optimized = opt->get();

  if (flags & SHOW_IR) {
    std::cout << "[" << path << "] IR:" << std::endl;
    IRPrinter *ir_printer = new IRPrinter(optimized);
    ir_printer->print();
    std::cout << "\n\n";
  }

  WindEmitter *backend = new WindEmitter(optimized);
  backend->Process();
  std::string output = "";
  if (this->flags & EMIT_OBJECT && this->files.size()==1 && this->output != "") {
    output = backend->emitObj(this->output);
  } else {
    output = backend->emitObj();
  }
  if (this->flags & SHOW_ASM) {
    std::cout << "[" << path << "] ASM:" << std::endl;
    std::cout << backend->GetAsm() << std::endl;
  }
  this->objects.push_back(output);

  std::vector<std::string> user_ld_flags = global_isc->getLdFlags();
  for (std::string flag : user_ld_flags) {
    this->user_ld_flags.push_back(flag);
  }

  std::vector<std::string> pending_src = global_isc->getImports();
  for (std::string src : pending_src) {
    global_isc->popImport();
    this->emitObject(src);
  }
  
  delete ir;
  delete opt;
  delete backend;
}

void WindUserInterface::ldDefFlags(WindLdInterface *ld) {
  ld->addFlag("-m elf_x86_64");
  for (std::string flag : this->user_ld_flags) {
    ld->addFlag(flag);
  }
}

void WindUserInterface::ldExecFlags(WindLdInterface *ld) {
  ld->addFlag("-dynamic-linker /lib64/ld-linux-x86-64.so.2");
  ld->addFlag("-lc");
  std::string path = std::string(WIND_RUNTIME_PATH);
  if (path[0] != '/') {
    path = std::filesystem::path(getExeDir()).append(path).string();
  }
  std::string runtime_lib = path + "/wrt.o";
  ld->addFile(runtime_lib);
}

/**
 * @brief Processes the input files.
 */

void WindUserInterface::processFiles() {
  if (files.size() == 0) {
    std::cerr << "No input file provided\n";
    _Exit(1);
  }
  for (std::string file : files) {
    this->emitObject(file);
  }

  WindLdInterface *ld = new WindLdInterface(this->output);
  ldDefFlags(ld);
  if (this->flags & EMIT_OBJECT) {
    if (this->files.size()==1 && this->output != "") {
      delete ld;
      return;
    }
    ld->addFlag("-r");
  } else {
    ldExecFlags(ld);
  }
  for (std::string obj : this->objects) {
    ld->addFile(obj);
  }
  std::string outtmp = ld->link();
  for (std::string obj : this->objects) {
    std::filesystem::remove(obj);
  }
  if (this->output == "") {
    std::filesystem::remove(outtmp);
  }

  delete ld;
}