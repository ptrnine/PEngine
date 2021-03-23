#include "grx_skybox.hpp"
#include <GL/glew.h>

namespace grx::details {
    using namespace core;

    uint load_skybox(const array<grx_float_color_map_rgb, 6>& maps) {
        uint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
        for (uint i = 0; i < maps.size(); ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0,
                         GL_RGB16F,
                         static_cast<GLsizei>(maps[i].size().x()),
                         static_cast<GLsizei>(maps[i].size().y()),
                         0,
                         GL_RGB,
                         GL_FLOAT,
                         maps[i].data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return id;
    }

    void skybox_bind_texture(uint id) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    }
}
