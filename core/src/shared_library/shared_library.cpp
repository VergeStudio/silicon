module;
#include <memory>

#include <string>

module silicon.library;

import silicon.exception;

namespace silicon::library {
void *SharedLibrary::GetSymbol(const std::string &rSymbolName) {
    void *pResult = findSymbol(rSymbolName);
    if(pResult != nullptr) {
        return pResult;
    }

    throw silicon::exception::RuntimeError("[SharedLibrary::GetSymbol]: can't find symbol ", rSymbolName);
}

bool SharedLibrary::HasSymbol(const std::string &rSymbolName) {
    return findSymbol(rSymbolName) != nullptr;
}

std::string SharedLibrary::GetOSName(const std::string &rName) {
    return Prefix() + rName + Suffix();
}
} // namespace silicon::library

// module silicon.library;
// module;
