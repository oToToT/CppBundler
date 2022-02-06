#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

class CppExpanderCallback : public clang::PPCallbacks {
public:
  CppExpanderCallback(clang::Rewriter &R) : TheRewriter(R) {
    BeginLoc.emplace_back(clang::SourceLocation(), clang::FileID());
  }
  void InclusionDirective(clang::SourceLocation HashLoc,
                          const clang::Token &IncludeTok,
                          llvm::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange,
                          const clang::FileEntry *File,
                          llvm::StringRef SearchPath,
                          llvm::StringRef RelativePath,
                          const clang::Module *Imported,
                          clang::SrcMgr::CharacteristicKind FileType) final {
    if (FileType != clang::SrcMgr::C_User) {
      return;
   }

    BeginLoc.back().first = HashLoc;
  }

  void FileChanged(clang::SourceLocation Loc,
                   clang::PPCallbacks::FileChangeReason Reason,
                   clang::SrcMgr::CharacteristicKind FileType,
                   clang::FileID PrevFID) final {
    if (FileType != clang::SrcMgr::C_User) {
      return;
    }

    auto &SM = TheRewriter.getSourceMgr();

    if (Reason == clang::PPCallbacks::EnterFile) {
      if (BeginLoc.back().first.isValid()) {
        BeginLoc.back().second = SM.getFileID(Loc);
      }
      BeginLoc.emplace_back(clang::SourceLocation(), clang::FileID());
    } else if (Reason == clang::PPCallbacks::ExitFile) {
      auto BLoc = BeginLoc.back().first;
      auto FID = BeginLoc.back().second;
      if (BLoc.isValid() and FID.isValid()) {
        clang::SourceRange InclusionRange(BLoc, Loc);
        
        const auto &IncludedBuffer = TheRewriter.getEditBuffer(FID);
        std::string IncludedContent;
        llvm::raw_string_ostream IncludedStream(IncludedContent);
        IncludedBuffer.write(IncludedStream).flush();

        TheRewriter.ReplaceText(InclusionRange, IncludedContent);
      }
      BeginLoc.pop_back();
    }
  }

private:
  clang::Rewriter &TheRewriter;
  llvm::SmallVector<std::pair<clang::SourceLocation, clang::FileID>, 32> BeginLoc;
};

class CppExpanderAction : public clang::PreprocessOnlyAction {
protected:
  virtual void ExecuteAction() {
    const auto &CI = getCompilerInstance();
    auto &SM = CI.getSourceManager();
    auto &PP = CI.getPreprocessor();

    clang::Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SM, CI.getLangOpts());
    PP.addPPCallbacks(std::make_unique<CppExpanderCallback>(TheRewriter));

    clang::PreprocessOnlyAction::ExecuteAction();

    TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
  }
};

static inline void printHelpMessage() {
  llvm::outs() << "Usage: cpp-expander FILE [OPTIONS]...\n";
}

int main(int argc, const char *argv[]) {
  if (argc <= 1) {
    printHelpMessage();
    return 0;
  }

  std::vector<std::string> Sources, Args;
  Sources.emplace_back(argv[1]);
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--help") {
      printHelpMessage();
      return 0;
    } else if (i >= 2) {
      Args.emplace_back(argv[i]);
    }
  }

  clang::tooling::FixedCompilationDatabase CompileDb(".", std::move(Args));
  clang::tooling::ClangTool Tool(CompileDb, Sources);
  return Tool.run(clang::tooling::newFrontendActionFactory<CppExpanderAction>().get());
}
