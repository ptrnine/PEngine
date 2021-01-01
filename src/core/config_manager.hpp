#pragma once

#include <filesystem>
#include "assert.hpp"
#include "container_extensions.hpp"
#include "files.hpp"
#include "log.hpp"
#include "platform_dependent.hpp"
#include "ston.hpp"
#include "types.hpp"
#include "serialization.hpp"

namespace core
{
class config_exception : public std::exception {
public:
    config_exception(string message): msg(move(message)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return msg.data();
    }

private:
    core::string msg;
};

struct no_quotes_iterator {
    no_quotes_iterator(string_view::iterator beg, string_view::iterator en): begin(beg), end(en) {
        if (!skip('"'))
            skip('\'');
    }

    bool skip(char quote) {
        if (begin != end && *begin == quote) {
            ++begin;
            ++idx;
            for (; begin != end && *begin != quote; ++begin, ++idx)
                ;

            if (begin == end)
                throw std::runtime_error("Missing closing bracket ["s + quote + "]");

            ++begin;
            ++idx;
            return true;
        }
        else {
            return false;
        }
    }

    no_quotes_iterator& operator++() {
        if (begin != end) {
            ++begin;
            ++idx;
        }

        if (!skip('"'))
            skip('\'');

        return *this;
    }

    bool operator==(const no_quotes_iterator& i) const {
        return begin == i.begin;
    }

    bool operator!=(const no_quotes_iterator& i) const {
        return begin != i.begin;
    }

    auto operator*() const {
        return pair(*begin, idx);
    }

    string_view::iterator  begin, end;
    string_view::size_type idx = 0;
};

class no_quotes_view {
public:
    no_quotes_view(const string_view& str): _begin{str.begin(), str.end()}, _end{str.end(), str.end()} {}

    [[nodiscard]] auto begin() const {
        return _begin;
    }
    [[nodiscard]] auto end() const {
        return _end;
    }

private:
    no_quotes_iterator _begin, _end;
};

inline string_view remove_single_line_comment(string_view line) {
    for (auto [c, idx] : no_quotes_view(line)) {
        if (line.substr(idx).starts_with("//") || c == ';')
            return line.substr(0, idx);
    }
    return line;
}

template <typename IterT, typename... Ts>
inline void skip_before_one_of(IterT& i, IterT end, Ts... symbols) {
    for (; i != end && ((symbols != *i) && ...); ++i);
}

template <typename IterT, typename... Ts>
inline void skip_all(IterT& i, IterT end, Ts... symbols) {
    for (; i != end && ((symbols == *i) || ...); ++i);
}

template <typename T>
inline static string_view::size_type strsize_cast(T val) {
    return static_cast<string_view::size_type>(val);
}

class cfg_global_state {
    SINGLETON_IMPL(cfg_global_state);

private:
    cfg_global_state()  = default;
    ~cfg_global_state() = default;

    string _cfg_entry_name = "fs.cfg";
    mutable std::mutex _mtx;

    mutable std::mutex _path_mtx;
    optional<string> _cached_default_cfg_path;

    mpmc_hash_map<string, string> _cached_values;

public:
    optional<string> cached_value(const string& section, const string& key) const {
        optional<string> result;
        _cached_values.find_fn(section + "]" + key, [&](const string& val) { result = val; });
        return result;
    }

    optional<string> cached_value(const string& key) const {
        return cached_value("__global", key);
    }

    void set_cached_value(const string& section, const string& key, const string& value) {
        _cached_values.insert_or_assign(section + "]" + key, value);
    }

    void set_cached_value(const string& key, const string& value) {
        set_cached_value("__global", key, value);
    }

    optional<string> cached_default_cfg_path() const {
        auto lock = std::lock_guard{_path_mtx};
        auto res = _cached_default_cfg_path;

        return res;
    }

    std::string cfg_entry_name() const {
        auto lock = std::lock_guard{_mtx};
        std::string res = _cfg_entry_name;

        return res;
    }

