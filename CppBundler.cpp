#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

class CppExpanderCallback : public clang::PPCallbacks {
public:
  CppExpanderCallback(clang::Rewriter &R) : TheRewriter(R) {
    InclusionSourceRanges.push_back({});
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

    InclusionSourceRanges.emplace_back(HashLoc, FilenameRange.getEnd());
  }

  void FileSkipped(const clang::FileEntryRef &SkippedFile,
                   const clang::Token &FilenameTok,
                   clang::SrcMgr::CharacteristicKind FileType) final {
    if (FileType != clang::SrcMgr::C_User) {
      return;
    }

    TheRewriter.RemoveText(InclusionSourceRanges.back());
    InclusionSourceRanges.pop_back();
  }

  void FileChanged(clang::SourceLocation Loc,
                   clang::PPCallbacks::FileChangeReason Reason,
                   clang::SrcMgr::CharacteristicKind FileType,
                   clang::FileID PrevFID) final {
    if (FileType != clang::SrcMgr::C_User) {
      return;
    }
    if (Reason != clang::PPCallbacks::ExitFile) {
      return;
    }

    auto &SM = TheRewriter.getSourceMgr();
    if (SM.isWrittenInBuiltinFile(Loc)) {
      return;
    }

    auto IncludeFileLoc = SM.getLocForStartOfFile(PrevFID);
    if (IncludeFileLoc.isInvalid()) {
      return;
    }
    if (SM.getFileCharacteristic(IncludeFileLoc) != clang::SrcMgr::C_User) {
      return;
    }

    const auto &IncludedBuffer = TheRewriter.getEditBuffer(PrevFID);
    std::string IncludedContent;
    llvm::raw_string_ostream IncludedStream(IncludedContent);
    IncludedBuffer.write(IncludedStream).flush();

    TheRewriter.ReplaceText(InclusionSourceRanges.back(), IncludedContent);
    InclusionSourceRanges.pop_back();
  }

private:
  clang::Rewriter &TheRewriter;
  llvm::SmallVector<clang::SourceRange, 32> InclusionSourceRanges;
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
  llvm::outs() << "Usage: cpp-bundle FILE [OPTIONS]...\n";
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
  return Tool.run(
      clang::tooling::newFrontendActionFactory<CppExpanderAction>().get());
}
