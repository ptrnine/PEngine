#include "grx_utils.hpp"
#include <core/container_extensions.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace core;

namespace {
    struct order_value {
        string_view name;
        bool is_position; // true - position, false - bind_shape_matrix
        pair<size_t, size_t> span;
    };
}

/*
 * Shity function for baking bind_shape_matrix
 * Todo: recalc normals
 */
string grx_utils::collada_bake_bind_shape_matrix(string_view data) {
    using span_map = hash_map<string_view, pair<size_t, size_t>>;
    span_map position_spans, bind_shape_matrix_spans;
    vector<order_value> order;

    /*
     * Find position (vertices) span for every mesh
     */
    size_t i = 0;
    while ((i = data.find("-positions-array", i)) != string::npos) {
        auto end = i;
        i = data.rfind('\"', i);

        if (i > 3 && string_view(data.data() + i - 3, 3) == "id=") {
            string_view name(data.data() + i + 1, end - i - 1);

            auto pos_start = data.find('>', end);
            auto pos_end   = data.find('<', pos_start);

            if (pos_end != string::npos && pos_start != string::npos) {
                position_spans.emplace(name, pair(pos_start + 1, pos_end - pos_start - 1));
                order.emplace_back(order_value{name, true, pair(pos_start + 1, pos_end - pos_start - 1)});
            }
        }
        i = end + 1;
    }

    /*
     * Find bind shape matrix for every mesh
     */
    i = 0;
    while ((i = data.find("<skin source=\"#", i)) != string::npos) {
        i += sizeof("<skin source=\"#") - 1;
        auto end = data.find('\"', i);
        if (end == string::npos) break;

        string_view name(data.data() + i, end - i);

        i = data.find("<bind_shape_matrix>", i);
        if (i == string::npos) break;

        auto start = i + sizeof("<bind_shape_matrix>") - 1;
        end = data.find('<', start);

        if (start != string::npos && end != string::npos) {
            bind_shape_matrix_spans.emplace(name, pair(start, end - start));
            order.emplace_back(order_value{name, false, pair(start, end - start)});
        }
    }
    if (bind_shape_matrix_spans.empty())
        return core::string(data);

    /*
     * Sort spans by entry position
     */
    std::sort(order.begin(), order.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.span.first < rhs.span.first;
    });

    constexpr auto identity = string_view("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
    string new_data;
    new_data.reserve(data.size());

    /*
     * Fold new data string
     */
    size_t next_start = 0;
    for (auto& [name, is_position, span] : order) {
        new_data += string_view(data.data() + next_start, span.first - next_start);
        next_start = span.first + span.second;

        /*
         * If it is matrix span - append identity and go next
         */
        if (!is_position) {
            new_data += identity;
            continue;
        }

        auto span_pos = bind_shape_matrix_spans.find(name);

        if (span_pos != bind_shape_matrix_spans.end()) {
            string_view matrix_str(data.data() + span_pos->second.first, span_pos->second.second);

            /*
             * Get matrix from sequence
             */
            array<float, 16> raw_matrix;
            auto splits = matrix_str / split(' ');
            for (size_t k = 0; k < std::min(splits.size(), size_t(16)); ++k)
                raw_matrix[k] = splits[k] / to_number<float>();

            auto matrix = glm::transpose(glm::make_mat4x4(raw_matrix.data()));

            auto positions_span = position_spans[name];
            string_view positions_str(data.data() + positions_span.first, positions_span.second);
            splits = positions_str / split(' ');

            string output_positions;
            output_positions.reserve(positions_str.size());

            /*
             * Perform transformation and push vertex to output_positions
             */
            for (size_t k = 0; k < splits.size() / 3; ++k) {
                auto vec = matrix * glm::vec4(
                        splits[k * 3 + 0] / to_number<float>(),
                        splits[k * 3 + 1] / to_number<float>(),
                        splits[k * 3 + 2] / to_number<float>(), 1.f);

                output_positions += std::to_string(vec.x) + ' ' +
                                    std::to_string(vec.y) + ' ' +
                                    std::to_string(vec.z) + ' ';
            }
            output_positions.pop_back();

            new_data += output_positions;
        }
    }
    new_data += string_view(data.data() + next_start);

    return new_data;
}