    void cfg_entry_name(const std::string& name) {
        _cached_values.clear();

        auto lock2 = std::lock_guard{_path_mtx};
        _cached_default_cfg_path = nullopt;

        auto lock = std::lock_guard{_mtx};
        _cfg_entry_name = name;
    }

    void cached_default_cfg_path(const std::string& name) {
        auto lock2 = std::lock_guard{_path_mtx};
        _cached_default_cfg_path = name;
    }
};

inline cfg_global_state& CFG_STATE() {
    return cfg_global_state::instance();
}

inline string DEFAULT_CFG_PATH() {
    if (auto path = CFG_STATE().cached_default_cfg_path())
        return move(*path);

    auto cfg_entry_name = CFG_STATE().cfg_entry_name();
    auto path = std::filesystem::path(platform_dependent::get_exe_dir());

    auto search_entry = [cfg_entry_name](const std::filesystem::path& path) -> optional<std::filesystem::path> {
        for (auto& p : std::filesystem::directory_iterator(path)) {
            if (p.is_regular_file()) {
                auto filename = p.path().filename().string();
                if (filename == cfg_entry_name)
                    return p;
            }
        }
        return nullopt;
    };

    while (!path.empty()) {
        auto result = search_entry(path);
        if (result) {
            CFG_STATE().cached_default_cfg_path(*result);
            return *result;
        }
        path = path.parent_path();
    }

    throw config_exception("Can't find entry config fs.cfg");
}

inline string DEFAULT_CFG_DIR() {
    auto path = DEFAULT_CFG_PATH();
    path.resize(path.size() - CFG_STATE().cfg_entry_name().size());
    return path;
}

inline string cfg_make_relative(const string& path) {
    auto dir = DEFAULT_CFG_DIR();
    if (path.starts_with(dir))
        return path.substr(dir.size());
    else
        return path;
}

template <typename T>
inline T remove_brackets_if_exists(const T& str, pair<char, char> symbols = {'{', '}'}) {
    if (str.size() > 1 && str.front() == symbols.first && str.back() == symbols.second)
        return str.substr(1, str.size() - 2) / remove_trailing_whitespaces();
    else
        return str;
}

inline vector<string_view> config_unpack(string_view value) {
    auto unbracketed = remove_brackets_if_exists(value);
    {
        int level = 0;
        for (auto c : unbracketed) {
            if (c == '{')
                ++level;
            else if (c == '}')
                --level;

            if (level < 0) {
                unbracketed = value;
                break;
            }
        }
    }

    vector<string_view>    result;
    size_t                 bracket_level = 0;
    string_view::size_type last_start    = 0;

    for (auto [c, idx] : no_quotes_view(unbracketed)) {
        if (c == '{') {
            ++bracket_level;
        }
        else if (c == '}') {
            if (bracket_level == 0)
                throw std::runtime_error("Extra bracket");
            --bracket_level;
        }
        else if (bracket_level == 0 && c == ',') {
            result.emplace_back(unbracketed.substr(last_start, idx - last_start) / remove_trailing_whitespaces());
            last_start = idx + 1;
        }
    }

    if (bracket_level != 0)
        throw std::runtime_error("Missing closing bracket");

    if (last_start != unbracketed.size())
        result.emplace_back(unbracketed.substr(last_start, unbracketed.size() - last_start) /
                            remove_trailing_whitespaces());

    if (result.size() < 1 && (value.front() != '{' || value.back() != '}'))
        throw std::runtime_error("Nothing to unpack!");

    return result;
}

struct cast_helper {
    [[nodiscard]]
    inline string try_interpolation(string_view str) const {
        return f ? f(str) : string(str);
    }
    function<string(string_view)> f;
};

template <typename T> requires requires(T v) { string(v); }
string config_cast(string_view str, const cast_helper& helper = {}) {
    return remove_brackets_if_exists(
            remove_brackets_if_exists(
                helper.try_interpolation(str), {'"', '"'}), {'\'', '\''});
}

template <typename T> requires std::is_same_v<T, bool>
T config_cast(string_view str, const cast_helper& helper = {}) {
    auto value = helper.try_interpolation(str);

    if (value == "true" || value == "on")
        return true;
    else if (value == "false" || value == "off")
        return false;
    else
        throw config_exception("Invalid boolean value '" + value + '\'');
}

template <typename T> requires(Number<T> && !std::is_same_v<T, bool>)
T config_cast(string_view str, const cast_helper& helper = {}) {
    return helper.try_interpolation(str) / to_number<T>();
}

template <typename T>
concept ConfigHasEmplaceBack = requires(T v) {
    typename T::value_type;
    v.emplace_back(std::declval<typename T::value_type>());
};

template <typename T>
concept ConfigHasEmplaceOnly = (!ConfigHasEmplaceBack<T>)&&requires(T v) {
    typename T::value_type;
    v.emplace(std::declval<typename T::value_type>());
};

template <typename T> requires ConfigHasEmplaceBack<T> || ConfigHasEmplaceOnly<T>
T config_cast(string_view str, const cast_helper& helper = {});

template <typename T> requires requires { typename tuple_size<T>::type; }
T config_cast(string_view str, const cast_helper& helper = {});

template <typename T, size_t... idxs>
T config_cast_tuple_like_impl(const vector<string_view>& unpacked,
                              const cast_helper&         helper,
                              std::index_sequence<idxs...>&&) {
    return T{config_cast<tuple_element_t<idxs, T>>(unpacked[idxs], helper)...};
}

template <typename T> requires requires { typename tuple_size<T>::type; }
T config_cast(string_view str, const cast_helper& helper) {
    auto unpacked = config_unpack(str);

    if (tuple_size_v<T> != unpacked.size())
        throw config_exception(format("Invalid tuple size. Requested: {} Has: {}", tuple_size_v<T>, unpacked.size()));

    return config_cast_tuple_like_impl<T>(unpacked, helper, std::make_index_sequence<tuple_size_v<T>>());
}

template <typename T> requires ConfigHasEmplaceBack<T> || ConfigHasEmplaceOnly<T>
T config_cast(string_view str, const cast_helper& helper) {
    using value_type = typename T::value_type;
    auto unpacked    = config_unpack(str);

    T result;

    for (auto& unpacked_str : unpacked) {
        if constexpr (ConfigHasEmplaceBack<T>)
            result.emplace_back(config_cast<value_type>(unpacked_str, helper));
        else if constexpr (ConfigHasEmplaceOnly<T>)
            result.emplace(config_cast<value_type>(unpacked_str, helper));
    }

    return result;
}

/**
 * @brief Represents a value from config key
 */
class config_value {
public:
    config_value() = default;

