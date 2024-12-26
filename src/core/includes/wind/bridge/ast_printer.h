#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <wind/bridge/ast.h>

class ASTPrinter : public ASTVisitor {
public:
    void *visit(const BinaryExpr &node) override;
    void *visit(const VariableRef &node) override;
    void *visit(const VarAddressing &node) override;
    void *visit(const Literal &node) override;
    void *visit(const Return &node) override;
    void *visit(const Body &node) override;
    void *visit(const Function &node) override;
    void *visit(const ArgDecl &node) override;
    void *visit(const VariableDecl &node) override;
    void *visit(const GlobalDecl &node) override;
    void *visit(const FnCall &node) override;
    void *visit(const InlineAsm &node) override;
    void *visit(const StringLiteral &node) override;
    void *visit(const TypeDecl &node) override;
    void *visit(const Branching &node) override;
    void *visit(const Looping &node) override;

private:
    void print_tabs();
    unsigned tabs = 0;
    bool in_expr = false;
};

#endif // AST_PRINTER_H
