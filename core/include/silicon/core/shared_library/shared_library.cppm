module;
#include <memory>

#include <expected>
#include <string>
#include <system_error>

export module silicon.library;

namespace silicon::library {
export class shared_library final {

    /// The shared_library class dynamically
    /// loads shared libraries at Run-time.

  public:
    enum class flags {
        /// On platforms that use dlopen(), use RTLD_GLOBAL. This is the default
        /// if no flags are given.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        kShLibGlobal = 1,

        /// On platforms that use dlopen(), use RTLD_LOCAL instead of RTLD_GLOBAL.
        ///
        /// Note that if this flag is specified, RTTI (including dynamic_cast and throw) will
        /// not work for types defined in the shared library with GCC and possibly other
        /// compilers as well. See http://gcc.gnu.org/faq.html#dso for more information.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        kShLibLocal = 2
    };

  public:
    /// Creates a shared_library object.
    shared_library();

    /// Destroys the shared_library. The actual library
    /// remains loaded.
    virtual ~shared_library();

  public:
    /// Loads a shared library from the given path,
    /// using the given flags. See the flags enumeration
    /// for valid values.
    /// Returns an error_code on failure
    /// (silicon::error::library_error::kAlreadyLoaded / kLoadFailed, or a
    /// system errno via silicon::error::system_error).
    [[nodiscard]] auto load(const std::string &, int32_t flags = 0) -> std::expected<void, std::error_code>;

    /// Unloads a shared library. Returns an error_code on failure.
    [[nodiscard]] auto unload() -> std::expected<void, std::error_code>;

    /// Returns true iff a library has been loaded.
    [[nodiscard]] bool is_loaded() const;

    /// Returns true iff the loaded library contains
    /// a symbol with the given name.
    bool has_symbol(const std::string &);

    /// Returns the address of the symbol with
    /// the given name. For functions, this
    /// is the entry point of the function.
    /// Returns an error_code (kSymbolNotFound) if the symbol does not exist.
    [[nodiscard]] auto get_symbol(const std::string &) -> std::expected<void *, std::error_code>;

    /// Returns the path of the library, as
    /// specified in a call to load() or the
    /// constructor.
    const std::string &get_path() const;

    /// Returns the platform-specific filename prefix
    /// for shared libraries.
    /// Most platforms would return "lib" as prefix, while
    /// on Cygwin, the "cyg" prefix will be returned.
    static std::string prefix();

    /// Returns the platform-specific filename suffix
    /// for shared libraries (including the period).
    /// In debug mode, the suffix also includes a
    /// "d" to specify the debug version of a library.
    static std::string suffix();

    /// Returns the platform-specific filename
    /// for shared libraries by prefixing and suffixing name
    /// with prefix() and suffix()
    static std::string get_os_name(const std::string &);

  private:
    shared_library(const shared_library &) = delete;
    shared_library &operator=(const shared_library &) = delete;

    void *find_symbol(const std::string &);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

};

} // namespace silicon::library

// module silicon.library;
// module;