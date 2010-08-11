/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cpplocalsymbols.h"
#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>
#include <ASTVisitor.h>
#include <AST.h>
#include <Scope.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Literals.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;

namespace {

class FindLocalSymbols: protected ASTVisitor
{
    Scope *_functionScope;
    Document::Ptr _doc;

public:
    FindLocalSymbols(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()), _doc(doc), hasD(false), hasQ(false)
    { }

    // local and external uses.
    SemanticInfo::LocalUseMap localUses;
    bool hasD;
    bool hasQ;

    void operator()(DeclarationAST *ast)
    {
        localUses.clear();

        if (!ast)
            return;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            if (def->symbol) {
                _functionScope = def->symbol;
                accept(ast);
            }
        } else if (ObjCMethodDeclarationAST *decl = ast->asObjCMethodDeclaration()) {
            if (decl->method_prototype->symbol) {
                _functionScope = decl->method_prototype->symbol;
                accept(ast);
            }
        }
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    void enterScope(Scope *scope)
    {
        _scopeStack.append(scope);

        for (unsigned i = 0; i < scope->memberCount(); ++i) {
            if (Symbol *member = scope->memberAt(i)) {
                if (! member->isGenerated() && (member->isDeclaration() || member->isArgument())) {
                    if (member->name() && member->name()->isNameId()) {
                        const Identifier *id = member->identifier();
                        unsigned line, column;
                        getTokenStartPosition(member->sourceLocation(), &line, &column);
                        localUses[member].append(SemanticInfo::Use(line, column, id->size(), SemanticInfo::Use::Local));
                    }
                }
            }
        }
    }

    virtual bool visit(IdExpressionAST *ast)
    {
        if (SimpleNameAST *simpleName = ast->name->asSimpleName()) {
            const Identifier *id = identifier(simpleName->identifier_token);
            for (int i = _scopeStack.size() - 1; i != -1; --i) {
                if (Symbol *member = _scopeStack.at(i)->find(id)) {
                    if (!member->isGenerated() && (member->sourceLocation() < ast->firstToken() || member->scope()->isFunction())) {
                        unsigned line, column;
                        getTokenStartPosition(simpleName->identifier_token, &line, &column);
                        localUses[member].append(SemanticInfo::Use(line, column, id->size(), SemanticInfo::Use::Local));
                        return false;
                    }
                }
            }
        }

        return true;
    }

    virtual bool visit(QtMemberDeclarationAST *ast)
    {
        if (tokenKind(ast->q_token) == T_Q_D)
            hasD = true;
        else
            hasQ = true;

        return true;
    }

    virtual bool visit(FunctionDefinitionAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(FunctionDefinitionAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(CompoundStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(CompoundStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(IfStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(IfStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(WhileStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(WhileStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ForStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(ForStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ForeachStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(ForeachStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(SwitchStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(SwitchStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(CatchClauseAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(CatchClauseAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ExpressionOrDeclarationStatementAST *ast)
    {
        accept(ast->declaration);
        return false;
    }

private:
    QList<Scope *> _scopeStack;
};

} // end of anonymous namespace


LocalSymbols::LocalSymbols(CPlusPlus::Document::Ptr doc, CPlusPlus::DeclarationAST *ast)
{
    FindLocalSymbols FindLocalSymbols(doc);
    FindLocalSymbols(ast);
    hasD = FindLocalSymbols.hasD;
    hasQ = FindLocalSymbols.hasQ;
    uses = FindLocalSymbols.localUses;
}
