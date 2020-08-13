#pragma once

#include "types.hpp"
#include "ston.hpp"

namespace core
{
class args_view {
public:
    args_view() = delete;

    args_view(int argc, char** argv) {
        _name = argv[0];
        std::copy(argv + 1, argv + argc, std::back_inserter(_data));
    }

    [[nodiscard]]
    string_view program_name() const {
        return _name;
    }

    bool get(string_view name) {
        auto found = std::find(_data.begin(), _data.end(), name);
        return found != _data.end() ? _data.erase(found), true : false;
    }

    template <typename T>
    static string_view type_to_str() {
        if constexpr (std::is_floating_point_v<T>)
            return "float";
        else if constexpr (std::is_signed_v<T>)
            return "integer";
        else if constexpr (std::is_unsigned_v<T>)
            return "unsigned_integer";
        else if constexpr (std::is_same_v<T, string>)
            return "string";
        else
            return "unknown";
    }

    template <typename T = string>
    optional<T> by_key_opt(string_view name) {
        auto starts_with = [&](string_view v) { return v.starts_with(name); };
        auto found = std::find_if(_data.begin(), _data.end(), starts_with);

        if (found != _data.end()) {
            try {
                if (*found != name && found->substr(name.length()).front() == '=') {
                    auto str = static_cast<string>(found->substr(name.length() + 1));
                    _data.erase(found);

                    if constexpr (std::is_integral_v<T>)
                        return ston<T>(str);
                    else
                        return str;
                }
                else if (std::next(found) != _data.end()) {
                    auto str = static_cast<string>(*std::next(found));
                    _data.erase(found, std::next(found, 2));

                    if constexpr (std::is_integral_v<T>)
                        return ston<T>(str);
                    else
                        return str;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Argument of '" << *found << "'"
                          << " must be a " << type_to_str<T>() << std::endl;
            }
        }

        return std::nullopt;
    }

    template <typename T = string>
    optional<T> by_key_opt(initializer_list<string_view> name_variants) {
        for (auto& name : name_variants)
            if (auto val = by_key_opt<T>(name))
                return *val;

        return std::nullopt;
    }

    template <typename T = string>
    T by_key_require(string_view name, string error_message = {}) {
        if (auto arg = by_key_opt<T>(name))
            return *arg;
        else {
            if (error_message.empty())
                error_message = "Missing option " + string(name) + "=" + string(type_to_str<T>());
            throw std::invalid_argument(error_message);
        }
    }

    template <typename T = string>
    T by_key_require(initializer_list<string_view> name_variants, string error_message = {}) {
        for (auto& name : name_variants)
            if (auto num = by_key_opt<T>(name))
                return *num;

        if (error_message.empty()) {
            error_message = "Missing option ";
            for (auto& name : name_variants) {
                error_message += string(name) + "=" + string(type_to_str<T>());
                break;
            }
        }
        throw std::invalid_argument(error_message);
    }

    template <typename T = string>
    T by_key_default(string_view name, T default_val) {
        if (auto arg = by_key_opt<T>(name))
            return *arg;
        else
            return default_val;
    }

    template <typename T = string>
    T by_key_default(initializer_list<string_view> name_variants, T default_val) {
        for (auto& name : name_variants)
            if (auto num = by_key_opt<T>(name))
                return *num;

        return default_val;
    }

    [[nodiscard]]
    size_t size() const {
        return _data.size();
    }

    [[nodiscard]]
    bool empty() const {
        return _data.empty();
    }

    optional<string> next() {
        if (!empty()) {
            auto res = _data.front();
            _data.pop_front();
            return string(res);
        }
        return {};
    }

    string next_unwrap(string_view error_message) {
        if (auto arg = next())
            return *arg;
        else
            throw std::invalid_argument(string(error_message));
    }

    string next_default(string_view default_val) {
        if (auto arg = next())
            return *arg;
        else
            return string(default_val);
    }

    void require_end(string_view error_message = {}) {
        if (!_data.empty()) {
            if (error_message.empty())
                throw std::runtime_error("Unknown option "s + string(_data.front()));
            else
                throw std::runtime_error(string(error_message));
        }
    }

private:
    string_view       _name;
    list<string_view> _data;
};

} // namespace core