    template <Printable T>
    config_value(T&& val) {
        _value = format("{}", std::forward<T>(val));
    }

    void print(std::ostream& os) const {
        os << _value;
    }

    /**
     * @brief Cast the config value to T type
     *
     * @tparam T - the new type
     * @param helper - optional callback, thats do some needed operations with raw value before cast will be performed
     *
     * @throw config_exception if cast is invalid
     *
     * @return the value with T type
     */
    template <typename T>
    [[nodiscard]]
    auto cast(const cast_helper& helper = {}) const {
        return config_cast<T>(_value, helper);
    }

    /**
     * @brief Returns raw string value
     *
     * @return the string with raw value
     */
    [[nodiscard]]
    const string& str() const {
        return _value;
    }

    /**
     * @brief Returns raw string value
     *
     * @return the string with raw value
     */
    void str(string_view value) {
        _value = string(value);
    }

    /**
     * @brief Check if the value inherited from any parent section
     *
     * @return true if inherited, false otherwise
     */
    [[nodiscard]]
    bool is_inherited() const {
        return _is_inherited;
    }

protected:
    friend class config_manager;
    friend class config_section;

    auto& str() {
        return _value;
    }

    void make_inherited() {
        _is_inherited = true;
    }

private:
    string _value;
    bool   _is_inherited = false;
};


/**
 * @brief Read path from '#entry' key in config file
 *
 * It useful, when you want to read a section with 'config_section::direct_read()' function
 *
 * @param name - the entry name
 * @param file_path - the path to config file
 *
 * @return the string with the path
 */
inline string cfg_reentry(string_view name, const string& file_path = DEFAULT_CFG_PATH()) {
    auto path = path_eval(file_path);
    auto file = read_file_unwrap(path);

    for (auto& line : file / split_view({'\n', '\r'})) {
        if (!line.empty() && line.front() == '#') {
            auto splits = (line.substr(1) / remove_trailing_whitespaces()) / split_view({'\t', ' '});

            if (splits.size() > 2 && splits[0] == "entry" && splits[1] == name)
                return path_eval(
                    path / "../" /
                    remove_brackets_if_exists(remove_brackets_if_exists(splits[2], {'"', '"'}), {'\'', '\''}));
        }
    }

    PeRelAbortF("Can't find entry '{}' in file '{}'", name, path);

    return "";
}

/**
 * @brief Represents a config section
 */
class config_section {
public:
    /**
     * @brief Reads a specific section
     *
     * This function read only one section from config file.
     * It useful when you want to read a specific section without config_manager's overhead.
     * This way has some limitations:
     * - All '#include' will be skipped
     * - Section inheritance is not supported: std::terminate will be called if section has inheritance
     * - Value interpolation is not supported: all strings with interpolation will remain unevaluated
     *
     * @param name - section name
     * @param entry_path - path to config file
     *
     * @return read section
     */
    static config_section direct_read(string_view name = "__global", const string& entry_path = DEFAULT_CFG_PATH()) {
        auto result = config_section(name);
        auto path   = path_eval(entry_path);

        struct file_node {
            file_node(string_view data): lines(data / split({'\n', '\r'})) {}
            vector<string> lines;
            size_t         i = 0;
        };

        auto file_stack = vector<file_node>(1, file_node(read_file_unwrap(path)));
        bool is_global  = name == "__global";
        bool on_section = is_global ? true : false;

        auto current_value_pos = result._map.end();
        auto section_start     = '[' + result._name + ']';

        while (!file_stack.empty()) {
            for (; file_stack.back().i != file_stack.back().lines.size(); ++file_stack.back().i) {
                auto& line = file_stack.back().lines[file_stack.back().i];

                line = remove_single_line_comment(line) / remove_trailing_whitespaces();

                if (line.empty())
                    continue;
                if (on_section && line.front() == '[')
                    return result;
                else if (!on_section && line.starts_with(section_start)) {
                    PeRelRequireF((line.substr(section_start.size()) / remove_trailing_whitespaces()).empty(),
                             "Unrecognized symbols after section {} definition. "
                             "Note: direct reading doesn't support section inheritance",
                             section_start);
                    on_section = true;
                }
                else if (line.front() == '#') {
                    line = line.substr(1) / remove_trailing_whitespaces();
                    if (line.starts_with("include") && !is_global) {
                        line          = line.substr(sizeof("include") - 1) / remove_trailing_whitespaces();
                        auto new_path = path_eval(
                            path / ".." /
                            remove_brackets_if_exists(remove_brackets_if_exists(line, {'"', '"'}), {'\'', '\''}));

                        file_stack.back().i++; // Skip current line after incuded file processing
                        file_stack.emplace_back(read_file_unwrap(new_path));
                        file_stack.back().i--; // Prevent skipping first line of included file
                    }
                }
                else if (on_section) {
                    auto splits = line / split_view('=');
                    for (auto& s : splits) s = s / remove_trailing_whitespaces();

                    if (splits.size() == 1 && current_value_pos != result._map.end()) {
                        current_value_pos->second.str() += splits.front();
                    }
                    else if (splits.size() == 2) {
                        current_value_pos =
                            result._map.insert_or_assign(string(splits[0]), config_value(string(splits[1]))).first;
                    }
                    else if (splits.size() > 2) {
                        LOG_WARNING("config_manager: {}: skipping invalid line '{}'", path, line);
                    }
                }
            }

            file_stack.pop_back();
        }

        PeRelRequireF(on_section, "Can't find section {}", section_start);

        return result;
    }

