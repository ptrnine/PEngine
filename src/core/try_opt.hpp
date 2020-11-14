#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <memory>

namespace core
{

class thrower_base {
public:
    virtual ~thrower_base() = default;
    virtual void throw_exception() const = 0;
    [[nodiscard]] virtual const std::string& error_message() const = 0;
    [[nodiscard]] virtual std::unique_ptr<thrower_base> get_copy() const = 0;
};

template <typename E>
class thrower : public thrower_base {
public:
    thrower() = default;
    thrower(std::string message): msg(move(message)) {}

    void throw_exception() const override {
        throw E(msg);
    }

    [[nodiscard]]
    const std::string& error_message() const override {
        return msg;
    }

    [[nodiscard]]
    std::unique_ptr<thrower_base> get_copy() const override {
        return std::make_unique<thrower<E>>(msg);
    }

private:
    std::string msg;
};


/**
 * @brief Same as std::optional, but can store exception_ptr for detailed error message
 *
 * @tparam T - type of stored value
 */
template <typename T>
class try_opt {
public:
    using value_type = T;

    try_opt() = default;
    ~try_opt() = default;

    try_opt(const try_opt& opt): _opt(opt._opt) {
        if (opt._thrower)
            _thrower = opt._thrower->get_copy();
    }

    try_opt& operator=(const try_opt& opt) {
        _opt = opt._opt;
        if (opt._thrower)
            _thrower = opt._thrower->get_copy();
    }

    try_opt(try_opt&& opt) noexcept = default;
    try_opt& operator=(try_opt&& opt) noexcept = default;

    try_opt(std::unique_ptr<thrower_base>&& thrower_): _thrower(std::move(thrower_)) {}

    try_opt(const T& value): _opt(value) {}
    try_opt(T&& value) noexcept(noexcept(T(std::declval<T>()))): _opt(std::move(value)) {}

    template <typename ExceptT> requires std::is_base_of_v<std::exception, ExceptT>
    try_opt(ExceptT exception_obj):
        _thrower(std::make_unique<thrower<ExceptT>>(exception_obj.what())) {}

    /**
     * @brief Access to stored value
     *
     * Throws std::runtime_error if has no value or throws stored exception_ptr
     *
     * @return Reference to stored value
     */
    T& value() & {
        if (!has_value()) {
            if (_thrower)
                _thrower->throw_exception();
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
            if (_thrower)
                _thrower->throw_exception();
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
            if (_thrower)
                _thrower->throw_exception();
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
            if (_thrower)
                _thrower->throw_exception();
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
        _thrower = nullptr;
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
        _thrower = nullptr;
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
        _thrower = nullptr;
        return _opt.emplace(ilist, std::forward<Ts>(args)...);
    }

    /**
     * @brief Returns pointer to exception thrower or nullptr
     *
     * @return pointer to exception thrower or nullptr
     */
    [[nodiscard]]
    std::unique_ptr<thrower_base> thrower_ptr() const noexcept {
        if (_thrower)
            return _thrower->get_copy();
        else
            return nullptr;
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
        if (has_value())
            return try_opt<R>(callback(*_opt));
        else if (_thrower)
            return try_opt<R>(_thrower->get_copy());
        else
            return try_opt<R>();
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
    std::optional<T>              _opt;
    std::unique_ptr<thrower_base> _thrower;
};
} // namespace core

