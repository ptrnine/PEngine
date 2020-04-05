#pragma once

#include "core/log.hpp"
#include "types.hpp"
#include "container_extensions.hpp"
#include "assert.hpp"
#include "ston.hpp"
#include "platform_dependent.hpp"

namespace core
{
    namespace {
        struct no_quotes_iterator {
            no_quotes_iterator(string_view::iterator beg, string_view::iterator en): begin(beg), end(en) {
                if (!skip('"'))
                    skip('\'');
            }

            bool skip(char quote) {
                if (begin != end && *begin == quote) {
                    ++begin;
                    ++idx;
                    for (; begin != end && *begin != quote; ++begin, ++idx);

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

            string_view::iterator begin, end;
            string_view::size_type idx = 0;
        };

        class no_quotes_view {
        public:
            no_quotes_view(const string_view& str): _begin{str.begin(), str.end()}, _end{str.end(), str.end()} {}

            auto begin() const { return _begin; }
            auto end()   const { return _end; }

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
    } // anonymous namespace


    template <typename T>
    inline T remove_brackets_if_exists(const T& str, pair<char, char> symbols = { '{', '}'}) {
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

        vector<string_view> result;
        size_t bracket_level = 0;
        string_view::size_type last_start = 0;

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
            result.emplace_back(
                    unbracketed.substr(last_start, unbracketed.size() - last_start) / remove_trailing_whitespaces());

        if (result.size() < 2 && (value.front() != '{' || value.back() != '}'))
            throw std::runtime_error("Nothing to unpack!");

        return result;
    }

    struct cast_helper {
        inline string try_interpolation(string_view str) const {
            return f ? f(str) : string(str);
        }
        function<string(string_view)> f;
    };

    template <typename T> requires requires (T v) { string(v); }
    string config_cast(string_view str, const cast_helper& helper = {}) {
        return remove_brackets_if_exists(
                remove_brackets_if_exists(
                        helper.try_interpolation(str), {'"', '"'}),
                {'\'', '\''});
    }

    template <typename T> requires std::is_same_v<T, bool>
    T config_cast(string_view str, const cast_helper& helper = {}) {
        auto value = helper.try_interpolation(str);

        if (value == "true" || value == "on" )
            return true;
        else if (value == "false" || value == "off")
            return false;
        else {
            RABORTF("Invalid boolean value '{}'", value);
            return false;
        }
    }

    template <typename T> requires (Number<T> && !std::is_same_v<T, bool>)
    T config_cast(string_view str, const cast_helper& helper = {}) {
        return helper.try_interpolation(str) / to_number<T>();
    }


    template <typename T>
    concept ConfigHasEmplaceBack = requires (T v) {
        typename T::value_type;
        v.emplace_back(std::declval<typename T::value_type>());
    };

    template <typename T>
    concept ConfigHasEmplaceOnly =  (!ConfigHasEmplaceBack<T>) && requires (T v) {
        typename T::value_type;
        v.emplace(std::declval<typename T::value_type>());
    };

    template <typename T> requires ConfigHasEmplaceBack<T> || ConfigHasEmplaceOnly<T>
    T config_cast(string_view str, const cast_helper& helper = {});

    template <typename T> requires requires { typename std::tuple_size<T>::type; }
    T config_cast(string_view str, const cast_helper& helper = {});

    template <typename T, size_t... idxs>
    T config_cast_static_cortage_impl(const vector<string_view>& unpacked, const cast_helper& helper, std::index_sequence<idxs...>&&) {
        return T{config_cast<std::tuple_element_t<idxs, T>>(unpacked[idxs], helper)...};
    }

    template <typename T> requires requires { typename std::tuple_size<T>::type; }
    T config_cast(string_view str, const cast_helper& helper) {
        auto unpacked = config_unpack(str);

        RASSERTF(std::tuple_size_v<T> == unpacked.size(),
                "Invalid cortege size. Requested: {} Has: {}", std::tuple_size_v<T>, unpacked.size());


        return config_cast_static_cortage_impl<T>(unpacked, helper, std::make_index_sequence<std::tuple_size_v<T>>());
    }

    template <typename T> requires ConfigHasEmplaceBack<T> || ConfigHasEmplaceOnly<T>
    T config_cast(string_view str, const cast_helper& helper) {
        using value_type = typename T::value_type;
        auto unpacked = config_unpack(str);

        T result;

        for (auto& unpacked_str : unpacked) {
            if constexpr (ConfigHasEmplaceBack<T>)
                result.emplace_back(config_cast<value_type>(unpacked_str, helper));
            else if constexpr (ConfigHasEmplaceOnly<T>)
                result.emplace(config_cast<value_type>(unpacked_str, helper));

        }

        return result;
    }

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

        template <typename T>
        auto cast(const cast_helper& helper = {}) const {
            return config_cast<T>(_value, helper);
        }

        auto& str() const {
            return _value;
        }

        void str(string_view value) {
            _value = string(value);
        }

        bool is_inherited() const {
            return _is_inherited;
        }

    protected:
        friend class config_manager;

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


    class config_section {
    public:
        config_section(string_view name): _name(name) {}

        auto& values() const {
            return _map;
        }

        const auto& parents() const {
            return _parents;
        }

        const string& name() const {
            return _name;
        }

        optional<config_value> raw_value(string_view key) const {
            auto found = _map.find(string(key));
            return found != _map.end() ? optional(found->second) : nullopt;
        }

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


    class config_manager {
    public:
        explicit config_manager(const string& entry_path = ::platform_dependent::get_exe_dir() / "../fs.cfg") {
            auto path  = path_eval(entry_path);
            _entry_dir = path_eval(path / "../");

            do_file(path);
        }

        auto& sections() const {
            return _sections;
        }

        auto& sections() {
            return _sections;
        }

        template <typename T>
        optional<T> read(string_view key) const {
            return read<T>("__global", key);
        }

        template <typename T>
        optional<T> read(string_view section, string_view key) const {
            auto found = _sections.find(string(section));
            if (found != _sections.end())
                if (auto value = found->second.raw_value(key))
                    return value->cast<T>({[this](string_view value) -> string {
                        if (value.size() > 6 && value.front() != '\'' && value.back() != '\'') {
                            auto result = string(value);
                            
                            for (string::size_type i = 0; i < result.size(); ++i) {
                                if (result[i] == '$' && i + 1 < result.size() && result[i + 1] == '(') {
                                    string::size_type j = i + 2;
                                    for (; j < result.size() && result[j] != ')'; ++j);

                                    if (j < result.size() && result[j] == ')') {
                                        auto name = string_view(result).substr(i + 2, j - i - 2);
                                        auto delim = name.find("::");

                                        auto value = delim != string_view::npos ?
                                                raw_str(name.substr(0, delim), name.substr(delim + 2)) :
                                                raw_str("__global", name);

                                        if (value) {
                                        string::size_type skip_expanded = 0;

                                            if (value->size() > 2 && ((value->front() == '\'' && value->back() == '\'') || 
                                                                      (value->front() == '"'  && value->back() == '"')))
                                            {
                                                if (value->front() == '\'')
                                                    skip_expanded = value->size() - 2;

                                                value = value->substr(1, value->size() - 2);
                                            }

                                            result.replace(i, j - i + 1, *value);

                                            --i;
                                            i += skip_expanded;
                                        } else {
                                            LOG_WARNING("Can't interpolate value \'{}\'", name);
                                        }
                                    }
                                }
                            }
                            return result;
                        }
                        return string(value);
                    }});
            
            return nullopt;
        }

        template <typename T>
        T read_unwrap(string_view key) const {
            return *read<T>(key);
        }

        template <typename T>
        T read_unwrap(string_view section, string_view key) const {
            return *read<T>(section, key);
        }

        template <typename T>
        T read_default(string_view key, T&& default_value) const {
            if (auto v = read<T>(key))
                return *v;
            else
                return forward<T>(default_value);
        }

        template <typename T>
        T read_default(string_view section, string_view key, const T& default_value) const {
            if (auto v = read<T>(section, key))
                return *v;
            else
                return default_value;
        }

        template <typename... ArgsT>
        void init_cfg_values(ArgsT&... args) const;

        const string& entry_dir() const {
            return _entry_dir;
        }

    private:
        optional<string> raw_str(string_view section, string_view key) const {
            auto sect = _sections.find(string(section));
            if (sect != _sections.end())
                if (auto value = sect->second.raw_value(key))
                    return value->str();

            return nullopt;
        }

        void load_parents_for(config_section& section) {
            for (auto& parent_section_name : section.parents()) {
                auto parent_section = _sections.find(parent_section_name);
                RASSERTF(parent_section != _sections.end(), "Can't find parent section '{}' in section '{}'",
                         parent_section_name, section.name());

                for (auto& [key, value] : parent_section->second.values()) {
                    auto [position, is_inserted] = section.values().emplace(key, value);
                    if (is_inserted)
                        position->second.make_inherited();
                }
            }
        }

        void do_file(const string& path) {
            auto file = read_file(path);

            RASSERTF(file.has_value(), "Can't open file '{}'", path);

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

                    RASSERTF(pos != line.end(), "{}", "Missing closing ']'");

                    auto section_name = string(line.substr(1, strsize_cast(pos - line.begin()) - 1));
                    auto [position, is_inserted] = _sections.emplace(section_name, section_name);

                    if (section_name != "__global") {
                        RASSERTF(is_inserted, "Dupplicate section '{}'", section_name);
                        _file_paths.emplace(section_name, path);
                    }

                    current_section = &(position->second);

                    ++pos; // skip ']'

                    skip_all(pos, line.end(), ' ', '\t');

                    if (*pos == ':') {
                        auto parents = line.substr(strsize_cast(pos + 1 - line.begin())) / split({','});
                    
                        for (auto& p : parents)
                            p = p / remove_trailing_whitespaces();

                        current_section->setup_parents(parents);

                        load_parents_for(*current_section);
                    }
                }
                else if (line.front() == '#') {
                    auto pos = line.begin() + 1;

                    skip_all(pos, line.end(), ' ', '\t');
                    auto start = pos;

                    skip_before_one_of(pos, line.end(), ' ', '\t');

                    auto preprocessor_directive = line.substr(strsize_cast(start - line.begin()), strsize_cast(pos - start));
                    auto command = line.substr(strsize_cast(pos - line.begin())) / remove_trailing_whitespaces();

                    if (preprocessor_directive == "include")
                        do_file(path_eval(path / ".." /
                            remove_brackets_if_exists(
                                remove_brackets_if_exists(command, {'"', '"'}), {'\'', '\''})));
                    else
                        LOG_WARNING("config_manager: {}: unknown preprocessor_directive '{}'", path, preprocessor_directive);

                }
                else {
                    auto splits = line / split_view(_value_delimiter);
                    for (auto& s : splits)
                        s = s / remove_trailing_whitespaces();

                    if (splits.size() == 1) {
                        current_value->str() += splits.front();
                    }
                    else if (splits.size() == 2) {
                        auto rc = current_section->values().insert_or_assign(
                                string(splits[0]), config_value(string(splits[1])));
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
        char _value_delimiter = '=';
    };


    template <typename T>
    class cfg_val {
    public:
        cfg_val(string_view key): _key(key) {}
        cfg_val(string_view section, string_view key): _section(section), _key(key) {}

        void read(const config_manager& m) {
            _value = m.read_default<T>(_section, _key, _value);
        }

        const T& value() const { return _value; }
        T&       value()       { return _value; }

    private:
        string _section = "__global";
        string _key;
        T _value;
    };

    template <typename... ArgsT>
    void config_manager::init_cfg_values(ArgsT&... args) const {
        ((args.read(*this), ...));
    }

} // namespace core

#define CFG_VAL(TYPE, SECTION, NAME) core::cfg_val<TYPE> NAME = core::cfg_val<TYPE>(SECTION, #NAME)