    config_section(string_view name): _name(name) {}

    /**
     * @brief Returns a reference to the map with config keys and values
     *
     * @return a reference to map with config keys and values
     */
    [[nodiscard]]
    auto values() const -> const hash_map<string, config_value>& {
        return _map;
    }

    /**
     * @brief Returns a reference to the map with config keys and values
     *
     * @return a reference to map with config keys and values
     */
    [[nodiscard]]
        auto parents() const -> const vector<string> {
        return _parents;
    }

    /**
     * @brief Gets the name of the section
     *
     * @return the string with the name of the section
     */
    [[nodiscard]]
    const string& name() const {
        return _name;
    }

    /**
     * @brief Gets the raw string value by the key
     *
     * @param key - the key of value
     *
     * @return the try_opt with the value or with the config_exception, if value was not found
     */
    [[nodiscard]]
    try_opt<config_value> raw_value(string_view key) const {
        auto found = _map.find(string(key));
        return found != _map.end() ? try_opt(found->second)
                                   : try_opt<config_value>(
                                         config_exception(format("Can't find key '{}' in section [{}]", key, _name)));
    }

    /**
     * @brief Gets the value by the key
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     *
     * @return the try_opt with the value or with the config_exception, if the value was not found
     */
    template <typename T>
    [[nodiscard]]
    try_opt<T> read(const string& key) const {
        auto found = _map.find(key);
        if (found != _map.end()) {
            try {
                return found->second.cast<T>();
            }
            catch (const std::exception& e) {
                return config_exception(e.what());
            }
        }
        else {
            return config_exception(format("Can't find key '{}' in section [{}]", key, _name));
        }
    }

