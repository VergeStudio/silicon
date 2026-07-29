#pragma once

import silicon.coroutine;

namespace silicon::network {

/// @brief Abstract interface for a network socket.
///
/// The concrete socket class (silicon::network::socket) implements this
/// interface. socket retains its value semantics (copy via dup, move, etc.)
/// and the virtual destructor is a small extra cost for a thin fd wrapper.
///
/// Usage in DI:
///   c.bind<ISocket>().to<socket>(di::in_unique);
class ISocket {
  public:
    ISocket() = default;
    ISocket(const ISocket &) = delete;
    ISocket(ISocket &&) = delete;
    auto operator=(const ISocket &) -> ISocket & = delete;
    auto operator=(ISocket &&) -> ISocket & = delete;

    virtual ~ISocket() = default;

    /// @brief Returns true if the socket's fd is valid.
    [[nodiscard]] virtual auto is_ok() const -> bool = 0;

    /// @brief Sets the socket to the given blocking mode.
    /// @param block blocking_t::yes or blocking_t::no
    /// @return true on success.
    virtual auto blocking(int block) -> bool = 0;

    /// @brief Shuts the socket down with the given operations.
    virtual auto shutdown(int how) -> bool = 0;

    /// @brief Closes the socket and sets it to an invalid state.
    virtual auto close() -> void = 0;

    /// @brief Returns the native handle (file descriptor).
    [[nodiscard]] virtual auto native_handle() const -> int = 0;
};

} // namespace silicon::network
