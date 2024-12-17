#include <vector>
#include <string>
#include <wind/processing/lexer.h>
#include <wind/processing/token.h>
#include <wind/reporter/parser.h>
#include <wind/bridge/ast.h>
#include <map>

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

  Body *commitAST(Body *ast);
  
private:
  std::map<int, SourceDesc> sources;
  Body *volatile_ast;
};

extern WindISC *global_isc;

void InitISC();

#endif