    /**
     * @brief Gets the value by the key
     *
     * @throw config_exception if the value was not found
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     *
     * @return the read value
     */
    template <typename T>
    [[nodiscard]]
    T read_unwrap(const string& key) const {
        return read<T>(key).value();
    }

    /**
     * @brief Gets the value by the key
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     * @param default_value - will be returned if the value would not found by the key
     *
     * @return the value if it exists, default_value otherwise
     */
    template <typename T>
    T read_default(const string& key, T&& default_value) const {
        if (auto v = read<T>(key))
            return *v;
        else
            return forward<T>(default_value);
    }

    /**
     * @brief write the section to output stream
     *
     * @param os - output stream
     */
    void print(std::ostream& os) const {
        os << '[' << "_name" << ']';

        if (!_parents.empty()) {
            os << ": ";
            auto parents = format("{}", _parents);
            os << remove_brackets_if_exists(parents).substr(0, parents.size() - 1);
        }
        os << '\n';

        auto max_key_length = std::max_element(_map.begin(), _map.end(), [](auto& a, auto& b) {
                                  return a.first.size() < b.first.size();
                              })->first.size();

        auto saved_flags = os.flags();
        os << std::left;

        for (auto& [k, v] : _map)
            if (!v.is_inherited())
                os << std::setw(static_cast<int>(max_key_length)) << k << " = " << v.str() << '\n';

        os.flags(saved_flags);
    }

protected:
    friend class config_manager;

    void setup_parents(vector<string> parents) {
        _parents = move(parents);
    }

    auto& values() {
        return _map;
    }

private:
    vector<string>                 _parents;
    hash_map<string, config_value> _map;
    string                         _name;
};


/**
 * @brief a manager that holds config sections and performs access to them
 */
class config_manager {
public:
    /**
     * @brief Constructs manager from the entry file
     *
     * @param entry_path - the path to the entry file
     */
    explicit config_manager(const string& entry_path = DEFAULT_CFG_PATH()) {
        auto path  = path_eval(entry_path);
        _entry_dir = path_eval(path / "../");

        do_file(path);
    }

