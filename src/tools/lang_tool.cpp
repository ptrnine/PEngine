#include "core/main.cpp"

#include <core/views/split.hpp>
#include <core/views/sub.hpp>
#include <core/acts/trim.hpp>
#include <core/acts/to.hpp>
#include <core/match_syntax.hpp>
#include <nuklear.h>

using namespace core;

PE_DEFAULT_ARGS("--disable-file-logs", "--disable-stdout-logs");

enum class cmode_t {
    remove_comments,
    c_preprocessor,
    cpp_preprocessor,
    nuklear_binding
};

void remove_comments() {
    enum state_t {
        on_code = 0,
        on_quotes,
        on_double_quotes,
        on_raw_literal,
        on_comment,
        on_multiline_comment_start,
        on_multiline_comment
    } state = on_code;
    std::string raw_literal;

    for (std::string sline; std::getline(std::cin, sline);) {
        std::string_view line = sline;
        std::string output_line;

        if (line.empty()) {
            std::cout << std::endl;
            continue;
        }

        for (size_t i = 0; i < line.size(); ++i) {
            switch (state) {
                case on_code:
                    if (line[i] == '\'')
                        state = on_quotes;
                    else if (line[i] == '"')
                        state = on_double_quotes;
                    else if (line.substr(i).starts_with("//"))
                        state = on_comment;
                    else if (line.substr(i).starts_with("/*")) {
                        ++i;
                        state = on_multiline_comment_start;
                    }
                    break;

                case on_quotes:
                    if (line[i] == '\'')
                        state = on_code;
                    else if (line.substr(i).starts_with("\\'")) {
                        output_line += line[i];
                        ++i;
                    }
                    break;

                case on_double_quotes:
                    if (line[i] == '"')
                        state = on_code;
                    else if (line.substr(i).starts_with("\\\"")) {
                        output_line += line[i];
                        ++i;
                    }
                    break;

                default: break;
            }

            if (state == on_multiline_comment_start) {
                state = on_multiline_comment;
                continue;
            }
            else if (state == on_multiline_comment) {
                if (line.substr(i).starts_with("*/")) {
                    ++i;
                    state = on_code;
                }
                continue;
            }
            else if (state == on_comment) {
                state = on_code;
                break;
            }

            output_line += line[i];
        }

        if (static_cast<size_t>(output_line / count_if([](char c) {
                                    return c == ' ' || c == '\t';
                                })) < output_line.size())
            std::cout << output_line << std::endl;
    }
}

struct argument_t {
    template <typename I>
    argument_t(I b, I e) {
        auto arg = string_view(b, e);

        auto splits = arg / views::split(' ', '\t') / acts::to<vector<string_view>>();
        if (splits.front() == "const") {
            is_const = true;
            splits.erase(splits.begin());
        }

        if (splits.front() == "struct" || splits.front() == "enum" || splits.front() == "unsigned") {
            PeRelAssert(splits.size() >= 2);
            type = build_string(splits[0], " "sv, splits[1]);
            splits.erase(splits.begin(), splits.begin() + 2);
        } else {
            type = splits.front();
            splits.erase(splits.begin());

            if (type.find("(*") != string::npos) {
                while (!splits.empty()) {
                    type.push_back(' ');
                    type += splits.front();
                    splits.erase(splits.begin());
                }
                auto start_f_name = type.find("(*");
                auto end_f_name   = type.find(')', start_f_name);
                name = type.substr(start_f_name + 2, end_f_name - start_f_name - 2);
                type.erase(start_f_name, end_f_name - start_f_name + 1);
                type = "nkfunc<" + type + ">";
            }
        }

        while (splits.front() == "*") {
            type.push_back('*');
            splits.erase(splits.begin());
        }

        if (!splits.empty()) {
            PeRelRequire(splits.size() == 1);
            name = splits.front();
        }

        while (!name.empty() && name.front() == '*') {
            type.push_back('*');
            name.erase(name.begin());
        }

        auto starts_with_nk = [](string_view s) {
            return s.starts_with("struct nk_") || s.starts_with("enum nk_") ||s.starts_with("nk_");
        };

        if (name.empty()) {
            name = type / match {
                (_it_ == "char*")              --> "str"sv,
                (_it_ == "char")               --> "sybmol"sv,
                (_it_ == "struct nk_context*") --> "_ctx"sv,
                starts_with_nk                 --> [&]() {
                    auto found = type.find("nk_");
                    auto res = string_view(type).substr(found + 3);
                    return res.substr(0, res.find('*'));
                },
                noopt                          --> "CHANGE_THIS_NAME"sv
            };
        }

        //std::cout << "OK, " << (is_const ? "const " : "") << type << " " << name << std::endl;
    }

