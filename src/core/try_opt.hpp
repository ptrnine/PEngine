#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace core
{
/**
 * @brief Same as std::optional, but can store exception_ptr for detailed error message
 *
 * @tparam T - type of stored value
 */
template <typename T>
class try_opt {
public:
    static_assert(!std::is_base_of_v<std::exception, T>,
                  "Constructing try_opt from exception require explicit type specifying");

    try_opt() = default;

    template <typename E> requires std::is_base_of_v<std::exception, E>
    try_opt(E exception) noexcept:
        _exception_ptr(std::make_exception_ptr(exception)) {}

    try_opt(std::exception_ptr ptr): _exception_ptr(std::move(ptr)) {}

    try_opt(const T& value): _opt(value) {}
    try_opt(T&& value) noexcept(noexcept(T(std::declval<T>()))): _opt(std::move(value)) {}

    /**
     * @brief Access to stored value
     *
     * Throws std::runtime_error if has no value or throws stored exception_ptr
     *
     * @return Reference to stored value
     */
    T& value() & {
        if (!has_value()) {
            if (_exception_ptr)
                std::rethrow_exception(_exception_ptr);
            else
                throw std::runtime_error("try_opt was not set");
        }

        return *_opt;
    }

    /**
     * @brief Access to stored value
     *
     * Throws std::runtime_error if has no value or throws stored exception_ptr
     *
     * @return Reference to stored value
     */
    const T& value() const& {
        if (!has_value()) {
            if (_exception_ptr)
                std::rethrow_exception(_exception_ptr);
            else
                throw std::runtime_error("try_opt was not set");
        }

        return *_opt;
    }

    /**
     * @brief Access to stored value
     *
     * Throws std::runtime_error if has no value or throws stored exception_ptr
     *
     * @return Reference to stored value
     */
    T&& value() && {
        if (!has_value()) {
            if (_exception_ptr)
                std::rethrow_exception(_exception_ptr);
            else
                throw std::runtime_error("try_opt was not set");
        }

        return std::move(*_opt);
    }

    /**
     * @brief Access to stored value
     *
     * Throws std::runtime_error if has no value or throws stored exception_ptr
     *
     * @return Reference to stored value
     */
    const T&& value() const&& {
        if (!has_value()) {
            if (_exception_ptr)
                std::rethrow_exception(_exception_ptr);
            else
                throw std::runtime_error("try_opt was not set");
        }

        return std::move(*_opt);
    }

    /**
     * @brief Reset stored value or exception
     */
    void reset() noexcept {
        _opt.reset();
        _exception_ptr = nullptr;
    }

    /**
     * @brief Construct the contained value in-place
     *
     * Replace already existed value
     *
     * @tparam Ts - types of arguments
     * @param args - arguments to pass to the constructor
     *
     * @return reference to the new contained value
     */
    template <typename... Ts>
    T& emplace(Ts&&... args) {
        _exception_ptr = nullptr;
        return _opt.emplace(std::forward<Ts>(args)...);
    }

    /**
     * @brief Construct the contained value in-place
     *
     * @tparam U - the type of initializer_list value
     * @tparam Ts - types of arguments
     * @param ilist - the initializer_list to pass to the constructor
     * @param args - arguments to pass to the constructor
     *
     * @return reference to the new contained value
     */
    template <typename U, typename... Ts>
    T& emplace(std::initializer_list<U> ilist, Ts&&... args) {
        _exception_ptr = nullptr;
        return _opt.emplace(ilist, std::forward<Ts>(args)...);
    }

    [[nodiscard]]
    const std::exception_ptr& exception_ptr() const noexcept {
        return _exception_ptr;
    }

    /**
     * @brief Checks whether *this contains a value
     *
     * @return true if *this contains a value, otherwise false
     */
    [[nodiscard]]
    constexpr bool has_value() const noexcept {
        return _opt.has_value();
    }
    template <typename F, typename R = std::invoke_result_t<F, T>>
    [[nodiscard]]
    constexpr try_opt<R> map(F callback) const {
        return has_value() ? try_opt<R>(callback(*_opt)) : try_opt<R>(_exception_ptr);
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr T* operator->() {
        return _opt.operator->();
    }

    constexpr const T* operator->() const {
        return _opt.operator->();
    }

    constexpr T& operator*() & {
        return _opt.operator*();
    }

    constexpr T&& operator*() && {
        return std::move(_opt.operator*());
    }

    constexpr const T& operator*() const& {
        return _opt.operator*();
    }

    constexpr const T&& operator*() const&& {
        return std::move(_opt.operator*());
    }

private:
    std::optional<T>   _opt;
    std::exception_ptr _exception_ptr;
};
} // namespace core