    /**
     * @brief Gets a reference to the map with all sections
     *
     * @return a reference to the map with all sections
     */
    [[nodiscard]]
    auto sections() const -> const hash_map<string, config_section>& {
        return _sections;
    }

    /**
     * @brief Gets a reference to the map with all sections
     *
     * @return a reference to the map with all sections
     */
    [[nodiscard]]
    auto sections() -> hash_map<string, config_section>& {
        return _sections;
    }

    /**
     * @brief Gets the value from the global section by the key
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     *
     * @return the try_opt with the value or with the config_exception, if the value was not found
     */
    template <typename T>
    try_opt<T> read(string_view key) const {
        return read<T>("__global", key);
    }

    /**
     * @brief Gets the value from the section by the key
     *
     * @tparam T - the type of the value
     * @param section - the section
     * @param key - the key of the value
     *
     * @return the try_opt with the value or with the config_exception, if the value or the section was not found
     */
    template <typename T>
    try_opt<T> read(string_view section, string_view key) const {
        auto found = _sections.find(string(section));
        if (found != _sections.end()) {
            try {
                auto interpolation_func = [this](string_view value) -> string {
                    constexpr size_t min_inter_len = 6; // minimal string $(::a)

                    bool can_be_interpolated = value.size() > min_inter_len &&
                                               value.front() != '\''        &&
                                               value.back()  != '\'';

                    if (!can_be_interpolated)
                        return string(value);

                    auto result = string(value);

                    for (string::size_type i = 0; i < result.size(); ++i) {
                        bool interpolation_start =
                            result[i] == '$' && i + 1 < result.size() && result[i + 1] == '(';

                        if (!interpolation_start)
                            continue;

                        string::size_type j = i + 2;
                        for (; j < result.size() && result[j] != ')'; ++j);

                        if (j < result.size() && result[j] == ')') {
                            auto name  = string_view(result).substr(i + 2, j - i - 2);
                            auto delim = name.find("::");

                            auto value = delim != string_view::npos
                                             ? raw_str(name.substr(0, delim), name.substr(delim + 2))
                                             : raw_str("__global", name);

                            if (value) {
                                string::size_type skip_expanded = 0;

                                if (value->size() > 2 && ((value->front() == '\'' && value->back() == '\'') ||
                                                      (value->front() == '"' && value->back() == '"'))) {
                                    if (value->front() == '\'')
                                        skip_expanded = value->size() - 2;

                                    value = value->substr(1, value->size() - 2);
                                }

                                result.replace(i, j - i + 1, *value);

                                --i;
                                i += skip_expanded;
                            }
                            else {
                                LOG_WARNING("Can't interpolate value \'{}\'", name);
                            }
                        }
                    }
                    return result;
                };

                return found->second.raw_value(key).value().cast<T>({interpolation_func});
            }
            catch (const std::exception& e) {
                return config_exception(e.what());
            }
        }
        else {
            return config_exception(format("Can't find section [{}]", section));
        }
    }

    /**
     * @brief Gets the value from the global section by the key
     *
     * @throw config_exception if the value was not found
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     *
     * @return the read value
     */
    template <typename T>
    T read_unwrap(string_view key) const {
        return read<T>(key).value();
    }

    /**
     * @brief Gets the value from the section by the key
     *
     * @throw config_exception if the value or the section was not found
     *
     * @tparam T - the type of the value
     * @param section - the section
     * @param key - the key of the value
     *
     * @return the read value
     */
    template <typename T>
    T read_unwrap(string_view section, string_view key) const {
        return read<T>(section, key).value();
    }

    /**
     * @brief Gets the value from the global section by the key
     *
     * @tparam T - the type of the value
     * @param key - the key of the value
     * @param default_value - will be returned if the value would not found by the key
     *
     * @return the value if it exists, default_value otherwise
     */
    template <typename T>
    T read_default(string_view key, T&& default_value) const {
        if (auto v = read<T>(key))
            return *v;
        else
            return forward<T>(default_value);
    }

