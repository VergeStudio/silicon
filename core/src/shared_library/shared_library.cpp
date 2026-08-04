module;
#include <memory>

#include <string>

module silicon.library;

import silicon.exception;

namespace silicon::library {
void *SharedLibrary::GetSymbol(const std::string &symbol_name) {
    void *result = FindSymbol(symbol_name);
    if(result != nullptr) {
        return result;
    }

    throw silicon::exception::RuntimeError("[SharedLibrary::GetSymbol]: can't find symbol ", symbol_name);
}

bool SharedLibrary::HasSymbol(const std::string &symbol_name) {
    return FindSymbol(symbol_name) != nullptr;
}

std::string SharedLibrary::GetOSName(const std::string &name) {
    return Prefix() + name + Suffix();
}
} // namespace silicon::library

// module silicon.library;
// module;
