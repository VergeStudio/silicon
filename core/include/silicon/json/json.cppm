module;






#pragma warning(disable : 5202)























#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#ifndef JSON_NO_IO
#    include <iosfwd>
#endif
#include <iterator>
#include <memory>



#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <string>
#include <utility>
#include <vector>

#if defined(JSON_HAS_CPP_17)
#    if JSON_HAS_STATIC_RTTI
#        include <any>
#    endif
#    include <string_view>
#endif

export module silicon.json;





import std;
export import :json_fwd;
export import :adl_serializer;
export import :byte_container_with_subtype;
export import :ordered_map;
export import :detail.exceptions;
export import :detail.hash;
export import :detail.json_custom_base_class;
export import :detail.json_pointer;
export import :detail.json_ref;
export import :detail.string_concat;
export import :detail.string_escape;
export import :detail.value_t;
export import :detail.conversions.from_json;
export import :detail.conversions.to_chars;
export import :detail.conversions.to_json;
export import :detail.input.binary_reader;
export import :detail.input.input_adapters;
export import :detail.input.json_sax;
export import :detail.input.lexer;
export import :detail.input.parser;
export import :detail.input.position_t;
export import :detail.output.binary_writer;
export import :detail.output.output_adapters;
export import :detail.output.serializer;
export import :detail.meta.cpp_future;
export import :detail.meta.detected;
export import :detail.meta.identity_tag;
export import :detail.meta.is_sax;
export import :detail.meta.std_fs;
export import :detail.meta.type_traits;
export import :detail.meta.void_t;
export import :detail.meta.call_std.begin;
export import :detail.meta.call_std.end;
export import :detail.iterators.internal_iterator;
export import :detail.iterators.iter_impl;
export import :detail.iterators.iteration_proxy;
export import :detail.iterators.iterator_traits;
export import :detail.iterators.json_reverse_iterator;
export import :detail.iterators.primitive_iterator;