    /**
     * @brief Gets the value from the section by the key
     *
     * @tparam T - the type of the value
     * @param section - the section
     * @param key - the key of the value
     * @param default_value - will be returned if the value or the section would not found by the key
     *
     * @return the value if it exists, default_value otherwise
     */
    template <typename T>
    T read_default(string_view section, string_view key, const T& default_value) const {
        if (auto v = read<T>(section, key))
            return *v;
        else
            return default_value;
    }


    /**
     * @brief Init config values
     *
     * @tparam ArgsT - type of config values
     * @param args - config values
     */
    template <typename... ArgsT>
    void read_cfg_values(ArgsT&... args) const;

    /**
     * @brief Gets entry dir of the config_manager
     *
     * @return string with the entry dir
     */
    [[nodiscard]]
    const string& entry_dir() const {
        return _entry_dir;
    }

private:
    [[nodiscard]]
    try_opt<string> raw_str(string_view section, string_view key) const {
        auto sect = _sections.find(string(section));
        if (sect != _sections.end()) {
            if (auto value = sect->second.raw_value(key))
                return value->str();
            else
                return config_exception(format("Can't find value at key '{}' in section [{}]", key, section));
        }

        return config_exception(format("Can't find section [{}]", section));
    }

    void load_parents_for(config_section& section) {
        for (auto& parent_section_name : section.parents()) {
            auto parent_section = _sections.find(parent_section_name);
            PeRelRequireF(parent_section != _sections.end(),
                     "Can't find parent section '{}' in section '{}'",
                     parent_section_name,
                     section.name());

            for (auto& [key, value] : parent_section->second.values()) {
                auto [position, is_inserted] = section.values().emplace(key, value);
                if (is_inserted)
                    position->second.make_inherited();
            }
        }
    }

    void do_file(const string& path) {
        auto file = read_file(path);

        PeRelRequireF(file.has_value(), "Can't open file '{}'", path);

        auto lines = file.value() / split_view({'\n'}, true);

        auto current_section = &(_sections.emplace("__global", "__global").first->second);
        auto current_value   = &(current_section->values()["dummy_global"]);

        for (auto& line : lines) {
            line = remove_single_line_comment(line) / remove_trailing_whitespaces();

            if (line.empty())
                continue;

            if (line.front() == '[') {
                auto pos = line.begin() + 1;

                skip_before_one_of(pos, line.end(), ']');

                PeRelRequireF(pos != line.end(), "{}", "Missing closing ']'");

                auto section_name            = string(line.substr(1, strsize_cast(pos - line.begin()) - 1));
                auto [position, is_inserted] = _sections.emplace(section_name, section_name);

                if (section_name != "__global") {
                    PeRelRequireF(is_inserted, "Dupplicate section '{}'", section_name);
                    _file_paths.emplace(section_name, path);
                }

                current_section = &(position->second);

                ++pos; // skip ']'

                skip_all(pos, line.end(), ' ', '\t');

                if (*pos == ':') {
                    ++pos;
                    auto parents = line.substr(strsize_cast(pos - line.begin())) / split({','});

                    for (auto& p : parents) p = p / remove_trailing_whitespaces();

                    current_section->setup_parents(parents);

                    load_parents_for(*current_section);
                }
            }
            else if (line.front() == '#') {
                auto pos = line.begin() + 1;

                skip_all(pos, line.end(), ' ', '\t');
                auto start = pos;

                skip_before_one_of(pos, line.end(), ' ', '\t');

                auto preprocessor_directive =
                    line.substr(strsize_cast(start - line.begin()), strsize_cast(pos - start));
                auto command = line.substr(strsize_cast(pos - line.begin())) / remove_trailing_whitespaces();

                if (preprocessor_directive == "include")
                    do_file(path_eval(
                        path / ".." /
                        remove_brackets_if_exists(remove_brackets_if_exists(command, {'"', '"'}), {'\'', '\''})));
                else if (preprocessor_directive != "entry")
                    LOG_WARNING(
                        "config_manager: {}: unknown preprocessor_directive '{}'", path, preprocessor_directive);
            }
            else {
                auto splits = line / split_view(_value_delimiter);
                for (auto& s : splits) s = s / remove_trailing_whitespaces();

                if (splits.size() == 1) {
                    current_value->str() += splits.front();
                }
                else if (splits.size() == 2) {
                    auto rc =
                        current_section->values().insert_or_assign(string(splits[0]), config_value(string(splits[1])));
                    current_value = &(rc.first->second);
                }
                else if (splits.size() > 2) {
                    LOG_WARNING("config_manager: {}: skipping invalid line '{}'", path, line);
                }
            }
        }
    }

private:
    hash_map<string, string>         _file_paths;
    hash_map<string, config_section> _sections;

