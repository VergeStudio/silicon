module;
#include <memory>

#include <expected>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.library;
export import silicon.library.error;

namespace silicon::library {
export class CORE_API shared_library final {




  public:
    enum class flags {




        kShLibGlobal = 1,








        kShLibLocal = 2
    };

  public:

    shared_library();



    virtual ~shared_library();

  public:






    [[nodiscard]] std::expected<void, std::error_code> load(const std::string &, int32_t = 0) ;


    [[nodiscard]] std::expected<void, std::error_code> unload() ;


    [[nodiscard]] bool is_loaded() const;



    bool has_symbol(const std::string &);





    [[nodiscard]] std::expected<void *, std::error_code> get_symbol(const std::string &) ;




    const std::string &get_path() const;





    static std::string prefix();





    static std::string suffix();




    static std::string get_os_name(const std::string &);

  private:
    shared_library(const shared_library &) = delete;
    shared_library &operator=(const shared_library &) = delete;

    void *find_symbol(const std::string &);

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

};


}