    template <typename R>
    argument_t(R rng): argument_t(rng.begin(), rng.end()) {}

    string type;
    string name;
    bool   is_const = false;
};

void nuklear_binding() {
    constexpr auto make_uniq_args = [](list<argument_t>& args) {
        set<string> names;
        for (auto& arg : args) {
            int num = 1;
            auto name_pref = arg.name;
            auto name = arg.name;

            while (names.contains(name))
                name = name_pref + std::to_string(num++);

            names.insert(name);
            arg.name = name;
        }
    };
    constexpr auto print_member_f = [](string_view             return_type,
                                       string_view             name,
                                       const list<argument_t>& args) {
        constexpr auto arg_cast = [](const argument_t& arg) {
            return arg.type / match {
                "nk_bool" <-
                    [&] { return tuple("bool"sv, format("({} ? nk_true : nk_false)", arg.name)); },
                (_it_ == "struct nk_vec2") -->
                    tuple("vec2f"sv, format("nk_vec2({}.x(), {}.y())", arg.name, arg.name)),
                (_it_ == "struct nk_vec2i") -->
                    tuple("vec2i"sv, format("nk_vec2i(static_cast<short>({}.x()), static_cast<short>({}.y()))",
                                arg.name, arg.name)),
                (_it_ == "struct nk_color") -->
                    tuple("color_rgba"sv, format("nk_color{}{}.r(), {}.g(), {}.b(), {}.a(){}",
                                '{', arg.name, arg.name, arg.name, arg.name, '}')),
                (_it_ == "struct nk_colorf") -->
                    tuple("float_color_rgba"sv, format("nk_colorf{}{}.r(), {}.g(), {}.b(), {}.a(){}",
                                '{', arg.name, arg.name, arg.name, arg.name, '}')),
                (_it_ == "struct nk_color*") -->
                    tuple("color_rgba&"sv, format("reinterpret_cast<{}>(&{})",
                                (arg.is_const ? "const struct nk_color*" : "struct nk_color*"),
                                arg.name)),
                (_it_ == "struct nk_colorf*") -->
                    tuple("float_color_rgba&"sv, format("reinterpret_cast<{}>(&{})",
                                (arg.is_const ? "const struct nk_colorf*" : "struct nk_colorf*"),
                                arg.name)),
                noopt --> tuple(string_view(arg.type), arg.name)
            };
        };

        constexpr auto return_t_cast = [](string_view type, const string& expr) {
            return type / match {
                "nk_bool"sv        <- [&] { return tuple("bool"sv,  format("{} == nk_true", expr)); },
                "struct nk_vec2"sv <- [&] { return tuple("vec2f"sv,
                        format("{}({})", "[](struct nk_vec2 v) { return vec2f{v.x, v.y}; }", expr)); },
                "struct nk_vec2i"sv <- [&] { return tuple("vec2isv"sv,
                        format("{}({})", "[](struct nk_vec2i v) { return vec2i{v.x, v.y}; }", expr)); },
                "struct nk_color"sv <- [&] { return tuple("color_rgba"sv,
                        format("{}({})", "[](struct nk_color c) { return color_rgba{c.r, c.g, c.b, c.a}; }",
                            expr)); },
                "struct nk_colorf"sv <- [&] { return tuple("float_color_rgba"sv,
                        format("{}({})", "[](struct nk_colorf c) { return float_color_rgba{c.r, c.g, c.b, c.a}; }",
                            expr)); },
                noopt <- [&] { return tuple(string_view(type), expr); }
            };
        };

        auto signature_str = string(name) + "(";
        auto call_str = "nk_" + string(name) + "(";

        for (auto i = args.begin(); i != args.end(); ++i) {
            if (i->type == "struct nk_context*") {
                call_str += "_ctx";
            } else {
                signature_str += (i->is_const ? "const " : "");
                auto [type, cast_f] = arg_cast(*i);
                signature_str += build_string(type, " "sv, i->name);
                call_str += cast_f;

                if (i != std::prev(args.end()))
                    signature_str += ", ";
            }

            if (i != std::prev(args.end()))
                call_str += ", ";
        }
        signature_str += ")";
        call_str += ")";

        auto [ret_t, cast_call_str] = return_t_cast(return_type, call_str);

        std::cout << "    " << ret_t << " " << signature_str << " {\n";
        std::cout << "        return " << cast_call_str << ";\n";
        std::cout << "    }\n" << std::endl;
    };


    std::cout << "#pragma once\n"
                 "\n"
                 "#include <graphics/grx_types.hpp>\n"
                 "#include <nuklear.h>\n"
                 "\n"
                 "namespace ui {\n"
                 "using core::vec2f;\n"
                 "using core::vec2i;\n"
                 "using grx::color_rgba;\n"
                 "using grx::float_color_rgba;\n"
                 "\n"
                 "template <typename F>\n"
                 "using nkfunc = F*;\n"
                 "\n"
                 "class ui_nuklear_base {\n"
                 "public:\n";

    for (string sline; std::getline(std::cin, sline);) {
        string_view line = sline;
        auto start_args = string_view::npos;

        if (start_args == string_view::npos)
            start_args = line.find("(struct nk_context*");
        if (start_args == string_view::npos)
            start_args = line.find("(const struct nk_context*");
        if (start_args == string_view::npos)
            start_args = line.find("(const struct nk_context *");
        if (start_args == string_view::npos)
            start_args = line.find("(struct nk_context *");

        auto end_args = string_view::npos;
        if (start_args != string_view::npos)
            end_args = line.find(");", start_args);
        if (end_args == string_view::npos)
            continue;

        auto start_func_name = start_args - 1;
        auto end_func_name = start_args;
        ++start_args;

        char c = line[start_func_name];
        while (start_func_name < line.size() && ((isalpha(c) && c != ' ' && c != '\t') ||
               isdigit(c) || c == '_')) {
            --start_func_name;
            c = line[start_func_name];
        }
        ++start_func_name;

        if (start_func_name > line.size())
            continue;

        constexpr auto split_f = [parentheses = 0](char c) mutable {
            if (parentheses == 0 && c == ',')
                return true;
            if (c == '(')
                ++parentheses;
            else if (c == ')')
                --parentheses;
            return false;
        };

        auto args = line.substr(start_args, end_args - start_args) / views::split_when(split_f) /
                    views::sub(acts::trim(' ', '\t')) / acts::to<list<argument_t>>();
        auto func_name = line.substr(start_func_name, end_func_name - start_func_name);
        auto member_f_name = func_name.starts_with("nk_") ? func_name.substr(3) : func_name;

        auto return_type = line.substr(0, start_func_name);
        if (return_type.starts_with("extern "))
            return_type = return_type.substr("extern "sv.size());
        return_type = return_type / acts::trim(' ', '\t') / acts::to<string_view>();

        make_uniq_args(args);

        print_member_f(return_type, member_f_name, args);
    }

    std::cout << "protected:\n"
                 "    struct nk_context* _ctx;\n"
                 "};\n"
                 "} // namespace ui" << std::endl;
}

int pe_main(args_view args) {
    auto strmode = args.next("Missing mode argument");
    auto optmode = magic_enum::enum_cast<cmode_t>(strmode);
    cmode_t mode;
    if (optmode)
        mode = *optmode;
    else
        throw std::runtime_error("Invalid mode " + strmode);

    switch (mode) {
        case cmode_t::remove_comments:
            args.require_end();
            remove_comments();
            break;
        case cmode_t::c_preprocessor:
            return platform_dependent::exec("gcc", {"-E", "-x", "c", "-P", "-C", "-"});
            break;
        case cmode_t::cpp_preprocessor:
            return platform_dependent::exec("g++", {"-E", "-x", "c++", "-P", "-C", "-"});
            break;
        case cmode_t::nuklear_binding:
            nuklear_binding();
            break;
    }

    return 0;
}