    string _entry_dir;
    char   _value_delimiter = '=';
};


/**
 * @brief Represents a value that can be read from the config
 *
 * @tparam T - type of config value
 */
template <typename T>
class cfg_val {
public:
    /**
     * @brief Constructs with specific key and __global section
     *
     * @param - the key
     */
    cfg_val(string_view key): _key(key) {}

    /**
     * @brief Constructs with specific key and section
     *
     * @param key - the key
     * @param section - the section
     */
    cfg_val(string_view section, string_view key): _section(section), _key(key) {}

    /**
     * @brief Reads *this value from config_manager
     *
     * @param m - config manager for reading value
     */
    void read(const config_manager& m) {
        _value = m.read_default<T>(_section, _key, _value);
    }

    /**
     * @brief Gets a reference to the stored value
     *
     * @return the stored value
     */
    const T& value() const {
        return _value;
    }

    /**
     * @brief Gets a reference to the stored value
     *
     * @return the stored value
     */
    T& value() {
        return _value;
    }

private:
    string _section = "__global";
    string _key;
    T      _value;
};

template <typename... ArgsT>
void config_manager::read_cfg_values(ArgsT&... args) const {
    ((args.read(*this), ...));
}

inline string cfg_read_path(string_view section, const string& key) {
    if (auto cached = CFG_STATE().cached_value(string(section), key))
        return *cached;
    else {
        auto value =
            DEFAULT_CFG_DIR() / config_section::direct_read(section).read_unwrap<string>(key);
        CFG_STATE().set_cached_value(string(section), key, value);
        return value;
    }
}

inline string cfg_read_path(const string& key) {
    if (auto cached = CFG_STATE().cached_value(key))
        return *cached;
    else {
        auto value = DEFAULT_CFG_DIR() / config_section::direct_read().read_unwrap<string>(key);
        CFG_STATE().set_cached_value(key, value);
        return value;
    }
}


/**
 * @brief Stores a path with prefix directory key
 */
class cfg_path {
public:
    PE_SERIALIZE(dir_key, path)

    cfg_path() = default;

    /**
     * @brief Constructs a path with the default directory prefix (location of fs.cfg)
     *
     * @param ipath - the relative path after default directory prefix
     */
    cfg_path(string_view ipath): path(ipath) {}

    /**
     * @brief Constructs a path with specified directory prefix key
     *
     * @param idir_key - the directory prefix key that will be reads from fs.cfg
     * @param ipath - the relative path after directory prefix
     */
    cfg_path(string_view idir_key, string_view ipath): dir_key(idir_key), path(ipath) {}

    /**
     * @brief Constructs the absolute path
     *
     * @return the absolute path
     */
    [[nodiscard]]
    string absolute() const {
        if (dir_key.empty())
            return DEFAULT_CFG_DIR() / path;
        else
            return cfg_read_path(dir_key) / path;
    }

    string dir_key;
    string path;
};
} // namespace core

/**
 *
 * @brief Declare cfg_val variable with specified type, section and key
 *
 * @param TYPE - type of the value stored in cfg_val
 * @param SECTION - the section name
 * @param name - the name for variable declaration and the key name
 */
#define CFG_VAL(TYPE, SECTION, NAME) core::cfg_val<TYPE> NAME = core::cfg_val<TYPE>(SECTION, #NAME)

