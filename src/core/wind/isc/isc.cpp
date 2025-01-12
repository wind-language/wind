#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/processing/token.h>
#include <wind/reporter/parser.h>
#include <wind/isc/isc.h>
#include <wind/processing/utils.h>
#include <wind/bridge/ast_printer.h>
#include <map>
#include <memory>
#include <iostream>
#include <filesystem>

WindISC *global_isc;

WindISC::WindISC() {}

void WindISC::setPath(uint16_t id, std::string path) {
  if (id >= this->sources.size()) {
    this->sources.insert({id, {path, nullptr, nullptr}});
  } else {
    this->sources[id].path = path;
  }
}

void WindISC::setStream(uint16_t id, TokenStream *stream) {
  this->sources[id].stream = stream;
}

void WindISC::newParserReport(uint16_t id, std::string src) {
  this->sources[id].parser_report = new ParserReport(src);
}

int16_t WindISC::getSrcId(std::string path) {
  path = getRealPath(path);
  for (const auto &src : this->sources) {
    if (src.second.path == path) {
      return src.first;
    }
  }
  return -1;
}

TokenStream *WindISC::getStream(uint16_t id) {
  return this->sources[id].stream;
}

ParserReport *WindISC::getParserReport(uint16_t id) {
  return this->sources[id].parser_report;
}

std::string WindISC::getPath(uint16_t id) {
  return this->sources[id].path;
}

int WindISC::workOnInclude(std::string path) {
  WindLexer *lexer = TokenizeFile(path.c_str());
  if (lexer == nullptr) {
    return 1;
  }
  WindParser *parser = new WindParser(lexer->get(), path);
  Body *ast = parser->parse();
  this->volatile_ast = ast;
  return 0;
}

int WindISC::workOnImport(std::string path) {
  if (!std::filesystem::exists(path)) {
    return 1;
  }
  std::string dir_name = std::filesystem::path(path).filename().string();
  std::string interface_file = path + "/" + dir_name + ".wi";
  if (!std::filesystem::exists(interface_file)) {
    return 1;
  }
  std::string source_file = path + "/" + dir_name + ".w";
  this->imp_toprocess.push_back(source_file);
  if (this->workOnInclude(interface_file)) {
    return 1;
  }
  return 0;
}

Body *sumAST(Body *a, Body *b) {
  Body *result = new Body({});
  for (auto &child : a->get()) {
    *result += std::unique_ptr<ASTNode>(child.get());
  }
  for (auto &child : b->get()) {
    *result += std::unique_ptr<ASTNode>(child.get());
  }
  return result;
}

Body *WindISC::commitAST(Body *ast) {
  if (this->volatile_ast == nullptr) {
    return nullptr;
  }
  Body *resx = sumAST(ast, this->volatile_ast);
  this->volatile_ast = nullptr;
  return resx;
}


void InitISC() {
  global_isc = new WindISC();
}