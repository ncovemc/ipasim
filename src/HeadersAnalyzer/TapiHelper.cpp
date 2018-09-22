// TapiHelper.cpp

#include "TapiHelper.hpp"

#include "ErrorReporting.hpp"

#include <clang/Basic/FileSystemOptions.h>

using namespace clang;
using namespace llvm;
using namespace std;
using namespace tapi::internal;

TBDHandler::TBDHandler(HAContext &HAC)
    : HAC(HAC), FM(FileSystemOptions()), IFM(FM) {}

void TBDHandler::HandleFile(const string &Path) {
  bool HasTBDExtension = filesystem::path(Path).extension() == ".tbd";

  // Check file.
  auto FileOrError = IFM.readFile(Path);
  if (!FileOrError) {
    // If the file hasn't `.tbd` extension, it's OK that we cannot read it.
    if (HasTBDExtension)
      reportError(Twine(toString(FileOrError.takeError())) + " (" + Path + ")");
    else
      consumeError(FileOrError.takeError());
    return;
  }
  // If we can read it and it hasn't `.tbd` extensions, well, that's weird.
  if (!HasTBDExtension)
    reportWarning(Twine("TBD file without `.tbd` extension (") + Path + ")");
  InterfaceFileBase *FileBase = *FileOrError;
  // TODO: Shouldn't this be `armv7s`?
  if (!FileBase->getArchitectures().contains(Architecture::armv7)) {
    reportError(Twine("TBD file does not contain architecture ARMv7 (") + Path +
                ")");
    return;
  }
  auto *File = dynamic_cast<InterfaceFile *>(FileBase);
  if (!File) {
    reportError(Twine("interface file expected (") + Path + ")");
    return;
  }

  // Save the Dylib.
  auto InsertPair(HAC.iOSLibs.insert({File->getInstallName()}));
  if (!InsertPair.second) {
    // Ignore Dylibs with already-found install name, the corresponding TBD
    // files should be identical.
    return;
  }
  DylibPtr Lib(InsertPair.first);

  // Find exports.
  for (Symbol *Sym : File->exports()) {
    // Determine symbol name.
    string Name;
    switch (Sym->getKind()) {
    case SymbolKind::ObjectiveCClass: {
      // Save class.
      auto Class = HAC.iOSClasses.insert(Sym->getName().str()).first;
      Class->Dylibs.push_back(Lib);
      continue;
    }
    case SymbolKind::ObjectiveCInstanceVariable:
    case SymbolKind::ObjectiveCClassEHType:
      // Skip `ObjectiveC*` symbols, since they aren't functions.
      break;
    case SymbolKind::GlobalSymbol:
      Name = Sym->getName();
      break;
    default:
      reportError(Twine("unrecognized symbol type (") +
                  Sym->getAnnotatedName() + ")");
      continue;
    }

    // Save export.
    ExportPtr Exp = HAC.iOSExps.find(Name);
    if (Exp != HAC.iOSExps.end())
      Lib->Exports.push_back(Exp);
    else
      Lib->Exports.push_back(HAC.addExport(move(Name)));
  }
}