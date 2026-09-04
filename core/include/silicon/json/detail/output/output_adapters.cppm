module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#ifndef JSON_NO_IO
#    include <ios>
#    include <ostream>
#endif

export module silicon.json:detail.output.output_adapters;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename CharType>
struct output_adapter_protocol {
    virtual void write_character(CharType c) = 0;
    virtual void write_characters(const CharType *s, std::size_t length) = 0;
    virtual ~output_adapter_protocol() = default;

    output_adapter_protocol() = default;
    output_adapter_protocol(const output_adapter_protocol &) = default;
    output_adapter_protocol(output_adapter_protocol &&) noexcept = default;
    output_adapter_protocol &operator=(const output_adapter_protocol &) = default;
    output_adapter_protocol &operator=(output_adapter_protocol &&) noexcept = default;
};

export template<typename CharType>
using output_adapter_t = std::shared_ptr<output_adapter_protocol<CharType>>;

export template<typename CharType, typename AllocatorType = std::allocator<CharType>>
class output_vector_adapter: public output_adapter_protocol<CharType> {
  public:
    explicit output_vector_adapter(std::vector<CharType, AllocatorType> &vec) noexcept
        : v(vec) {}

    void write_character(CharType c) override {
        v.push_back(c);
    }

    JSON_HEDLEY_NON_NULL(2)
    void write_characters(const CharType *s, std::size_t length) override {
        v.insert(v.end(), s, s + length);
    }

  private:
    std::vector<CharType, AllocatorType> &v;
};

#ifndef JSON_NO_IO

export template<typename CharType>
class output_stream_adapter: public output_adapter_protocol<CharType> {
  public:
    explicit output_stream_adapter(std::basic_ostream<CharType> &s) noexcept
        : stream(s) {}

    void write_character(CharType c) override {
        stream.put(c);
    }

    JSON_HEDLEY_NON_NULL(2)
    void write_characters(const CharType *s, std::size_t length) override {
        stream.write(s, static_cast<std::streamsize>(length));
    }

  private:
    std::basic_ostream<CharType> &stream;
};
#endif

export template<typename CharType, typename StringType = std::basic_string<CharType>>
class output_string_adapter: public output_adapter_protocol<CharType> {
  public:
    explicit output_string_adapter(StringType &s) noexcept
        : str(s) {}

    void write_character(CharType c) override {
        str.push_back(c);
    }

    JSON_HEDLEY_NON_NULL(2)
    void write_characters(const CharType *s, std::size_t length) override {
        str.append(s, length);
    }

  private:
    StringType &str;
};

export template<typename CharType, typename StringType = std::basic_string<CharType>>
class output_adapter {
  public:
    template<typename AllocatorType = std::allocator<CharType>>
    output_adapter(std::vector<CharType, AllocatorType> &vec)
        : oa(std::make_shared<output_vector_adapter<CharType, AllocatorType>>(vec)) {}

#ifndef JSON_NO_IO
    output_adapter(std::basic_ostream<CharType> &s)
        : oa(std::make_shared<output_stream_adapter<CharType>>(s)) {}
#endif

    output_adapter(StringType &s)
        : oa(std::make_shared<output_string_adapter<CharType, StringType>>(s)) {}

    operator output_adapter_t<CharType>() {
        return oa;
    }

  private:
    output_adapter_t<CharType> oa = nullptr;
};

}
SILICON_JSON_NAMESPACE_END
