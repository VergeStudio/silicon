module;

// C++23: std::generator replaces custom implementation when available.
// <version> is required so the __cpp_lib_generator feature macro is visible
// before the branch is taken (a bare defined() check would silently fall
// back to the custom implementation).
#include <version>

#if __has_include(<generator>) && defined(__cpp_lib_generator)
#include <generator>
#else
// Fallback: custom generator (for compilers without std::generator support)
#include <coroutine>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#endif

export module silicon.coroutine:generator;

#if __has_include(<generator>) && defined(__cpp_lib_generator)

export namespace silicon::coroutine {
template <typename T>
using generator = std::generator<T>;
} // namespace silicon::coroutine

#else

export namespace silicon::coroutine {
template <typename T>
class generator;


template <typename T>
class generator_promise {
  public:
    using value_type = std::remove_reference_t<T>;
    using reference_type = std::conditional_t<std::is_reference_v<T>, T, T &>;
    using pointer_type = value_type *;

    generator_promise() = default;

    auto get_return_object() noexcept -> generator<T>;

    constexpr auto initial_suspend() const noexcept { return std::suspend_always{}; }

    constexpr auto final_suspend() const noexcept { return std::suspend_always{}; }

    template <typename U = T, std::enable_if_t<!std::is_rvalue_reference<U>::value, int> = 0>
    auto yield_value(std::remove_reference_t<T> &value) noexcept {
        m_value = std::addressof(value);
        return std::suspend_always{};
    }

    auto yield_value(std::remove_reference_t<T> &&value) noexcept {
        m_value = std::addressof(value);
        return std::suspend_always{};
    }

    [[noreturn]] void unhandled_exception() { throw; }

    auto return_void() noexcept -> void {}

    auto value() const noexcept -> reference_type { return static_cast<reference_type>(*m_value); }

    auto rethrow_if_exception() -> void {
        if (m_exception) {
            std::rethrow_exception(m_exception);
        }
    }

  private:
    pointer_type m_value{nullptr};
    std::exception_ptr m_exception;
};

struct generator_sentinel {};

template <typename T>
class generator_iterator {
    using coroutine_handle = std::coroutine_handle<generator_promise<T>>;

  public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = typename generator_promise<T>::value_type;
    using reference = typename generator_promise<T>::reference_type;
    using pointer = typename generator_promise<T>::pointer_type;

    generator_iterator() noexcept = default;
    explicit generator_iterator(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}

    friend auto operator==(const generator_iterator &it, generator_sentinel) noexcept -> bool {
        return it.m_coroutine == nullptr || it.m_coroutine.done();
    }
    friend auto operator==(generator_sentinel s, const generator_iterator &it) noexcept -> bool { return it == s; }

    generator_iterator &operator++() {
        m_coroutine.resume();
        if (m_coroutine.done()) {
            m_coroutine.promise().rethrow_if_exception();
        }
        return *this;
    }

    void operator++(int) { (void)operator++(); }

    reference operator*() const noexcept { return m_coroutine.promise().value(); }
    pointer operator->() const noexcept { return std::addressof(operator*()); }

  private:
    coroutine_handle m_coroutine{nullptr};
};



template <typename T>
class generator : public std::ranges::view_base {
  public:
    using promise_type = generator_promise<T>;
    using iterator = generator_iterator<T>;
    using sentinel = generator_sentinel;

    generator() noexcept : m_coroutine(nullptr) {}
    generator(const generator &) = delete;
    generator(generator &&other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

    auto operator=(const generator &) = delete;
    auto operator=(generator &&other) noexcept -> generator & {
        if (std::addressof(other) != this) {
            if (m_coroutine) {
                m_coroutine.destroy();
            }
            m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }
        return *this;
    }

    ~generator() {
        if (m_coroutine) {
            m_coroutine.destroy();
        }
    }

    auto begin() -> iterator {
        if (m_coroutine != nullptr) {
            m_coroutine.resume();
            if (m_coroutine.done()) {
                m_coroutine.promise().rethrow_if_exception();
            }
        }
        return iterator{m_coroutine};
    }

    auto end() noexcept -> sentinel { return sentinel{}; }

  private:
    friend class generator_promise<T>;
    explicit generator(std::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}

    std::coroutine_handle<promise_type> m_coroutine;
};


template <typename T>
auto generator_promise<T>::get_return_object() noexcept -> generator<T> {
    return generator<T>{std::coroutine_handle<generator_promise<T>>::from_promise(*this)};
}


} // namespace silicon::coroutine

#endif
