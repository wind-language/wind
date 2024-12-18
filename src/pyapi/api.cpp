#include <pybind11/pybind11.h>
#include <wind/isc/isc.h>
#include <wind/processing/lexer.h>
#include <wind/bridge/ast.h>
#include <wind/processing/parser.h>

namespace py = pybind11;


void initLib() {
  InitISC();
}

void newRun() {
  global_isc->tabulaRasa();
}

int litId=0;

WindLexer *TokenizeLiteral(std::string data) {
  data.push_back('\n');
  WindLexer *lex = new WindLexer(data.c_str());
  lex->tokenize();
  global_isc->setPath(lex->srcId, "literal"+std::to_string(litId++));
  global_isc->setStream(lex->srcId, lex->get());
  global_isc->newParserReport(lex->srcId, data);
  return lex;
}

class PyASTNode : public ASTNode {
public:
    using ASTNode::ASTNode; // Inherit constructors

    void* accept(ASTVisitor &visitor) const override {
        PYBIND11_OVERRIDE_PURE(
            void*,        // Return type
            ASTNode,      // Parent class
            accept,       // Name of function in C++ (must match Python name)
            visitor       // Argument(s)
        );
    }
};

PYBIND11_MODULE(libpyapi, module) {
  module.doc() = "Wind Compiler python API";

  py::class_<TokenPos>(module, "TokenPos")
    .def(py::init<u_int16_t, u_int16_t>());

  py::enum_<Token::Type>(module, "TokenType")
    .value("IDENTIFIER", Token::Type::IDENTIFIER)
    .value("INTEGER", Token::Type::INTEGER)
    .value("PLUS", Token::Type::PLUS)
    .value("MINUS", Token::Type::MINUS)
    .value("MULTIPLY", Token::Type::MULTIPLY)
    .value("DIVIDE", Token::Type::DIVIDE)
    .value("ASSIGN", Token::Type::ASSIGN)
    .value("LPAREN", Token::Type::LPAREN)
    .value("RPAREN", Token::Type::RPAREN)
    .value("COLON", Token::Type::COLON)
    .value("LBRACE", Token::Type::LBRACE)
    .value("RBRACE", Token::Type::RBRACE)
    .value("COMMA", Token::Type::COMMA)
    .value("SEMICOLON", Token::Type::SEMICOLON)
    .value("LBRACKET", Token::Type::LBRACKET)
    .value("RBRACKET", Token::Type::RBRACKET)
    .value("AT", Token::Type::AT)
    .value("QMARK", Token::Type::QMARK)
    .value("STRING", Token::Type::STRING)
    .value("VARDC", Token::Type::VARDC)
    .value("AND", Token::Type::AND)
    .export_values();  
  
  py::class_<Token>(module, "Token")
    .def(py::init<std::string, Token::Type, std::string, TokenRange, TokenSrcId>())
    .def_readonly("value", &Token::value)
    .def_readonly("type", &Token::type)
    .def_readonly("name", &Token::name)
    .def_readonly("range", &Token::range)
    .def_readonly("srcId", &Token::srcId);

  py::class_<TokenStream>(module, "TokenStream")
    .def(py::init<>())
    .def("pop", &TokenStream::pop);


  py::class_<WindLexer>(module, "WindLexer")
    .def(py::init<const char *>())
    .def("get", &WindLexer::get);

  module.def("TokenizeFile", &TokenizeFile, "A function that tokenizes a file");
  module.def("TokenizeLiteral", &TokenizeLiteral, "A function that tokenizes a literal");
  module.def("initLib", &initLib, "A function that initializes the Wind ISC");
  module.def("newRun", &newRun, "A function that clears the Wind ISC");
}
