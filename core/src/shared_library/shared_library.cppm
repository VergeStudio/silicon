module;
#include <memory>

#include <mutex>
#include <string>

export module silicon.library;

namespace silicon::library {
export class SharedLibrary final {

    /// The SharedLibrary class dynamically
    /// loads shared libraries at Run-time.

  public:
    enum class Flags {
        /// On platforms that use dlopen(), use RTLD_GLOBAL. This is the default
        /// if no flags are given.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        ShLibGlobal = 1,

        /// On platforms that use dlopen(), use RTLD_LOCAL instead of RTLD_GLOBAL.
        ///
        /// Note that if this flag is specified, RTTI (including dynamic_cast and throw) will
        /// not work for types defined in the shared library with GCC and possibly other
        /// compilers as well. See http://gcc.gnu.org/faq.html#dso for more information.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        ShLibLocal = 2
    };

  public:
    /// Creates a SharedLibrary object.
    SharedLibrary();

    /// Destroys the SharedLibrary. The actual library
    /// remains loaded.
    virtual ~SharedLibrary() = default;

  public:
    /// Loads a shared library from the given path,
    /// using the given flags. See the Flags enumeration
    /// for valid values.
    /// Throws a LibraryAlreadyLoadedException if
    /// a library has already been loaded.
    /// Throws a LibraryLoadException if the library
    /// cannot be loaded.
    void Load(const std::string &, int32_t flags = 0);

    /// Unloads a shared library.
    void Unload();

    /// Returns true iff a library has been loaded.
    [[nodiscard]] bool IsLoaded() const;

    /// Returns true iff the loaded library contains
    /// a symbol with the given name.
    bool HasSymbol(const std::string &);

    /// Returns the address of the symbol with
    /// the given name. For functions, this
    /// is the entry point of the function.
    /// Throws a NotFoundException if the symbol
    /// does not exist.
    void *GetSymbol(const std::string &);

    /// Returns the path of the library, as
    /// specified in a call to load() or the
    /// constructor.
    const std::string &GetPath() const;

    /// Returns the platform-specific filename prefix
    /// for shared libraries.
    /// Most platforms would return "lib" as prefix, while
    /// on Cygwin, the "cyg" prefix will be returned.
    static std::string Prefix();

    /// Returns the platform-specific filename suffix
    /// for shared libraries (including the period).
    /// In debug mode, the suffix also includes a
    /// "d" to specify the debug version of a library.
    static std::string Suffix();

    /// Returns the platform-specific filename
    /// for shared libraries by prefixing and suffixing name
    /// with prefix() and suffix()
    static std::string GetOSName(const std::string &);

  private:
    SharedLibrary(const SharedLibrary &) = delete;
    SharedLibrary &operator=(const SharedLibrary &) = delete;

    void *findSymbol(const std::string &);

  private:
    struct P {
      public:
      std::string m_path;
      void *m_pHandle{nullptr};
      std::mutex m_mutex;
    };
    std::unique_ptr<P> m_p;

};

} // namespace silicon::library

// module silicon.library;
// module;