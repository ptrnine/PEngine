#pragma once

#include "types.hpp"

namespace core {
    template <typename T, Enum E, size_t S>
    class option_array {
    public:
        template <E option>
        void set(optional<T> value) {
            std::get<static_cast<std::underlying_type_t<E>>(option)>(_options) = move(value);
        }

        template <E option>
        void set(T value) {
            std::get<static_cast<std::underlying_type_t<E>>(option)>(_options) = move(value);
        }

        template <E option>
        [[nodiscard]]
        optional<T> try_get() const {
            return std::get<static_cast<std::underlying_type_t<E>>(option)>(_options);
        }

        template <E option>
        const T& get() const {
            return std::get<static_cast<std::underlying_type_t<E>>(option)>(_options).value();
        }

        template <E option>
        T& get() {
            return std::get<static_cast<std::underlying_type_t<E>>(option)>(_options).value();
        }

        template <E option>
        [[nodiscard]]
        const T* get_ptr() const {
            if (auto& opt = std::get<static_cast<std::underlying_type_t<E>>(option)>(_options))
                return &(*opt);
            else
                return nullptr;
        }

        template <E option>
        [[nodiscard]]
        T* get_ptr() const {
            if (auto& opt = std::get<static_cast<std::underlying_type_t<E>>(option)>(_options))
                return &(*opt);
            else
                return nullptr;
        }

        void set(E option, T value) {
            _options.at(static_cast<std::underlying_type_t<E>>(option)) = move(value);
        }

        void set(E option, optional<T> value) {
            _options.at(static_cast<std::underlying_type_t<E>>(option)) = move(value);
        }

        [[nodiscard]]
        optional<T> try_get(E option) const {
            return _options.at(static_cast<std::underlying_type_t<E>>(option));
        }

        const T& get(E option) const {
            return _options.at(static_cast<std::underlying_type_t<E>>(option)).value();
        }

        T& get(E option) {
            return _options.at(static_cast<std::underlying_type_t<E>>(option)).value();
        }

        [[nodiscard]]
        const T* get_ptr(E option) const {
            if (auto& opt = _options.at(static_cast<std::underlying_type_t<E>>(option)))
                return &(*opt);
            else
                return nullptr;
        }

        [[nodiscard]]
        T* get_ptr(E option) {
            if (auto& opt = _options.at(static_cast<std::underlying_type_t<E>>(option)))
                return &(*opt);
            else
                return nullptr;
        }

        [[nodiscard]]
        const array<optional<T>, S>& options() const {
            return _options;
        }

        [[nodiscard]]
        array<optional<T>, S>& options() {
            return _options;
        }

    private:
        array<optional<T>, S> _options;
    };
}