export {
SILICON_JSON_NAMESPACE_BEGIN


silicon_BASIC_JSON_TPL_DECLARATION class basic_json
    : public ::silicon::json::impl::detail::json_base_class<CustomBaseClass> {
  private:
    template<detail::value_t>
    friend struct detail::external_constructor;

    template<typename>
    friend class json_pointer;



    template<typename BasicJsonType, typename InputType>
    friend class detail::parser;
    friend detail::serializer<basic_json>;
    template<typename BasicJsonType>
    friend class detail::iter_impl;
    template<typename BasicJsonType, typename CharType>
    friend class detail::binary_writer;
    template<typename BasicJsonType, typename InputType, typename SAX>
    friend class detail::binary_reader;
    template<typename BasicJsonType>
    friend class detail::json_sax_dom_parser;
    template<typename BasicJsonType>
    friend class detail::json_sax_dom_callback_parser;
    friend class detail::exception;


    using basic_json_t = silicon_BASIC_JSON_TPL;
    using json_base_class_t = ::silicon::json::impl::detail::json_base_class<CustomBaseClass>;

    JSON_PRIVATE_UNLESS_TESTED:

        using lexer = ::silicon::json::impl::detail::lexer_base<basic_json>;

    template<typename InputAdapterType>
    static ::silicon::json::impl::detail::parser<basic_json, InputAdapterType> parser(
            InputAdapterType adapter,
            detail::parser_callback_t<basic_json> cb = nullptr,
            const bool allow_exceptions = true,
            const bool ignore_comments = false
    ) {
        return ::silicon::json::impl::detail::parser<basic_json, InputAdapterType>(std::move(adapter), std::move(cb), allow_exceptions, ignore_comments);
    }

  private:
    using primitive_iterator_t = ::silicon::json::impl::detail::primitive_iterator_t;
    template<typename BasicJsonType>
    using internal_iterator = ::silicon::json::impl::detail::internal_iterator<BasicJsonType>;
    template<typename BasicJsonType>
    using iter_impl = ::silicon::json::impl::detail::iter_impl<BasicJsonType>;
    template<typename Iterator>
    using iteration_proxy = ::silicon::json::impl::detail::iteration_proxy<Iterator>;
    template<typename Base>
    using json_reverse_iterator = ::silicon::json::impl::detail::json_reverse_iterator<Base>;

    template<typename CharType>
    using output_adapter_t = ::silicon::json::impl::detail::output_adapter_t<CharType>;

    template<typename InputType>
    using binary_reader = ::silicon::json::impl::detail::binary_reader<basic_json, InputType>;
    template<typename CharType>
    using binary_writer = ::silicon::json::impl::detail::binary_writer<basic_json, CharType>;

    JSON_PRIVATE_UNLESS_TESTED: using serializer = ::silicon::json::impl::detail::serializer<basic_json>;

  public:
    using value_t = detail::value_t;

    using json_pointer = ::silicon::json::impl::json_pointer<StringType>;
    template<typename T, typename SFINAE>
    using json_serializer = JSONSerializer<T, SFINAE>;

    using error_handler_t = detail::error_handler_t;

    using cbor_tag_handler_t = detail::cbor_tag_handler_t;

    using initializer_list_t = std::initializer_list<detail::json_ref<basic_json>>;

    using input_format_t = detail::input_format_t;

    using json_sax_t = json_sax<basic_json>;









    using exception = detail::exception;
    using parse_error = detail::parse_error;
    using invalid_iterator = detail::invalid_iterator;
    using type_error = detail::type_error;
    using out_of_range = detail::out_of_range;
    using other_error = detail::other_error;













    using value_type = basic_json;


    using reference = value_type &;

    using const_reference = const value_type &;


    using difference_type = std::ptrdiff_t;

    using size_type = std::size_t;


    using allocator_type = AllocatorType<basic_json>;


    using pointer = typename std::allocator_traits<allocator_type>::pointer;

    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;


    using iterator = iter_impl<basic_json>;

    using const_iterator = iter_impl<const basic_json>;

    using reverse_iterator = json_reverse_iterator<typename basic_json::iterator>;

    using const_reverse_iterator = json_reverse_iterator<typename basic_json::const_iterator>;





    static allocator_type get_allocator() {
        return allocator_type();
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json meta() {
        basic_json result;

        result["copyright"] = "(C) silicon contributors";
        result["name"] = "silicon JSON";
        result["url"] = "https://github.com/VergeStudio/silicon";
        result["version"]["string"] =
                detail::concat(std::to_string(SILICON_JSON_VERSION_MAJOR), '.', std::to_string(SILICON_JSON_VERSION_MINOR), '.', std::to_string(SILICON_JSON_VERSION_PATCH));
        result["version"]["major"] = SILICON_JSON_VERSION_MAJOR;
        result["version"]["minor"] = SILICON_JSON_VERSION_MINOR;
        result["version"]["patch"] = SILICON_JSON_VERSION_PATCH;

#ifdef _WIN32
        result["platform"] = "win32";
#elif defined __linux__
        result["platform"] = "linux";
#elif defined __APPLE__
        result["platform"] = "apple";
#elif defined __unix__
        result["platform"] = "unix";
#else
        result["platform"] = "unknown";
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
        result["compiler"] = {{"family", "icc"}, { "version",
                                                   __INTEL_COMPILER }};
#elif defined(__clang__)
        result["compiler"] = {{"family", "clang"}, { "version",
                                                     __clang_version__ }};
#elif defined(__GNUC__) || defined(__GNUG__)
        result["compiler"] = {{"family", "gcc"}, { "version",
                                                   detail::concat(
                                                           std::to_string(__GNUC__), '.',
                                                           std::to_string(__GNUC_MINOR__), '.',
                                                           std::to_string(__GNUC_PATCHLEVEL__)
                                                   )
                              }};
#elif defined(__HP_cc) || defined(__HP_aCC)
        result["compiler"] = "hp"
#elif defined(__IBMCPP__)
        result["compiler"] = {{"family", "ilecpp"}, { "version",
                                                      __IBMCPP__ }};
#elif defined(_MSC_VER)
        result["compiler"] = {{"family", "msvc"}, { "version",
                                                    _MSC_VER }};
#elif defined(__PGI)
        result["compiler"] = {{"family", "pgcpp"}, { "version",
                                                     __PGI }};
#elif defined(__SUNPRO_CC)
        result["compiler"] = {{"family", "sunpro"}, { "version",
                                                      __SUNPRO_CC }};
#else
        result["compiler"] = {{"family", "unknown"}, {"version", "unknown"}};
#endif

#if defined(_MSVC_LANG)
        result["compiler"]["c++"] = std::to_string(_MSVC_LANG);
#elif defined(__cplusplus)
        result["compiler"]["c++"] = std::to_string(__cplusplus);
#else
        result["compiler"]["c++"] = "unknown";
#endif
        return result;
    }














#if defined(JSON_HAS_CPP_14)


    using default_object_comparator_t = std::less<>;
#else
    using default_object_comparator_t = std::less<StringType>;
#endif



    using object_t = ObjectType<StringType, basic_json, default_object_comparator_t, AllocatorType<std::pair<const StringType, basic_json>>>;



    using array_t = ArrayType<basic_json, AllocatorType<basic_json>>;



    using string_t = StringType;



    using boolean_t = BooleanType;



    using number_integer_t = NumberIntegerType;



    using number_unsigned_t = NumberUnsignedType;



    using number_float_t = NumberFloatType;



    using binary_t = silicon::json::impl::byte_container_with_subtype<BinaryType>;



    using object_comparator_t = detail::actual_object_comparator_t<basic_json>;



  private:

    template<typename T, typename... Args>
    JSON_HEDLEY_RETURNS_NON_NULL static T *create(Args &&...args) {
        AllocatorType<T> alloc;
        using AllocatorTraits = std::allocator_traits<AllocatorType<T>>;

        auto deleter = [&](T *obj) {
            AllocatorTraits::deallocate(alloc, obj, 1);
        };
        std::unique_ptr<T, decltype(deleter)> obj(AllocatorTraits::allocate(alloc, 1), deleter);
        AllocatorTraits::construct(alloc, obj.get(), std::forward<Args>(args)...);
        JSON_ASSERT(obj != nullptr);
        return obj.release();
    }





    JSON_PRIVATE_UNLESS_TESTED:
        
        union json_value {

        object_t *object;

        array_t *array;

        string_t *string;

        binary_t *binary;

        boolean_t boolean;

        number_integer_t number_integer;

        number_unsigned_t number_unsigned;

        number_float_t number_float;


        json_value() = default;

        json_value(boolean_t v) noexcept: boolean(v) {}

        json_value(number_integer_t v) noexcept: number_integer(v) {}

        json_value(number_unsigned_t v) noexcept: number_unsigned(v) {}

        json_value(number_float_t v) noexcept: number_float(v) {}

        json_value(value_t t) {
            switch(t) {
                case value_t::object: {
                    object = create<object_t>();
                    break;
                }

                case value_t::array: {
                    array = create<array_t>();
                    break;
                }

                case value_t::string: {
                    string = create<string_t>("");
                    break;
                }

                case value_t::binary: {
                    binary = create<binary_t>();
                    break;
                }

                case value_t::boolean: {
                    boolean = static_cast<boolean_t>(false);
                    break;
                }

                case value_t::number_integer: {
                    number_integer = static_cast<number_integer_t>(0);
                    break;
                }

                case value_t::number_unsigned: {
                    number_unsigned = static_cast<number_unsigned_t>(0);
                    break;
                }

                case value_t::number_float: {
                    number_float = static_cast<number_float_t>(0.0);
                    break;
                }

                case value_t::null: {
                    object = nullptr;
                    break;
                }

                case value_t::discarded:
                default: {
                    object = nullptr;
                    if(JSON_HEDLEY_UNLIKELY(t == value_t::null)) {
                        JSON_THROW(other_error::create(500, "961c151d2e87f2686a955a9be24d316f1362bf21 3.11.3", nullptr));
                    }
                    break;
                }
            }
        }


        json_value(const string_t &value): string(create<string_t>(value)) {}


        json_value(string_t &&value): string(create<string_t>(std::move(value))) {}


        json_value(const object_t &value): object(create<object_t>(value)) {}


        json_value(object_t &&value): object(create<object_t>(std::move(value))) {}


        json_value(const array_t &value): array(create<array_t>(value)) {}


        json_value(array_t &&value): array(create<array_t>(std::move(value))) {}


        json_value(const typename binary_t::container_type &value): binary(create<binary_t>(value)) {}


        json_value(typename binary_t::container_type &&value): binary(create<binary_t>(std::move(value))) {}


        json_value(const binary_t &value): binary(create<binary_t>(value)) {}


        json_value(binary_t &&value): binary(create<binary_t>(std::move(value))) {}

        void destroy(value_t t) {
            if(
                    (t == value_t::object && object == nullptr) ||
                    (t == value_t::array && array == nullptr) ||
                    (t == value_t::string && string == nullptr) ||
                    (t == value_t::binary && binary == nullptr)
            ) {

                return;
            }
            if(t == value_t::array || t == value_t::object) {

                std::vector<basic_json> stack;


                if(t == value_t::array) {
                    stack.reserve(array->size());
                    std::move(array->begin(), array->end(), std::back_inserter(stack));
                } else {
                    stack.reserve(object->size());
                    for(auto &&it: *object) {
                        stack.push_back(std::move(it.second));
                    }
                }

                while(!stack.empty()) {

                    basic_json current_item(std::move(stack.back()));
                    stack.pop_back();



                    if(current_item.is_array()) {
                        std::move(current_item.m_data.m_value.array->begin(), current_item.m_data.m_value.array->end(), std::back_inserter(stack));

                        current_item.m_data.m_value.array->clear();
                    } else if(current_item.is_object()) {
                        for(auto &&it: *current_item.m_data.m_value.object) {
                            stack.push_back(std::move(it.second));
                        }

                        current_item.m_data.m_value.object->clear();
                    }



                }
            }

            switch(t) {
                case value_t::object: {
                    AllocatorType<object_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, object);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, object, 1);
                    break;
                }

                case value_t::array: {
                    AllocatorType<array_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, array);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, array, 1);
                    break;
                }

                case value_t::string: {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, string, 1);
                    break;
                }

                case value_t::binary: {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, binary, 1);
                    break;
                }

                case value_t::null:
                case value_t::boolean:
                case value_t::number_integer:
                case value_t::number_unsigned:
                case value_t::number_float:
                case value_t::discarded:
                default: {
                    break;
                }
            }
        }
    };

  private:
    
    void assert_invariant(bool check_parents = true) const noexcept {
        JSON_ASSERT(m_data.m_type != value_t::object || m_data.m_value.object != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::array || m_data.m_value.array != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::string || m_data.m_value.string != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::binary || m_data.m_value.binary != nullptr);

#if JSON_DIAGNOSTICS
        JSON_TRY {

            JSON_ASSERT(!check_parents || !is_structured() || std::all_of(begin(), end(), [this](const basic_json &j) {
                return j.m_parent == this;
            }));
        }
        JSON_CATCH(...) {}
#endif
        static_cast<void>(check_parents);
    }

    void set_parents() {
#if JSON_DIAGNOSTICS
        switch(m_data.m_type) {
            case value_t::array: {
                for(auto &element: *m_data.m_value.array) {
                    element.m_parent = this;
                }
                break;
            }

            case value_t::object: {
                for(auto &element: *m_data.m_value.object) {
                    element.second.m_parent = this;
                }
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                break;
        }
#endif
    }

    iterator set_parents(iterator it, typename iterator::difference_type count_set_parents) {
#if JSON_DIAGNOSTICS
        for(typename iterator::difference_type i = 0; i < count_set_parents; ++i) {
            (it + i)->m_parent = this;
        }
#else
        static_cast<void>(count_set_parents);
#endif
        return it;
    }

    reference set_parent(reference j, std::size_t old_capacity = static_cast<std::size_t>(-1)) {
#if JSON_DIAGNOSTICS
        if(old_capacity != static_cast<std::size_t>(-1)) {

            JSON_ASSERT(type() == value_t::array);
            if(JSON_HEDLEY_UNLIKELY(m_data.m_value.array->capacity() != old_capacity)) {

                set_parents();
                return j;
            }
        }



#    ifdef JSON_HEDLEY_MSVC_VERSION
#        pragma warning(push)
#        pragma warning(disable : 4127)
#    endif
        if(detail::is_ordered_map<object_t>::value) {
            set_parents();
            return j;
        }
#    ifdef JSON_HEDLEY_MSVC_VERSION
#        pragma warning(pop)
#    endif

        j.m_parent = this;
#else
        static_cast<void>(j);
        static_cast<void>(old_capacity);
#endif
        return j;
    }

  public:






    using parse_event_t = detail::parse_event_t;



    using parser_callback_t = detail::parser_callback_t<basic_json>;












    basic_json(const value_t v)
        : m_data(v) {
        assert_invariant();
    }



    basic_json(std::nullptr_t = nullptr) noexcept
        : basic_json(value_t::null) {
        assert_invariant();
    }



    template<typename CompatibleType, typename U = detail::uncvref_t<CompatibleType>, detail::enable_if_t<!detail::is_basic_json<U>::value && detail::is_compatible_type<basic_json_t, U>::value, int> = 0>
    basic_json(CompatibleType &&val) noexcept(noexcept(
            JSONSerializer<U>::to_json(std::declval<basic_json_t &>(), std::forward<CompatibleType>(val))
    )) {
        JSONSerializer<U>::to_json(*this, std::forward<CompatibleType>(val));
        set_parents();
        assert_invariant();
    }



    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value && !std::is_same<basic_json, BasicJsonType>::value, int> = 0>
    basic_json(const BasicJsonType &val) {
        using other_boolean_t = typename BasicJsonType::boolean_t;
        using other_number_float_t = typename BasicJsonType::number_float_t;
        using other_number_integer_t = typename BasicJsonType::number_integer_t;
        using other_number_unsigned_t = typename BasicJsonType::number_unsigned_t;
        using other_string_t = typename BasicJsonType::string_t;
        using other_object_t = typename BasicJsonType::object_t;
        using other_array_t = typename BasicJsonType::array_t;
        using other_binary_t = typename BasicJsonType::binary_t;

        switch(val.type()) {
            case value_t::boolean:
                JSONSerializer<other_boolean_t>::to_json(*this, val.template get<other_boolean_t>());
                break;
            case value_t::number_float:
                JSONSerializer<other_number_float_t>::to_json(*this, val.template get<other_number_float_t>());
                break;
            case value_t::number_integer:
                JSONSerializer<other_number_integer_t>::to_json(*this, val.template get<other_number_integer_t>());
                break;
            case value_t::number_unsigned:
                JSONSerializer<other_number_unsigned_t>::to_json(*this, val.template get<other_number_unsigned_t>());
                break;
            case value_t::string:
                JSONSerializer<other_string_t>::to_json(*this, val.template get_ref<const other_string_t &>());
                break;
            case value_t::object:
                JSONSerializer<other_object_t>::to_json(*this, val.template get_ref<const other_object_t &>());
                break;
            case value_t::array:
                JSONSerializer<other_array_t>::to_json(*this, val.template get_ref<const other_array_t &>());
                break;
            case value_t::binary:
                JSONSerializer<other_binary_t>::to_json(*this, val.template get_ref<const other_binary_t &>());
                break;
            case value_t::null:
                *this = nullptr;
                break;
            case value_t::discarded:
                m_data.m_type = value_t::discarded;
                break;
            default:
                JSON_ASSERT(false);
        }
        JSON_ASSERT(m_data.m_type == val.type());
        set_parents();
        assert_invariant();
    }



    basic_json(initializer_list_t init, bool type_deduction = true, value_t manual_type = value_t::array) {


        bool is_an_object = std::all_of(init.begin(), init.end(), [](const detail::json_ref<basic_json> &element_ref) {



            return element_ref->is_array() && element_ref->size() == 2 && (*element_ref)[static_cast<size_type>(0)].is_string();
        });


        if(!type_deduction) {

            if(manual_type == value_t::array) {
                is_an_object = false;
            }


            if(JSON_HEDLEY_UNLIKELY(manual_type == value_t::object && !is_an_object)) {
                JSON_THROW(type_error::create(301, "cannot create object from initializer list", nullptr));
            }
        }

        if(is_an_object) {

            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;

            for(auto &element_ref: init) {
                auto element = element_ref.moved_or_copied();
                m_data.m_value.object->emplace(
                        std::move(*((*element.m_data.m_value.array)[0].m_data.m_value.string)),
                        std::move((*element.m_data.m_value.array)[1])
                );
            }
        } else {

            m_data.m_type = value_t::array;
            m_data.m_value.array = create<array_t>(init.begin(), init.end());
        }

        set_parents();
        assert_invariant();
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(const typename binary_t::container_type &init) {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = init;
        return res;
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(const typename binary_t::container_type &init, typename binary_t::subtype_type subtype) {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = binary_t(init, subtype);
        return res;
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(typename binary_t::container_type &&init) {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = std::move(init);
        return res;
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(typename binary_t::container_type &&init, typename binary_t::subtype_type subtype) {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = binary_t(std::move(init), subtype);
        return res;
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json array(initializer_list_t init = {}) {
        return basic_json(init, false, value_t::array);
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json object(initializer_list_t init = {}) {
        return basic_json(init, false, value_t::object);
    }



    basic_json(size_type cnt, const basic_json &val): m_data{cnt, val} {
        set_parents();
        assert_invariant();
    }



    template<class InputIT, typename std::enable_if<std::is_same<InputIT, typename basic_json_t::iterator>::value || std::is_same<InputIT, typename basic_json_t::const_iterator>::value, int>::type = 0>
    basic_json(InputIT first, InputIT last) {
        JSON_ASSERT(first.m_object != nullptr);
        JSON_ASSERT(last.m_object != nullptr);


        if(JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object)) {
            JSON_THROW(invalid_iterator::create(201, "iterators are not compatible", nullptr));
        }


        m_data.m_type = first.m_object->m_data.m_type;


        switch(m_data.m_type) {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string: {
                if(JSON_HEDLEY_UNLIKELY(!first.m_it.primitive_iterator.is_begin() || !last.m_it.primitive_iterator.is_end())) {
                    JSON_THROW(invalid_iterator::create(204, "iterators out of range", first.m_object));
                }
                break;
            }

            case value_t::null:
            case value_t::object:
            case value_t::array:
            case value_t::binary:
            case value_t::discarded:
            default:
                break;
        }

        switch(m_data.m_type) {
            case value_t::number_integer: {
                m_data.m_value.number_integer = first.m_object->m_data.m_value.number_integer;
                break;
            }

            case value_t::number_unsigned: {
                m_data.m_value.number_unsigned = first.m_object->m_data.m_value.number_unsigned;
                break;
            }

            case value_t::number_float: {
                m_data.m_value.number_float = first.m_object->m_data.m_value.number_float;
                break;
            }

            case value_t::boolean: {
                m_data.m_value.boolean = first.m_object->m_data.m_value.boolean;
                break;
            }

            case value_t::string: {
                m_data.m_value = *first.m_object->m_data.m_value.string;
                break;
            }

            case value_t::object: {
                m_data.m_value.object = create<object_t>(first.m_it.object_iterator, last.m_it.object_iterator);
                break;
            }

            case value_t::array: {
                m_data.m_value.array = create<array_t>(first.m_it.array_iterator, last.m_it.array_iterator);
                break;
            }

            case value_t::binary: {
                m_data.m_value = *first.m_object->m_data.m_value.binary;
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(invalid_iterator::create(206, detail::concat("cannot construct with iterators from ", first.m_object->type_name()), first.m_object));
        }

        set_parents();
        assert_invariant();
    }





    template<typename JsonRef, detail::enable_if_t<detail::conjunction<detail::is_json_ref<JsonRef>, std::is_same<typename JsonRef::value_type, basic_json>>::value, int> = 0>
    basic_json(const JsonRef &ref): basic_json(ref.moved_or_copied()) {}



    basic_json(const basic_json &other)
        : json_base_class_t(other) {
        m_data.m_type = other.m_data.m_type;

        other.assert_invariant();

        switch(m_data.m_type) {
            case value_t::object: {
                m_data.m_value = *other.m_data.m_value.object;
                break;
            }

            case value_t::array: {
                m_data.m_value = *other.m_data.m_value.array;
                break;
            }

            case value_t::string: {
                m_data.m_value = *other.m_data.m_value.string;
                break;
            }

            case value_t::boolean: {
                m_data.m_value = other.m_data.m_value.boolean;
                break;
            }

            case value_t::number_integer: {
                m_data.m_value = other.m_data.m_value.number_integer;
                break;
            }

            case value_t::number_unsigned: {
                m_data.m_value = other.m_data.m_value.number_unsigned;
                break;
            }

            case value_t::number_float: {
                m_data.m_value = other.m_data.m_value.number_float;
                break;
            }

            case value_t::binary: {
                m_data.m_value = *other.m_data.m_value.binary;
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                break;
        }

        set_parents();
        assert_invariant();
    }



    basic_json(basic_json &&other) noexcept
        : json_base_class_t(std::forward<json_base_class_t>(other)),
          m_data(std::move(other.m_data)) {

        other.assert_invariant(false);


        other.m_data.m_type = value_t::null;
        other.m_data.m_value = {};

        set_parents();
        assert_invariant();
    }



    basic_json &operator=(basic_json other) noexcept(
            std::is_nothrow_move_constructible<value_t>::value &&
            std::is_nothrow_move_assignable<value_t>::value &&
            std::is_nothrow_move_constructible<json_value>::value &&
            std::is_nothrow_move_assignable<json_value>::value &&
            std::is_nothrow_move_assignable<json_base_class_t>::value
    ) {

        other.assert_invariant();

        using std::swap;
        swap(m_data.m_type, other.m_data.m_type);
        swap(m_data.m_value, other.m_data.m_value);
        json_base_class_t::operator=(std::move(other));

        set_parents();
        assert_invariant();
        return *this;
    }



    ~basic_json() noexcept {
        assert_invariant(false);
    }



  public:










    string_t dump(const int indent = -1, const char indent_char = ' ', const bool ensure_ascii = false, const error_handler_t error_handler = error_handler_t::strict) const {
        string_t result;
        serializer s(detail::output_adapter<char, string_t>(result), indent_char, error_handler);

        if(indent >= 0) {
            s.dump(*this, true, ensure_ascii, static_cast<unsigned int>(indent));
        } else {
            s.dump(*this, false, ensure_ascii, 0);
        }

        return result;
    }



    constexpr value_t type() const noexcept {
        return m_data.m_type;
    }



    constexpr bool is_primitive() const noexcept {
        return is_null() || is_string() || is_boolean() || is_number() || is_binary();
    }



    constexpr bool is_structured() const noexcept {
        return is_array() || is_object();
    }



    constexpr bool is_null() const noexcept {
        return m_data.m_type == value_t::null;
    }



    constexpr bool is_boolean() const noexcept {
        return m_data.m_type == value_t::boolean;
    }



    constexpr bool is_bool() const noexcept {
        return is_boolean();
    }



    constexpr bool is_number() const noexcept {
        return is_number_integer() || is_number_float();
    }



    constexpr bool is_number_integer() const noexcept {
        return m_data.m_type == value_t::number_integer || m_data.m_type == value_t::number_unsigned;
    }



    constexpr bool is_number_unsigned() const noexcept {
        return m_data.m_type == value_t::number_unsigned;
    }



    constexpr bool is_int() const noexcept {
        return is_number_integer();
    }



    constexpr bool is_number_float() const noexcept {
        return m_data.m_type == value_t::number_float;
    }



    constexpr bool is_object() const noexcept {
        return m_data.m_type == value_t::object;
    }



    constexpr bool is_array() const noexcept {
        return m_data.m_type == value_t::array;
    }



    constexpr bool is_string() const noexcept {
        return m_data.m_type == value_t::string;
    }



    constexpr bool is_binary() const noexcept {
        return m_data.m_type == value_t::binary;
    }



    constexpr bool is_discarded() const noexcept {
        return m_data.m_type == value_t::discarded;
    }



    constexpr operator value_t() const noexcept {
        return m_data.m_type;
    }



  private:





    boolean_t get_impl(boolean_t * ) const {
        if(JSON_HEDLEY_LIKELY(is_boolean())) {
            return m_data.m_value.boolean;
        }

        JSON_THROW(type_error::create(302, detail::concat("type must be boolean, but is ", type_name()), this));
    }


    object_t *get_impl_ptr(object_t * ) noexcept {
        return is_object() ? m_data.m_value.object : nullptr;
    }


    constexpr const object_t *get_impl_ptr(const object_t * ) const noexcept {
        return is_object() ? m_data.m_value.object : nullptr;
    }


    array_t *get_impl_ptr(array_t * ) noexcept {
        return is_array() ? m_data.m_value.array : nullptr;
    }


    constexpr const array_t *get_impl_ptr(const array_t * ) const noexcept {
        return is_array() ? m_data.m_value.array : nullptr;
    }


    string_t *get_impl_ptr(string_t * ) noexcept {
        return is_string() ? m_data.m_value.string : nullptr;
    }


    constexpr const string_t *get_impl_ptr(const string_t * ) const noexcept {
        return is_string() ? m_data.m_value.string : nullptr;
    }


    boolean_t *get_impl_ptr(boolean_t * ) noexcept {
        return is_boolean() ? &m_data.m_value.boolean : nullptr;
    }


    constexpr const boolean_t *get_impl_ptr(const boolean_t * ) const noexcept {
        return is_boolean() ? &m_data.m_value.boolean : nullptr;
    }


    number_integer_t *get_impl_ptr(number_integer_t * ) noexcept {
        return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
    }


    constexpr const number_integer_t *get_impl_ptr(const number_integer_t * ) const noexcept {
        return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
    }


    number_unsigned_t *get_impl_ptr(number_unsigned_t * ) noexcept {
        return is_number_unsigned() ? &m_data.m_value.number_unsigned : nullptr;
    }


    constexpr const number_unsigned_t *get_impl_ptr(const number_unsigned_t * ) const noexcept {
        return is_number_unsigned() ? &m_data.m_value.number_unsigned : nullptr;
    }


    number_float_t *get_impl_ptr(number_float_t * ) noexcept {
        return is_number_float() ? &m_data.m_value.number_float : nullptr;
    }


    constexpr const number_float_t *get_impl_ptr(const number_float_t * ) const noexcept {
        return is_number_float() ? &m_data.m_value.number_float : nullptr;
    }


    binary_t *get_impl_ptr(binary_t * ) noexcept {
        return is_binary() ? m_data.m_value.binary : nullptr;
    }


    constexpr const binary_t *get_impl_ptr(const binary_t * ) const noexcept {
        return is_binary() ? m_data.m_value.binary : nullptr;
    }

  public:
    


    object_t &as_object() {
        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be object, but is ", type_name()), this));
        }
        return *m_data.m_value.object;
    }


    const object_t &as_object() const {
        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be object, but is ", type_name()), this));
        }
        return *m_data.m_value.object;
    }


    array_t &as_array() {
        if(JSON_HEDLEY_UNLIKELY(!is_array())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be array, but is ", type_name()), this));
        }
        return *m_data.m_value.array;
    }


    const array_t &as_array() const {
        if(JSON_HEDLEY_UNLIKELY(!is_array())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be array, but is ", type_name()), this));
        }
        return *m_data.m_value.array;
    }


    string_t as_string() const {
        if(JSON_HEDLEY_UNLIKELY(!is_string())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be string, but is ", type_name()), this));
        }
        return *m_data.m_value.string;
    }


    boolean_t as_bool() const {
        if(JSON_HEDLEY_UNLIKELY(!is_boolean())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be boolean, but is ", type_name()), this));
        }
        return m_data.m_value.boolean;
    }


    number_float_t as_number() const {
        if(JSON_HEDLEY_UNLIKELY(!is_number())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be number, but is ", type_name()), this));
        }
        return is_number_float() ? m_data.m_value.number_float
                                 : static_cast<number_float_t>(m_data.m_value.number_integer);
    }


    number_integer_t as_int() const {
        if(JSON_HEDLEY_UNLIKELY(!is_number_integer())) {
            JSON_THROW(type_error::create(302, detail::concat("type must be integer, but is ", type_name()), this));
        }
        return is_number_unsigned() ? static_cast<number_integer_t>(m_data.m_value.number_unsigned)
                                    : m_data.m_value.number_integer;
    }

  private:
    
    template<typename ReferenceType, typename ThisType>
    static ReferenceType get_ref_impl(ThisType &obj) {

        auto *ptr = obj.template get_ptr<typename std::add_pointer<ReferenceType>::type>();

        if(JSON_HEDLEY_LIKELY(ptr != nullptr)) {
            return *ptr;
        }

        JSON_THROW(type_error::create(303, detail::concat("incompatible ReferenceType for get_ref, actual type is ", obj.type_name()), &obj));
    }

  public:






    template<typename PointerType, typename std::enable_if<std::is_pointer<PointerType>::value, int>::type = 0>
    auto get_ptr() noexcept -> decltype(std::declval<basic_json_t &>().get_impl_ptr(std::declval<PointerType>())) {

        return get_impl_ptr(static_cast<PointerType>(nullptr));
    }



    template<typename PointerType, typename std::enable_if<std::is_pointer<PointerType>::value && std::is_const<typename std::remove_pointer<PointerType>::type>::value, int>::type = 0>
    constexpr auto get_ptr() const noexcept -> decltype(std::declval<const basic_json_t &>().get_impl_ptr(std::declval<PointerType>())) {

        return get_impl_ptr(static_cast<PointerType>(nullptr));
    }

  private:
    
    template<typename ValueType, detail::enable_if_t<detail::is_default_constructible<ValueType>::value && detail::has_from_json<basic_json_t, ValueType>::value, int> = 0>
    ValueType get_impl(detail::priority_tag<0> ) const noexcept(noexcept(
            JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t &>(), std::declval<ValueType &>())
    )) {
        auto ret = ValueType();
        JSONSerializer<ValueType>::from_json(*this, ret);
        return ret;
    }

    
    template<typename ValueType, detail::enable_if_t<detail::has_non_default_from_json<basic_json_t, ValueType>::value, int> = 0>
    ValueType get_impl(detail::priority_tag<1> ) const noexcept(noexcept(
            JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t &>())
    )) {
        return JSONSerializer<ValueType>::from_json(*this);
    }

    
    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    BasicJsonType get_impl(detail::priority_tag<2> ) const {
        return *this;
    }

    
    template<typename BasicJsonType, detail::enable_if_t<std::is_same<BasicJsonType, basic_json_t>::value, int> = 0>
    basic_json get_impl(detail::priority_tag<3> ) const {
        return *this;
    }

    
    template<typename PointerType, detail::enable_if_t<std::is_pointer<PointerType>::value, int> = 0>
    constexpr auto get_impl(detail::priority_tag<4> ) const noexcept
            -> decltype(std::declval<const basic_json_t &>().template get_ptr<PointerType>()) {

        return get_ptr<PointerType>();
    }

  public:
    
    template<typename ValueTypeCV, typename ValueType = detail::uncvref_t<ValueTypeCV>>
#if defined(JSON_HAS_CPP_14)
    constexpr
#endif
            auto get() const noexcept(
                    noexcept(std::declval<const basic_json_t &>().template get_impl<ValueType>(detail::priority_tag<4>{}))
            )
                    -> decltype(std::declval<const basic_json_t &>().template get_impl<ValueType>(detail::priority_tag<4>{})) {



        static_assert(!std::is_reference<ValueTypeCV>::value, "get() cannot be used with reference types, you might want to use get_ref()");
        return get_impl<ValueType>(detail::priority_tag<4>{});
    }

    
    template<typename PointerType, typename std::enable_if<std::is_pointer<PointerType>::value, int>::type = 0>
    auto get() noexcept -> decltype(std::declval<basic_json_t &>().template get_ptr<PointerType>()) {

        return get_ptr<PointerType>();
    }



    template<typename ValueType, detail::enable_if_t<!detail::is_basic_json<ValueType>::value && detail::has_from_json<basic_json_t, ValueType>::value, int> = 0>
    ValueType &get_to(ValueType &v) const noexcept(noexcept(
            JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t &>(), v)
    )) {
        JSONSerializer<ValueType>::from_json(*this, v);
        return v;
    }



    template<typename ValueType, detail::enable_if_t<detail::is_basic_json<ValueType>::value, int> = 0>
    ValueType &get_to(ValueType &v) const {
        v = *this;
        return v;
    }

    template<
            typename T,
            std::size_t N,
            typename Array = T (&)[N],
            detail::enable_if_t<
                    detail::has_from_json<basic_json_t, Array>::value,
                    int> = 0>
    Array get_to(T (&v)[N]) const
            noexcept(noexcept(JSONSerializer<Array>::from_json(
                    std::declval<const basic_json_t &>(),
                    v
            ))) {
        JSONSerializer<Array>::from_json(*this, v);
        return v;
    }



    template<typename ReferenceType, typename std::enable_if<std::is_reference<ReferenceType>::value, int>::type = 0>
    ReferenceType get_ref() {

        return get_ref_impl<ReferenceType>(*this);
    }



    template<typename ReferenceType, typename std::enable_if<std::is_reference<ReferenceType>::value && std::is_const<typename std::remove_reference<ReferenceType>::type>::value, int>::type = 0>
    ReferenceType get_ref() const {

        return get_ref_impl<ReferenceType>(*this);
    }

    
    template<typename ValueType, typename std::enable_if<detail::conjunction<detail::negation<std::is_pointer<ValueType>>, detail::negation<std::is_same<ValueType, std::nullptr_t>>, detail::negation<std::is_same<ValueType, detail::json_ref<basic_json>>>, detail::negation<std::is_same<ValueType, typename string_t::value_type>>, detail::negation<detail::is_basic_json<ValueType>>, detail::negation<std::is_same<ValueType, std::initializer_list<typename string_t::value_type>>>,
#if defined(JSON_HAS_CPP_17) && (defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSC_VER <= 1914))
                                                                             detail::negation<std::is_same<ValueType, std::string_view>>,
#endif
#if defined(JSON_HAS_CPP_17) && JSON_HAS_STATIC_RTTI
                                                                             detail::negation<std::is_same<ValueType, std::any>>,
#endif
                                                                             detail::is_detected_lazy<detail::get_template_function, const basic_json_t &, ValueType>>::value,
                                                         int>::type = 0>
    JSON_EXPLICIT operator ValueType() const {

        return get<ValueType>();
    }



    binary_t &get_binary() {
        if(!is_binary()) {
            JSON_THROW(type_error::create(302, detail::concat("type must be binary, but is ", type_name()), this));
        }

        return *get_ptr<binary_t *>();
    }



    const binary_t &get_binary() const {
        if(!is_binary()) {
            JSON_THROW(type_error::create(302, detail::concat("type must be binary, but is ", type_name()), this));
        }

        return *get_ptr<const binary_t *>();
    }













    reference at(size_type idx) {

        if(JSON_HEDLEY_LIKELY(is_array())) {
            JSON_TRY {
                return set_parent(m_data.m_value.array->at(idx));
            }
            JSON_CATCH(std::out_of_range &) {

                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }
        } else {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }
    }



    const_reference at(size_type idx) const {

        if(JSON_HEDLEY_LIKELY(is_array())) {
            JSON_TRY {
                return m_data.m_value.array->at(idx);
            }
            JSON_CATCH(std::out_of_range &) {

                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }
        } else {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }
    }



    reference at(const typename object_t::key_type &key) {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(key);
        if(it == m_data.m_value.object->end()) {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", key, "' not found"), this));
        }
        return set_parent(it->second);
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    reference at(KeyType &&key) {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if(it == m_data.m_value.object->end()) {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", string_t(std::forward<KeyType>(key)), "' not found"), this));
        }
        return set_parent(it->second);
    }



    const_reference at(const typename object_t::key_type &key) const {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(key);
        if(it == m_data.m_value.object->end()) {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", key, "' not found"), this));
        }
        return it->second;
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_reference at(KeyType &&key) const {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if(it == m_data.m_value.object->end()) {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", string_t(std::forward<KeyType>(key)), "' not found"), this));
        }
        return it->second;
    }



    reference operator[](size_type idx) {

        if(is_null()) {
            m_data.m_type = value_t::array;
            m_data.m_value.array = create<array_t>();
            assert_invariant();
        }


        if(JSON_HEDLEY_LIKELY(is_array())) {

            if(idx >= m_data.m_value.array->size()) {
#if JSON_DIAGNOSTICS

                const auto old_size = m_data.m_value.array->size();
                const auto old_capacity = m_data.m_value.array->capacity();
#endif
                m_data.m_value.array->resize(idx + 1);

#if JSON_DIAGNOSTICS
                if(JSON_HEDLEY_UNLIKELY(m_data.m_value.array->capacity() != old_capacity)) {

                    set_parents();
                } else {

                    set_parents(begin() + static_cast<typename iterator::difference_type>(old_size), static_cast<typename iterator::difference_type>(idx + 1 - old_size));
                }
#endif
                assert_invariant();
            }

            return m_data.m_value.array->operator[](idx);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a numeric argument with ", type_name()), this));
    }



    const_reference operator[](size_type idx) const {

        if(JSON_HEDLEY_LIKELY(is_array())) {
            return m_data.m_value.array->operator[](idx);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a numeric argument with ", type_name()), this));
    }



    reference operator[](typename object_t::key_type key) {

        if(is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }


        if(JSON_HEDLEY_LIKELY(is_object())) {
            auto result = m_data.m_value.object->emplace(std::move(key), nullptr);
            return set_parent(result.first->second);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }



    const_reference operator[](const typename object_t::key_type &key) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {
            auto it = m_data.m_value.object->find(key);
            JSON_ASSERT(it != m_data.m_value.object->end());
            return it->second;
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }



    template<typename T>
    reference operator[](T *key) {
        return operator[](typename object_t::key_type(key));
    }

    template<typename T>
    const_reference operator[](T *key) const {
        return operator[](typename object_t::key_type(key));
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    reference operator[](KeyType &&key) {

        if(is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }


        if(JSON_HEDLEY_LIKELY(is_object())) {
            auto result = m_data.m_value.object->emplace(std::forward<KeyType>(key), nullptr);
            return set_parent(result.first->second);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_reference operator[](KeyType &&key) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {
            auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
            JSON_ASSERT(it != m_data.m_value.object->end());
            return it->second;
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

  private:
    template<typename KeyType>
    using is_comparable_with_object_key = detail::is_comparable<
            object_comparator_t,
            const typename object_t::key_type &,
            KeyType>;

    template<typename ValueType>
    using value_return_type = std::conditional<
            detail::is_c_string_uncvref<ValueType>::value,
            string_t,
            typename std::decay<ValueType>::type>;

  public:


    template<class ValueType, detail::enable_if_t<!detail::is_transparent<object_comparator_t>::value && detail::is_getable<basic_json_t, ValueType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ValueType value(const typename object_t::key_type &key, const ValueType &default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            const auto it = find(key);
            if(it != end()) {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }



    template<class ValueType, class ReturnType = typename value_return_type<ValueType>::type, detail::enable_if_t<!detail::is_transparent<object_comparator_t>::value && detail::is_getable<basic_json_t, ReturnType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ReturnType value(const typename object_t::key_type &key, ValueType &&default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            const auto it = find(key);
            if(it != end()) {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }



    template<class ValueType, class KeyType, detail::enable_if_t<detail::is_transparent<object_comparator_t>::value && !detail::is_json_pointer<KeyType>::value && is_comparable_with_object_key<KeyType>::value && detail::is_getable<basic_json_t, ValueType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ValueType value(KeyType &&key, const ValueType &default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            const auto it = find(std::forward<KeyType>(key));
            if(it != end()) {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }



    template<class ValueType, class KeyType, class ReturnType = typename value_return_type<ValueType>::type, detail::enable_if_t<detail::is_transparent<object_comparator_t>::value && !detail::is_json_pointer<KeyType>::value && is_comparable_with_object_key<KeyType>::value && detail::is_getable<basic_json_t, ReturnType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ReturnType value(KeyType &&key, ValueType &&default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            const auto it = find(std::forward<KeyType>(key));
            if(it != end()) {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }



    template<class ValueType, detail::enable_if_t<detail::is_getable<basic_json_t, ValueType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ValueType value(const json_pointer &ptr, const ValueType &default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            JSON_TRY {
                return ptr.get_checked(this).template get<ValueType>();
            }
            JSON_INTERNAL_CATCH(out_of_range &) {
                return default_value;
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }



    template<class ValueType, class ReturnType = typename value_return_type<ValueType>::type, detail::enable_if_t<detail::is_getable<basic_json_t, ReturnType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    ReturnType value(const json_pointer &ptr, ValueType &&default_value) const {

        if(JSON_HEDLEY_LIKELY(is_object())) {

            JSON_TRY {
                return ptr.get_checked(this).template get<ReturnType>();
            }
            JSON_INTERNAL_CATCH(out_of_range &) {
                return std::forward<ValueType>(default_value);
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    template<class ValueType, class BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value && detail::is_getable<basic_json_t, ValueType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    ValueType value(const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr, const ValueType &default_value) const {
        return value(ptr.convert(), default_value);
    }

    template<class ValueType, class BasicJsonType, class ReturnType = typename value_return_type<ValueType>::type, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value && detail::is_getable<basic_json_t, ReturnType>::value && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    ReturnType value(const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr, ValueType &&default_value) const {
        return value(ptr.convert(), std::forward<ValueType>(default_value));
    }



    reference front() {
        return *begin();
    }



    const_reference front() const {
        return *cbegin();
    }



    reference back() {
        auto tmp = end();
        --tmp;
        return *tmp;
    }



    const_reference back() const {
        auto tmp = cend();
        --tmp;
        return *tmp;
    }



    template<class IteratorType, detail::enable_if_t<std::is_same<IteratorType, typename basic_json_t::iterator>::value || std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int> = 0>
    IteratorType erase(IteratorType pos) {

        if(JSON_HEDLEY_UNLIKELY(this != pos.m_object)) {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        IteratorType result = end();

        switch(m_data.m_type) {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary: {
                if(JSON_HEDLEY_UNLIKELY(!pos.m_it.primitive_iterator.is_begin())) {
                    JSON_THROW(invalid_iterator::create(205, "iterator out of range", this));
                }

                if(is_string()) {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.string, 1);
                    m_data.m_value.string = nullptr;
                } else if(is_binary()) {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.binary, 1);
                    m_data.m_value.binary = nullptr;
                }

                m_data.m_type = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object: {
                result.m_it.object_iterator = m_data.m_value.object->erase(pos.m_it.object_iterator);
                break;
            }

            case value_t::array: {
                result.m_it.array_iterator = m_data.m_value.array->erase(pos.m_it.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }



    template<class IteratorType, detail::enable_if_t<std::is_same<IteratorType, typename basic_json_t::iterator>::value || std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int> = 0>
    IteratorType erase(IteratorType first, IteratorType last) {

        if(JSON_HEDLEY_UNLIKELY(this != first.m_object || this != last.m_object)) {
            JSON_THROW(invalid_iterator::create(203, "iterators do not fit current value", this));
        }

        IteratorType result = end();

        switch(m_data.m_type) {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary: {
                if(JSON_HEDLEY_LIKELY(!first.m_it.primitive_iterator.is_begin() || !last.m_it.primitive_iterator.is_end())) {
                    JSON_THROW(invalid_iterator::create(204, "iterators out of range", this));
                }

                if(is_string()) {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.string, 1);
                    m_data.m_value.string = nullptr;
                } else if(is_binary()) {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.binary, 1);
                    m_data.m_value.binary = nullptr;
                }

                m_data.m_type = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object: {
                result.m_it.object_iterator = m_data.m_value.object->erase(first.m_it.object_iterator, last.m_it.object_iterator);
                break;
            }

            case value_t::array: {
                result.m_it.array_iterator = m_data.m_value.array->erase(first.m_it.array_iterator, last.m_it.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }

  private:
    template<typename KeyType, detail::enable_if_t<detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type erase_internal(KeyType &&key) {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return m_data.m_value.object->erase(std::forward<KeyType>(key));
    }

    template<typename KeyType, detail::enable_if_t<!detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type erase_internal(KeyType &&key) {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        const auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if(it != m_data.m_value.object->end()) {
            m_data.m_value.object->erase(it);
            return 1;
        }
        return 0;
    }

  public:


    size_type erase(const typename object_t::key_type &key) {


        return erase_internal(key);
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type erase(KeyType &&key) {
        return erase_internal(std::forward<KeyType>(key));
    }



    void erase(const size_type idx) {

        if(JSON_HEDLEY_LIKELY(is_array())) {
            if(JSON_HEDLEY_UNLIKELY(idx >= size())) {
                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }

            m_data.m_value.array->erase(m_data.m_value.array->begin() + static_cast<difference_type>(idx));
        } else {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }
    }












    iterator find(const typename object_t::key_type &key) {
        auto result = end();

        if(is_object()) {
            result.m_it.object_iterator = m_data.m_value.object->find(key);
        }

        return result;
    }



    const_iterator find(const typename object_t::key_type &key) const {
        auto result = cend();

        if(is_object()) {
            result.m_it.object_iterator = m_data.m_value.object->find(key);
        }

        return result;
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    iterator find(KeyType &&key) {
        auto result = end();

        if(is_object()) {
            result.m_it.object_iterator = m_data.m_value.object->find(std::forward<KeyType>(key));
        }

        return result;
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_iterator find(KeyType &&key) const {
        auto result = cend();

        if(is_object()) {
            result.m_it.object_iterator = m_data.m_value.object->find(std::forward<KeyType>(key));
        }

        return result;
    }



    size_type count(const typename object_t::key_type &key) const {

        return is_object() ? m_data.m_value.object->count(key) : 0;
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type count(KeyType &&key) const {

        return is_object() ? m_data.m_value.object->count(std::forward<KeyType>(key)) : 0;
    }



    bool contains(const typename object_t::key_type &key) const {
        return is_object() && m_data.m_value.object->find(key) != m_data.m_value.object->end();
    }



    template<class KeyType, detail::enable_if_t<detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    bool contains(KeyType &&key) const {
        return is_object() && m_data.m_value.object->find(std::forward<KeyType>(key)) != m_data.m_value.object->end();
    }



    bool contains(const json_pointer &ptr) const {
        return ptr.contains(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    bool contains(const typename ::silicon::json::impl::json_pointer<BasicJsonType> &ptr) const {
        return ptr.contains(this);
    }












    iterator begin() noexcept {
        iterator result(this);
        result.set_begin();
        return result;
    }



    const_iterator begin() const noexcept {
        return cbegin();
    }



    const_iterator cbegin() const noexcept {
        const_iterator result(this);
        result.set_begin();
        return result;
    }



    iterator end() noexcept {
        iterator result(this);
        result.set_end();
        return result;
    }



    const_iterator end() const noexcept {
        return cend();
    }



    const_iterator cend() const noexcept {
        const_iterator result(this);
        result.set_end();
        return result;
    }



    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }



    const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }



    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }



    const_reverse_iterator rend() const noexcept {
        return crend();
    }



    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }



    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

  public:





    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<iterator> iterator_wrapper(reference ref) noexcept {
        return ref.items();
    }






    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<const_iterator> iterator_wrapper(const_reference ref) noexcept {
        return ref.items();
    }



    iteration_proxy<iterator> items() noexcept {
        return iteration_proxy<iterator>(*this);
    }



    iteration_proxy<const_iterator> items() const noexcept {
        return iteration_proxy<const_iterator>(*this);
    }












    bool empty() const noexcept {
        switch(m_data.m_type) {
            case value_t::null: {

                return true;
            }

            case value_t::array: {

                return m_data.m_value.array->empty();
            }

            case value_t::object: {

                return m_data.m_value.object->empty();
            }

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {

                return false;
            }
        }
    }



    size_type size() const noexcept {
        switch(m_data.m_type) {
            case value_t::null: {

                return 0;
            }

            case value_t::array: {

                return m_data.m_value.array->size();
            }

            case value_t::object: {

                return m_data.m_value.object->size();
            }

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {

                return 1;
            }
        }
    }



    size_type max_size() const noexcept {
        switch(m_data.m_type) {
            case value_t::array: {

                return m_data.m_value.array->max_size();
            }

            case value_t::object: {

                return m_data.m_value.object->max_size();
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {

                return size();
            }
        }
    }












    void clear() noexcept {
        switch(m_data.m_type) {
            case value_t::number_integer: {
                m_data.m_value.number_integer = 0;
                break;
            }

            case value_t::number_unsigned: {
                m_data.m_value.number_unsigned = 0;
                break;
            }

            case value_t::number_float: {
                m_data.m_value.number_float = 0.0;
                break;
            }

            case value_t::boolean: {
                m_data.m_value.boolean = false;
                break;
            }

            case value_t::string: {
                m_data.m_value.string->clear();
                break;
            }

            case value_t::binary: {
                m_data.m_value.binary->clear();
                break;
            }

            case value_t::array: {
                m_data.m_value.array->clear();
                break;
            }

            case value_t::object: {
                m_data.m_value.object->clear();
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                break;
        }
    }



    void push_back(basic_json &&val) {

        if(JSON_HEDLEY_UNLIKELY(!(is_null() || is_array()))) {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }


        if(is_null()) {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }


        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->push_back(std::move(val));
        set_parent(m_data.m_value.array->back(), old_capacity);

    }



    reference operator+=(basic_json &&val) {
        push_back(std::move(val));
        return *this;
    }



    void push_back(const basic_json &val) {

        if(JSON_HEDLEY_UNLIKELY(!(is_null() || is_array()))) {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }


        if(is_null()) {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }


        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->push_back(val);
        set_parent(m_data.m_value.array->back(), old_capacity);
    }



    reference operator+=(const basic_json &val) {
        push_back(val);
        return *this;
    }



    void push_back(const typename object_t::value_type &val) {

        if(JSON_HEDLEY_UNLIKELY(!(is_null() || is_object()))) {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }


        if(is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;
            assert_invariant();
        }


        auto res = m_data.m_value.object->insert(val);
        set_parent(res.first->second);
    }



    reference operator+=(const typename object_t::value_type &val) {
        push_back(val);
        return *this;
    }



    void push_back(initializer_list_t init) {
        if(is_object() && init.size() == 2 && (*init.begin())->is_string()) {
            basic_json &&key = init.begin()->moved_or_copied();
            push_back(typename object_t::value_type(std::move(key.get_ref<string_t &>()), (init.begin() + 1)->moved_or_copied()));
        } else {
            push_back(basic_json(init));
        }
    }



    reference operator+=(initializer_list_t init) {
        push_back(init);
        return *this;
    }



    template<class... Args>
    reference emplace_back(Args &&...args) {

        if(JSON_HEDLEY_UNLIKELY(!(is_null() || is_array()))) {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace_back() with ", type_name()), this));
        }


        if(is_null()) {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }


        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->emplace_back(std::forward<Args>(args)...);
        return set_parent(m_data.m_value.array->back(), old_capacity);
    }



    template<class... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {

        if(JSON_HEDLEY_UNLIKELY(!(is_null() || is_object()))) {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace() with ", type_name()), this));
        }


        if(is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;
            assert_invariant();
        }


        auto res = m_data.m_value.object->emplace(std::forward<Args>(args)...);
        set_parent(res.first->second);


        auto it = begin();
        it.m_it.object_iterator = res.first;


        return {it, res.second};
    }




    template<typename... Args>
    iterator insert_iterator(const_iterator pos, Args &&...args) {
        iterator result(this);
        JSON_ASSERT(m_data.m_value.array != nullptr);

        auto insert_pos = std::distance(m_data.m_value.array->begin(), pos.m_it.array_iterator);
        m_data.m_value.array->insert(pos.m_it.array_iterator, std::forward<Args>(args)...);
        result.m_it.array_iterator = m_data.m_value.array->begin() + insert_pos;





        set_parents();
        return result;
    }



    iterator insert(const_iterator pos, const basic_json &val) {

        if(JSON_HEDLEY_LIKELY(is_array())) {

            if(JSON_HEDLEY_UNLIKELY(pos.m_object != this)) {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }


            return insert_iterator(pos, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }



    iterator insert(const_iterator pos, basic_json &&val) {
        return insert(pos, val);
    }



    iterator insert(const_iterator pos, size_type cnt, const basic_json &val) {

        if(JSON_HEDLEY_LIKELY(is_array())) {

            if(JSON_HEDLEY_UNLIKELY(pos.m_object != this)) {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }


            return insert_iterator(pos, cnt, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }



    iterator insert(const_iterator pos, const_iterator first, const_iterator last) {

        if(JSON_HEDLEY_UNLIKELY(!is_array())) {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }


        if(JSON_HEDLEY_UNLIKELY(pos.m_object != this)) {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }


        if(JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object)) {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        if(JSON_HEDLEY_UNLIKELY(first.m_object == this)) {
            JSON_THROW(invalid_iterator::create(211, "passed iterators may not belong to container", this));
        }


        return insert_iterator(pos, first.m_it.array_iterator, last.m_it.array_iterator);
    }



    iterator insert(const_iterator pos, initializer_list_t ilist) {

        if(JSON_HEDLEY_UNLIKELY(!is_array())) {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }


        if(JSON_HEDLEY_UNLIKELY(pos.m_object != this)) {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }


        return insert_iterator(pos, ilist.begin(), ilist.end());
    }



    void insert(const_iterator first, const_iterator last) {

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }


        if(JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object)) {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }


        if(JSON_HEDLEY_UNLIKELY(!first.m_object->is_object())) {
            JSON_THROW(invalid_iterator::create(202, "iterators first and last must point to objects", this));
        }

        m_data.m_value.object->insert(first.m_it.object_iterator, last.m_it.object_iterator);
    }



    void update(const_reference j, bool merge_objects = false) {
        update(j.begin(), j.end(), merge_objects);
    }



    void update(const_iterator first, const_iterator last, bool merge_objects = false) {

        if(is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }

        if(JSON_HEDLEY_UNLIKELY(!is_object())) {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", type_name()), this));
        }


        if(JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object)) {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }


        if(JSON_HEDLEY_UNLIKELY(!first.m_object->is_object())) {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", first.m_object->type_name()), first.m_object));
        }

        for(auto it = first; it != last; ++it) {
            if(merge_objects && it.value().is_object()) {
                auto it2 = m_data.m_value.object->find(it.key());
                if(it2 != m_data.m_value.object->end()) {
                    it2->second.update(it.value(), true);
                    continue;
                }
            }
            m_data.m_value.object->operator[](it.key()) = it.value();
#if JSON_DIAGNOSTICS
            m_data.m_value.object->operator[](it.key()).m_parent = this;
#endif
        }
    }



    void swap(reference other) noexcept(
            std::is_nothrow_move_constructible<value_t>::value &&
            std::is_nothrow_move_assignable<value_t>::value &&
            std::is_nothrow_move_constructible<json_value>::value &&
            std::is_nothrow_move_assignable<json_value>::value
    ) {
        std::swap(m_data.m_type, other.m_data.m_type);
        std::swap(m_data.m_value, other.m_data.m_value);

        set_parents();
        other.set_parents();
        assert_invariant();
    }



    friend void swap(reference left, reference right) noexcept(
            std::is_nothrow_move_constructible<value_t>::value &&
            std::is_nothrow_move_assignable<value_t>::value &&
            std::is_nothrow_move_constructible<json_value>::value &&
            std::is_nothrow_move_assignable<json_value>::value
    ) {
        left.swap(right);
    }



    void swap(array_t &other)
    {

        if(JSON_HEDLEY_LIKELY(is_array())) {
            using std::swap;
            swap(*(m_data.m_value.array), other);
        } else {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(array_t&) with ", type_name()), this));
        }
    }



    void swap(object_t &other)
    {

        if(JSON_HEDLEY_LIKELY(is_object())) {
            using std::swap;
            swap(*(m_data.m_value.object), other);
        } else {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(object_t&) with ", type_name()), this));
        }
    }



    void swap(string_t &other)
    {

        if(JSON_HEDLEY_LIKELY(is_string())) {
            using std::swap;
            swap(*(m_data.m_value.string), other);
        } else {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(string_t&) with ", type_name()), this));
        }
    }



    void swap(binary_t &other)
    {

        if(JSON_HEDLEY_LIKELY(is_binary())) {
            using std::swap;
            swap(*(m_data.m_value.binary), other);
        } else {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t&) with ", type_name()), this));
        }
    }



    void swap(typename binary_t::container_type &other)
    {

        if(JSON_HEDLEY_LIKELY(is_binary())) {
            using std::swap;
            swap(*(m_data.m_value.binary), other);
        } else {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t::container_type&) with ", type_name()), this));
        }
    }












#define JSON_IMPLEMENT_OPERATOR(op, null_result, unordered_result, default_result)                                     \
    const auto lhs_type = lhs.type();                                                                                  \
    const auto rhs_type = rhs.type();                                                                                  \
                                                                                                                       \
    if(lhs_type == rhs_type)                                                           \
    {                                                                                                                  \
        switch(lhs_type) {                                                                                             \
            case value_t::array:                                                                                       \
                return (*lhs.m_data.m_value.array)op(*rhs.m_data.m_value.array);                                       \
                                                                                                                       \
            case value_t::object:                                                                                      \
                return (*lhs.m_data.m_value.object)op(*rhs.m_data.m_value.object);                                     \
                                                                                                                       \
            case value_t::null:                                                                                        \
                return (null_result);                                                                                  \
                                                                                                                       \
            case value_t::string:                                                                                      \
                return (*lhs.m_data.m_value.string)op(*rhs.m_data.m_value.string);                                     \
                                                                                                                       \
            case value_t::boolean:                                                                                     \
                return (lhs.m_data.m_value.boolean)op(rhs.m_data.m_value.boolean);                                     \
                                                                                                                       \
            case value_t::number_integer:                                                                              \
                return (lhs.m_data.m_value.number_integer)op(rhs.m_data.m_value.number_integer);                       \
                                                                                                                       \
            case value_t::number_unsigned:                                                                             \
                return (lhs.m_data.m_value.number_unsigned)op(rhs.m_data.m_value.number_unsigned);                     \
                                                                                                                       \
            case value_t::number_float:                                                                                \
                return (lhs.m_data.m_value.number_float)op(rhs.m_data.m_value.number_float);                           \
                                                                                                                       \
            case value_t::binary:                                                                                      \
                return (*lhs.m_data.m_value.binary)op(*rhs.m_data.m_value.binary);                                     \
                                                                                                                       \
            case value_t::discarded:                                                                                   \
            default:                                                                                                   \
                return (unordered_result);                                                                             \
        }                                                                                                              \
    } else if(lhs_type == value_t::number_integer && rhs_type == value_t::number_float) {                              \
        return static_cast<number_float_t>(lhs.m_data.m_value.number_integer) op rhs.m_data.m_value.number_float;      \
    } else if(lhs_type == value_t::number_float && rhs_type == value_t::number_integer) {                              \
        return lhs.m_data.m_value.number_float op static_cast<number_float_t>(rhs.m_data.m_value.number_integer);      \
    } else if(lhs_type == value_t::number_unsigned && rhs_type == value_t::number_float) {                             \
        return static_cast<number_float_t>(lhs.m_data.m_value.number_unsigned) op rhs.m_data.m_value.number_float;     \
    } else if(lhs_type == value_t::number_float && rhs_type == value_t::number_unsigned) {                             \
        return lhs.m_data.m_value.number_float op static_cast<number_float_t>(rhs.m_data.m_value.number_unsigned);     \
    } else if(lhs_type == value_t::number_unsigned && rhs_type == value_t::number_integer) {                           \
        return static_cast<number_integer_t>(lhs.m_data.m_value.number_unsigned) op rhs.m_data.m_value.number_integer; \
    } else if(lhs_type == value_t::number_integer && rhs_type == value_t::number_unsigned) {                           \
        return lhs.m_data.m_value.number_integer op static_cast<number_integer_t>(rhs.m_data.m_value.number_unsigned); \
    } else if(compares_unordered(lhs, rhs)) {                                                                          \
        return (unordered_result);                                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    return (default_result);

    JSON_PRIVATE_UNLESS_TESTED:





        static bool compares_unordered(const_reference lhs, const_reference rhs, bool inverse = false) noexcept {
        if((lhs.is_number_float() && std::isnan(lhs.m_data.m_value.number_float) && rhs.is_number()) || (rhs.is_number_float() && std::isnan(rhs.m_data.m_value.number_float) && lhs.is_number())) {
            return true;
        }
#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
        return (lhs.is_discarded() || rhs.is_discarded()) && !inverse;
#else
        static_cast<void>(inverse);
        return lhs.is_discarded() || rhs.is_discarded();
#endif
    }

  private:
    bool compares_unordered(const_reference rhs, bool inverse = false) const noexcept {
        return compares_unordered(*this, rhs, inverse);
    }

  public:
#if JSON_HAS_THREE_WAY_COMPARISON


    bool operator==(const_reference rhs) const noexcept {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wfloat-equal"
#    endif
        const_reference lhs = *this;
        JSON_IMPLEMENT_OPERATOR(==, true, false, false)
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
    }



    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType>
    bool operator==(ScalarType rhs) const noexcept {
        return *this == basic_json(rhs);
    }



    bool operator!=(const_reference rhs) const noexcept {
        if(compares_unordered(rhs, true)) {
            return false;
        }
        return !operator==(rhs);
    }



    std::partial_ordering operator<=>(const_reference rhs) const noexcept
    {
        const_reference lhs = *this;


        JSON_IMPLEMENT_OPERATOR(<=>,
                                std::partial_ordering::equivalent, std::partial_ordering::unordered,
                                lhs_type <=> rhs_type)
    }



    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType>
    std::partial_ordering operator<=>(ScalarType rhs) const noexcept
    {
        return *this <=> basic_json(rhs);
    }

#    if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON





    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator<=(const_reference rhs) const noexcept {
        if(compares_unordered(rhs, true)) {
            return false;
        }
        return !(rhs < *this);
    }



    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType>
    bool operator<=(ScalarType rhs) const noexcept {
        return *this <= basic_json(rhs);
    }



    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator>=(const_reference rhs) const noexcept {
        if(compares_unordered(rhs, true)) {
            return false;
        }
        return !(*this < rhs);
    }



    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType>
    bool operator>=(ScalarType rhs) const noexcept {
        return *this >= basic_json(rhs);
    }
#    endif
#else


    friend bool operator==(const_reference lhs, const_reference rhs) noexcept {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wfloat-equal"
#    endif
        JSON_IMPLEMENT_OPERATOR(==, true, false, false)
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(const_reference lhs, ScalarType rhs) noexcept {
        return lhs == basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) == rhs;
    }



    friend bool operator!=(const_reference lhs, const_reference rhs) noexcept {
        if(compares_unordered(lhs, rhs, true)) {
            return false;
        }
        return !(lhs == rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(const_reference lhs, ScalarType rhs) noexcept {
        return lhs != basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) != rhs;
    }



    friend bool operator<(const_reference lhs, const_reference rhs) noexcept {



        JSON_IMPLEMENT_OPERATOR(<, false, false, operator<(lhs_type, rhs_type))
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(const_reference lhs, ScalarType rhs) noexcept {
        return lhs < basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) < rhs;
    }



    friend bool operator<=(const_reference lhs, const_reference rhs) noexcept {
        if(compares_unordered(lhs, rhs, true)) {
            return false;
        }
        return !(rhs < lhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(const_reference lhs, ScalarType rhs) noexcept {
        return lhs <= basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) <= rhs;
    }



    friend bool operator>(const_reference lhs, const_reference rhs) noexcept {

        if(compares_unordered(lhs, rhs)) {
            return false;
        }
        return !(lhs <= rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(const_reference lhs, ScalarType rhs) noexcept {
        return lhs > basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) > rhs;
    }



    friend bool operator>=(const_reference lhs, const_reference rhs) noexcept {
        if(compares_unordered(lhs, rhs, true)) {
            return false;
        }
        return !(lhs < rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(const_reference lhs, ScalarType rhs) noexcept {
        return lhs >= basic_json(rhs);
    }



    template<typename ScalarType, typename std::enable_if<std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(ScalarType lhs, const_reference rhs) noexcept {
        return basic_json(lhs) >= rhs;
    }
#endif

#undef JSON_IMPLEMENT_OPERATOR









#ifndef JSON_NO_IO


    friend std::ostream &operator<<(std::ostream &o, const basic_json &j) {

        const bool pretty_print = o.width() > 0;
        const auto indentation = pretty_print ? o.width() : 0;


        o.width(0);


        serializer s(detail::output_adapter<char>(o), o.fill());
        s.dump(j, pretty_print, false, static_cast<unsigned int>(indentation));
        return o;
    }







    JSON_HEDLEY_DEPRECATED_FOR(3.0.0, operator<<(std::ostream &, const basic_json &))
    friend std::ostream &operator>>(const basic_json &j, std::ostream &o) {
        return o << j;
    }
#endif











    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json parse(InputType &&i, const parser_callback_t cb = nullptr, const bool allow_exceptions = true, const bool ignore_comments = false) {
        basic_json result;
        parser(detail::input_adapter(std::forward<InputType>(i)), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json parse(IteratorType first, IteratorType last, const parser_callback_t cb = nullptr, const bool allow_exceptions = true, const bool ignore_comments = false) {
        basic_json result;
        parser(detail::input_adapter(std::move(first), std::move(last)), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, parse(ptr, ptr + len))
    static basic_json parse(detail::span_input_adapter &&i, const parser_callback_t cb = nullptr, const bool allow_exceptions = true, const bool ignore_comments = false) {
        basic_json result;
        parser(i.get(), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }



    template<typename InputType>
    static bool accept(InputType &&i, const bool ignore_comments = false) {
        return parser(detail::input_adapter(std::forward<InputType>(i)), nullptr, false, ignore_comments).accept(true);
    }



    template<typename IteratorType>
    static bool accept(IteratorType first, IteratorType last, const bool ignore_comments = false) {
        return parser(detail::input_adapter(std::move(first), std::move(last)), nullptr, false, ignore_comments).accept(true);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, accept(ptr, ptr + len))
    static bool accept(detail::span_input_adapter &&i, const bool ignore_comments = false) {
        return parser(i.get(), nullptr, false, ignore_comments).accept(true);
    }



    template<typename InputType, typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    static bool sax_parse(InputType &&i, SAX *sax, input_format_t format = input_format_t::json, const bool strict = true, const bool ignore_comments = false) {
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        return format == input_format_t::json
                       ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)
                       : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }



    template<class IteratorType, class SAX>
    JSON_HEDLEY_NON_NULL(3)
    static bool sax_parse(IteratorType first, IteratorType last, SAX *sax, input_format_t format = input_format_t::json, const bool strict = true, const bool ignore_comments = false) {
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        return format == input_format_t::json
                       ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)
                       : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }






    template<typename SAX>
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, sax_parse(ptr, ptr + len, ...))
    JSON_HEDLEY_NON_NULL(2) static bool sax_parse(detail::span_input_adapter &&i, SAX *sax, input_format_t format = input_format_t::json, const bool strict = true, const bool ignore_comments = false) {
        auto ia = i.get();
        return format == input_format_t::json

                       ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)

                       : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }
#ifndef JSON_NO_IO






    JSON_HEDLEY_DEPRECATED_FOR(3.0.0, operator>>(std::istream &, basic_json &))
    friend std::istream &operator<<(basic_json &j, std::istream &i) {
        return operator>>(i, j);
    }



    friend std::istream &operator>>(std::istream &i, basic_json &j) {
        parser(detail::input_adapter(i)).parse(false, j);
        return i;
    }
#endif








    JSON_HEDLEY_RETURNS_NON_NULL
    const char *type_name() const noexcept {
        switch(m_data.m_type) {
            case value_t::null:
                return "null";
            case value_t::object:
                return "object";
            case value_t::array:
                return "array";
            case value_t::string:
                return "string";
            case value_t::boolean:
                return "boolean";
            case value_t::binary:
                return "binary";
            case value_t::discarded:
                return "discarded";
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            default:
                return "number";
        }
    }

    JSON_PRIVATE_UNLESS_TESTED:




        struct data {

        value_t m_type = value_t::null;


        json_value m_value = {};

        data(const value_t v)
            : m_type(v), m_value(v) {
        }

        data(size_type cnt, const basic_json &val)
            : m_type(value_t::array) {
            m_value.array = create<array_t>(cnt, val);
        }

        data() noexcept = default;
        data(data &&) noexcept = default;
        data(const data &) noexcept = delete;
        data &operator=(data &&) noexcept = delete;
        data &operator=(const data &) noexcept = delete;

        ~data() noexcept {
            m_value.destroy(m_type);
        }
    };

    data m_data = {};

#if JSON_DIAGNOSTICS

    basic_json *m_parent = nullptr;
#endif








  public:


    static std::vector<std::uint8_t> to_cbor(const basic_json &j) {
        std::vector<std::uint8_t> result;
        to_cbor(j, result);
        return result;
    }



    static void to_cbor(const basic_json &j, detail::output_adapter<std::uint8_t> o) {
        binary_writer<std::uint8_t>(o).write_cbor(j);
    }



    static void to_cbor(const basic_json &j, detail::output_adapter<char> o) {
        binary_writer<char>(o).write_cbor(j);
    }



    static std::vector<std::uint8_t> to_msgpack(const basic_json &j) {
        std::vector<std::uint8_t> result;
        to_msgpack(j, result);
        return result;
    }



    static void to_msgpack(const basic_json &j, detail::output_adapter<std::uint8_t> o) {
        binary_writer<std::uint8_t>(o).write_msgpack(j);
    }



    static void to_msgpack(const basic_json &j, detail::output_adapter<char> o) {
        binary_writer<char>(o).write_msgpack(j);
    }



    static std::vector<std::uint8_t> to_ubjson(const basic_json &j, const bool use_size = false, const bool use_type = false) {
        std::vector<std::uint8_t> result;
        to_ubjson(j, result, use_size, use_type);
        return result;
    }



    static void to_ubjson(const basic_json &j, detail::output_adapter<std::uint8_t> o, const bool use_size = false, const bool use_type = false) {
        binary_writer<std::uint8_t>(o).write_ubjson(j, use_size, use_type);
    }



    static void to_ubjson(const basic_json &j, detail::output_adapter<char> o, const bool use_size = false, const bool use_type = false) {
        binary_writer<char>(o).write_ubjson(j, use_size, use_type);
    }



    static std::vector<std::uint8_t> to_bjdata(const basic_json &j, const bool use_size = false, const bool use_type = false) {
        std::vector<std::uint8_t> result;
        to_bjdata(j, result, use_size, use_type);
        return result;
    }



    static void to_bjdata(const basic_json &j, detail::output_adapter<std::uint8_t> o, const bool use_size = false, const bool use_type = false) {
        binary_writer<std::uint8_t>(o).write_ubjson(j, use_size, use_type, true, true);
    }



    static void to_bjdata(const basic_json &j, detail::output_adapter<char> o, const bool use_size = false, const bool use_type = false) {
        binary_writer<char>(o).write_ubjson(j, use_size, use_type, true, true);
    }



    static std::vector<std::uint8_t> to_bson(const basic_json &j) {
        std::vector<std::uint8_t> result;
        to_bson(j, result);
        return result;
    }



    static void to_bson(const basic_json &j, detail::output_adapter<std::uint8_t> o) {
        binary_writer<std::uint8_t>(o).write_bson(j);
    }



    static void to_bson(const basic_json &j, detail::output_adapter<char> o) {
        binary_writer<char>(o).write_bson(j);
    }



    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_cbor(InputType &&i, const bool strict = true, const bool allow_exceptions = true, const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_cbor(IteratorType first, IteratorType last, const bool strict = true, const bool allow_exceptions = true, const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_cbor(ptr, ptr + len)) static basic_json from_cbor(const T *ptr, std::size_t len, const bool strict = true, const bool allow_exceptions = true, const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error) {
        return from_cbor(ptr, ptr + len, strict, allow_exceptions, tag_handler);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_cbor(ptr, ptr + len))
    static basic_json from_cbor(detail::span_input_adapter &&i, const bool strict = true, const bool allow_exceptions = true, const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();

        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_msgpack(InputType &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_msgpack(IteratorType first, IteratorType last, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_msgpack(ptr, ptr + len)) static basic_json from_msgpack(const T *ptr, std::size_t len, const bool strict = true, const bool allow_exceptions = true) {
        return from_msgpack(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_msgpack(ptr, ptr + len))
    static basic_json from_msgpack(detail::span_input_adapter &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();

        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_ubjson(InputType &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_ubjson(IteratorType first, IteratorType last, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_ubjson(ptr, ptr + len)) static basic_json from_ubjson(const T *ptr, std::size_t len, const bool strict = true, const bool allow_exceptions = true) {
        return from_ubjson(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_ubjson(ptr, ptr + len))
    static basic_json from_ubjson(detail::span_input_adapter &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();

        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_bjdata(InputType &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bjdata).sax_parse(input_format_t::bjdata, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_bjdata(IteratorType first, IteratorType last, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bjdata).sax_parse(input_format_t::bjdata, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_bson(InputType &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }



    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT static basic_json from_bson(IteratorType first, IteratorType last, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_bson(ptr, ptr + len)) static basic_json from_bson(const T *ptr, std::size_t len, const bool strict = true, const bool allow_exceptions = true) {
        return from_bson(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_bson(ptr, ptr + len))
    static basic_json from_bson(detail::span_input_adapter &&i, const bool strict = true, const bool allow_exceptions = true) {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();

        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }











    reference operator[](const json_pointer &ptr) {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    reference operator[](const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr) {
        return ptr.get_unchecked(this);
    }



    const_reference operator[](const json_pointer &ptr) const {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    const_reference operator[](const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr) const {
        return ptr.get_unchecked(this);
    }



    reference at(const json_pointer &ptr) {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    reference at(const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr) {
        return ptr.get_checked(this);
    }



    const_reference at(const json_pointer &ptr) const {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or silicon::json::impl::json_pointer<basic_json::string_t>)
    const_reference at(const ::silicon::json::impl::json_pointer<BasicJsonType> &ptr) const {
        return ptr.get_checked(this);
    }



    basic_json flatten() const {
        basic_json result(value_t::object);
        json_pointer::flatten("", *this, result);
        return result;
    }



    basic_json unflatten() const {
        return json_pointer::unflatten(*this);
    }












    void patch_inplace(const basic_json &json_patch) {
        basic_json &result = *this;

        enum class patch_operations { add,
                                      remove,
                                      replace,
                                      move,
                                      copy,
                                      test,
                                      invalid };

        const auto get_op = [](const std::string &op) {
            if(op == "add") {
                return patch_operations::add;
            }
            if(op == "remove") {
                return patch_operations::remove;
            }
            if(op == "replace") {
                return patch_operations::replace;
            }
            if(op == "move") {
                return patch_operations::move;
            }
            if(op == "copy") {
                return patch_operations::copy;
            }
            if(op == "test") {
                return patch_operations::test;
            }

            return patch_operations::invalid;
        };


        const auto operation_add = [&result](json_pointer &ptr, basic_json val) {

            if(ptr.empty()) {
                result = val;
                return;
            }


            json_pointer const top_pointer = ptr.top();
            if(top_pointer != ptr) {
                result.at(top_pointer);
            }


            const auto last_path = ptr.back();
            ptr.pop_back();

            basic_json &parent = result.at(ptr);

            switch(parent.m_data.m_type) {
                case value_t::null:
                case value_t::object: {

                    parent[last_path] = val;
                    break;
                }

                case value_t::array: {
                    if(last_path == "-") {

                        parent.push_back(val);
                    } else {
                        const auto idx = json_pointer::template array_index<basic_json_t>(last_path);
                        if(JSON_HEDLEY_UNLIKELY(idx > parent.size())) {

                            JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), &parent));
                        }


                        parent.insert(parent.begin() + static_cast<difference_type>(idx), val);
                    }
                    break;
                }


                case value_t::string:
                case value_t::boolean:
                case value_t::number_integer:
                case value_t::number_unsigned:
                case value_t::number_float:
                case value_t::binary:
                case value_t::discarded:
                default:
                    JSON_ASSERT(false);
            }
        };


        const auto operation_remove = [this, &result](json_pointer &ptr) {

            const auto last_path = ptr.back();
            ptr.pop_back();
            basic_json &parent = result.at(ptr);


            if(parent.is_object()) {

                auto it = parent.find(last_path);
                if(JSON_HEDLEY_LIKELY(it != parent.end())) {
                    parent.erase(it);
                } else {
                    JSON_THROW(out_of_range::create(403, detail::concat("key '", last_path, "' not found"), this));
                }
            } else if(parent.is_array()) {

                parent.erase(json_pointer::template array_index<basic_json_t>(last_path));
            }
        };


        if(JSON_HEDLEY_UNLIKELY(!json_patch.is_array())) {
            JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &json_patch));
        }


        for(const auto &val: json_patch) {

            const auto get_value = [&val](const std::string &op,
                                          const std::string &member,
                                          bool string_type) -> basic_json & {

                auto it = val.m_data.m_value.object->find(member);


                const auto error_msg = (op == "op") ? "operation" : detail::concat("operation '", op, '\'');


                if(JSON_HEDLEY_UNLIKELY(it == val.m_data.m_value.object->end())) {

                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have member '", member, "'"), &val));
                }


                if(JSON_HEDLEY_UNLIKELY(string_type && !it->second.is_string())) {

                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have string member '", member, "'"), &val));
                }


                return it->second;
            };


            if(JSON_HEDLEY_UNLIKELY(!val.is_object())) {
                JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &val));
            }


            const auto op = get_value("op", "op", true).template get<std::string>();
            const auto path = get_value(op, "path", true).template get<std::string>();
            json_pointer ptr(path);

            switch(get_op(op)) {
                case patch_operations::add: {
                    operation_add(ptr, get_value("add", "value", false));
                    break;
                }

                case patch_operations::remove: {
                    operation_remove(ptr);
                    break;
                }

                case patch_operations::replace: {

                    result.at(ptr) = get_value("replace", "value", false);
                    break;
                }

                case patch_operations::move: {
                    const auto from_path = get_value("move", "from", true).template get<std::string>();
                    json_pointer from_ptr(from_path);


                    basic_json const v = result.at(from_ptr);





                    operation_remove(from_ptr);
                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::copy: {
                    const auto from_path = get_value("copy", "from", true).template get<std::string>();
                    const json_pointer from_ptr(from_path);


                    basic_json const v = result.at(from_ptr);




                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::test: {
                    bool success = false;
                    JSON_TRY {


                        success = (result.at(ptr) == get_value("test", "value", false));
                    }
                    JSON_INTERNAL_CATCH(out_of_range &) {

                    }


                    if(JSON_HEDLEY_UNLIKELY(!success)) {
                        JSON_THROW(other_error::create(501, detail::concat("unsuccessful: ", val.dump()), &val));
                    }

                    break;
                }

                case patch_operations::invalid:
                default: {


                    JSON_THROW(parse_error::create(105, 0, detail::concat("operation value '", op, "' is invalid"), &val));
                }
            }
        }
    }



    basic_json patch(const basic_json &json_patch) const {
        basic_json result = *this;
        result.patch_inplace(json_patch);
        return result;
    }



    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json diff(const basic_json &source, const basic_json &target, const std::string &path = "") {

        basic_json result(value_t::array);


        if(source == target) {
            return result;
        }

        if(source.type() != target.type()) {

            result.push_back(
                    {{"op", "replace"}, {"path", path}, {"value", target}}
            );
            return result;
        }

        switch(source.type()) {
            case value_t::array: {

                std::size_t i = 0;
                while(i < source.size() && i < target.size()) {

                    auto temp_diff = diff(source[i], target[i], detail::concat(path, '/', std::to_string(i)));
                    result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    ++i;
                }





                const auto end_index = static_cast<difference_type>(result.size());
                while(i < source.size()) {


                    result.insert(result.begin() + end_index, object({{"op", "remove"}, {"path", detail::concat(path, '/', std::to_string(i))}}));
                    ++i;
                }


                while(i < target.size()) {
                    result.push_back(
                            {{"op", "add"},
                             {"path", detail::concat(path, "/-")},
                             {"value", target[i]}}
                    );
                    ++i;
                }

                break;
            }

            case value_t::object: {

                for(auto it = source.cbegin(); it != source.cend(); ++it) {

                    const auto path_key = detail::concat(path, '/', detail::escape(it.key()));

                    if(target.find(it.key()) != target.end()) {

                        auto temp_diff = diff(it.value(), target[it.key()], path_key);
                        result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    } else {

                        result.push_back(object(
                                {{"op", "remove"}, {"path", path_key}}
                        ));
                    }
                }


                for(auto it = target.cbegin(); it != target.cend(); ++it) {
                    if(source.find(it.key()) == source.end()) {

                        const auto path_key = detail::concat(path, '/', detail::escape(it.key()));
                        result.push_back(
                                {{"op", "add"}, {"path", path_key}, {"value", it.value()}}
                        );
                    }
                }

                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {

                result.push_back(
                        {{"op", "replace"}, {"path", path}, {"value", target}}
                );
                break;
            }
        }

        return result;
    }











    void merge_patch(const basic_json &apply_patch) {
        if(apply_patch.is_object()) {
            if(!is_object()) {
                *this = object();
            }
            for(auto it = apply_patch.begin(); it != apply_patch.end(); ++it) {
                if(it.value().is_null()) {
                    erase(it.key());
                } else {
                    operator[](it.key()).merge_patch(it.value());
                }
            }
        } else {
            *this = apply_patch;
        }
    }


};



silicon_BASIC_JSON_TPL_DECLARATION
        std::string
        to_string(const silicon_BASIC_JSON_TPL &j) {
    return j.dump();
}

inline namespace literals {
inline namespace json_literals {



JSON_HEDLEY_NON_NULL(1)
#if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
inline silicon::json::impl::json operator""_json(const char *s, std::size_t n)
#else
inline silicon::json::impl::json operator"" _json(const char *s, std::size_t n)
#endif
{
    return silicon::json::impl::json::parse(s, s + n);
}



JSON_HEDLEY_NON_NULL(1)
#if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
inline silicon::json::impl::json::json_pointer operator""_json_pointer(const char *s, std::size_t n)
#else
inline silicon::json::impl::json::json_pointer operator"" _json_pointer(const char *s, std::size_t n)
#endif
{
    return silicon::json::impl::json::json_pointer(std::string(s, n));
}

}
}
SILICON_JSON_NAMESPACE_END
}





namespace std
{



silicon_BASIC_JSON_TPL_DECLARATION struct hash<silicon::json::impl::silicon_BASIC_JSON_TPL>
{
    std::size_t operator()(const silicon::json::impl::silicon_BASIC_JSON_TPL &j) const {
        return silicon::json::impl::detail::hash(j);
    }
};


template<>
struct less<::silicon::json::impl::detail::value_t>
{
    
    bool operator()(::silicon::json::impl::detail::value_t lhs, ::silicon::json::impl::detail::value_t rhs) const noexcept {
#if JSON_HAS_THREE_WAY_COMPARISON
        return std::is_lt(lhs <=> rhs);
#else
        return ::silicon::json::impl::detail::operator<(lhs, rhs);
#endif
    }
};


#ifndef JSON_HAS_CPP_20



silicon_BASIC_JSON_TPL_DECLARATION inline void swap(silicon::json::impl::silicon_BASIC_JSON_TPL &j1, silicon::json::impl::silicon_BASIC_JSON_TPL &j2) noexcept(
        is_nothrow_move_constructible<silicon::json::impl::silicon_BASIC_JSON_TPL>::value &&
        is_nothrow_move_assignable<silicon::json::impl::silicon_BASIC_JSON_TPL>::value
) {
    j1.swap(j2);
}

#endif

}

#if JSON_USE_GLOBAL_UDLS
#    if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
using silicon::json::impl::literals::json_literals::operator""_json;
using silicon::json::impl::literals::json_literals::operator""_json_pointer;
#    else
using silicon::json::impl::literals::json_literals::operator"" _json;
using silicon::json::impl::literals::json_literals::operator"" _json_pointer;
#    endif
#endif





export namespace silicon::json {
using json = silicon::json::impl::json;
}

export using silicon::json::json;


export namespace silicon::json {
using json_value = json;
}


export namespace silicon::json {
inline json parse(std::string_view text) {
    return json::parse(text, nullptr, false);
}
inline std::string serialize(const json &value) {
    return value.dump();
}
}

