#include <core/main.cpp>
#include <core/types.hpp>
#include <core/container_extensions.hpp>

using namespace core;

string read_anims(const string& filename) {
    auto data = read_file_unwrap(filename);

    auto start = data.find("<library_animations>");
    if (start == string::npos)
        std::runtime_error("No animations in " + filename);
    auto end = data.find("</library_animations>", start);

    start += sizeof("<library_animations>") - 1;
    return data.substr(start, end - start);
}

string set_anim_name(const string& anim_data, const string& anim_name) {
    string result;

    string::size_type last_start = 0;
    auto start = anim_data.find("<animation id=");

    while (start != string::npos) {
        auto pos = anim_data.find("name=\"", start);
        if (pos == string::npos)
            throw std::runtime_error("Missing name");

        result += string_view(anim_data).substr(last_start, pos - last_start);

        pos += sizeof("name=\"") - 1;
        pos = anim_data.find('"', pos);
        last_start = pos + 1;

        start = anim_data.find("<animation id=", last_start);
        result += "name=\"" + anim_name + "\"";
    }

    result += string_view(anim_data).substr(last_start);
    return result;
}

string insert_animations(const string& output_name, const vector<pair<string, string>>& anim_data) {
    auto data = read_file_unwrap(output_name);
    auto start = data.find("<library_animations>");

    if (start == string::npos)
        throw std::runtime_error("No <libraty_animations> section in " + output_name);

    start += sizeof("<library_animations>") - 1;

    string output = data.substr(0, start);
    for (auto& [anim, _] : anim_data)
        output += anim;

    output += string_view(data).substr(start);

    return output;
}

int pe_main(args_view args) {
    vector<pair<string, string>> anim_data;
    string output_name;

    while (auto fname_opt = args.next()) {
        auto& fname = *fname_opt;
        auto splits = fname / split(':');
        if (splits.size() == 1) {
            if (!output_name.empty())
                throw std::runtime_error("Duplicated output name");
            output_name = splits.front();
        } else if (splits.size() > 1) {
            anim_data.emplace_back(splits[0], splits[1]);
        }
    }

    if (anim_data.empty())
        throw std::runtime_error("Missing files for merge");

    for (auto& [file_name, anim_name] : anim_data) {
        /* Replace file name with anim data */
        file_name = set_anim_name(read_anims(file_name), anim_name);
    }

    auto output = insert_animations(output_name, anim_data);
    write_file_unwrap(output_name, output);

    return 0;
}
