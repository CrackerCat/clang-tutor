//==============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//
// USAGE:
//
// License: The Unlicense
//==============================================================================
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringMap.h"

using namespace clang;

//-----------------------------------------------------------------------------
// HelloWorld implementation
//-----------------------------------------------------------------------------
class HelloWorld : public RecursiveASTVisitor<HelloWorld> {
public:
  explicit HelloWorld(ASTContext *Context) : Context(Context) {}
  bool VisitCXXRecordDecl(CXXRecordDecl *Decl);

  llvm::StringMap<unsigned> getDeclMap() { return DeclMap; }

private:
  ASTContext *Context;
  // Map that contains the count of declaration in every input file
  llvm::StringMap<unsigned> DeclMap;
};

bool HelloWorld::VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
  FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getBeginLoc());

  // Basic sanity checking
  if (!FullLocation.isValid())
    return true;

  // There are 2 types of source locations: in a file or a macro expansion. The
  // latter contains the spelling location and the expansion location (both are
  // file locations), but only the latter is needed here (i.e. where the macro
  // is expanded). File locations are just that - file locations.
  if (FullLocation.isMacroID())
    FullLocation = FullLocation.getExpansionLoc();

  SourceManager& SrcMgr = Context->getSourceManager();
  const FileEntry* Entry = SrcMgr.getFileEntryForID(SrcMgr.getFileID(FullLocation));
  DeclMap[Entry->getName()]++;

  return true;
}

//-----------------------------------------------------------------------------
// ASTConsumer
//-----------------------------------------------------------------------------
class HelloWorldASTConsumer : public clang::ASTConsumer {
public:
  explicit HelloWorldASTConsumer(ASTContext *Ctx) : Visitor(Ctx) {}

  void HandleTranslationUnit(clang::ASTContext &Ctx) override {
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());

    if (Visitor.getDeclMap().empty()) {
      llvm::outs() << "(clang-tutor)  no declarations found " << "\n";
      return;
    }

    for (auto &Element : Visitor.getDeclMap()){
      llvm::outs() << "(clang-tutor)  file: " << Element.first() << "\n";
      llvm::outs() << "(clang-tutor)  count: " << Element.second << "\n";
    }
  }

private:
  HelloWorld Visitor;
};

//-----------------------------------------------------------------------------
// FrotendAction
//-----------------------------------------------------------------------------
class FindNamedClassAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef InFile) override {
    return std::unique_ptr<clang::ASTConsumer>(
        std::make_unique<HelloWorldASTConsumer>(&Compiler.getASTContext()));
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static FrontendPluginRegistry::Add<FindNamedClassAction>
    X(/*Name=*/"hello-world", /*Description=*/"The HelloWorld plugin");