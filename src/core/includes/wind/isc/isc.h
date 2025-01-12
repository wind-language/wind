#include <vector>
#include <string>
#include <wind/processing/lexer.h>
#include <wind/processing/token.h>
#include <wind/reporter/parser.h>
#include <wind/bridge/ast.h>
#include <map>
#include <algorithm>

#ifndef INTER_SOURCE_COMMUNICATION_H
#define INTER_SOURCE_COMMUNICATION_H

struct SourceDesc {
  std::string path;
  TokenStream *stream;
  ParserReport *parser_report;
};

class WindISC {
public:
  WindISC();
  void tabulaRasa() { this->sources.clear(); }
  uint16_t getNewSrcId() { return this->sources.size(); }
  void setPath(uint16_t id, std::string path);
  std::string getPath(uint16_t id);
  void setStream(uint16_t id, TokenStream *stream);
  void newParserReport(uint16_t id, std::string src);
  int16_t getSrcId(std::string path);
  TokenStream *getStream(uint16_t id);
  ParserReport *getParserReport(uint16_t id);

  int workOnInclude(std::string path);
  int workOnImport(std::string path);

  Body *commitAST(Body *ast);

  std::vector<std::string> getImports() { return this->imp_toprocess; }
  void popImport() { this->imp_toprocess.pop_back(); }
  
  void addLdFlag(std::string flag) {
    if (std::find(this->ld_user_flags.begin(), this->ld_user_flags.end(), flag) != this->ld_user_flags.end()) {
      return;
    }
    this->ld_user_flags.push_back(flag);
  }
  std::vector<std::string> getLdFlags() { return this->ld_user_flags; }
  
private:
  std::map<int, SourceDesc> sources;
  Body *volatile_ast;
  std::vector<std::string> imp_toprocess;
  std::vector<std::string> ld_user_flags;
};

extern WindISC *global_isc;

void InitISC();

#endif