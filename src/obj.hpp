#pragma once

#include <array>
#include <string>
#include <optional>
#include <filesystem>
#include <vector>

namespace myrt::obj
{
    using vec3_type = std::array<float, 3>;
    using vec2_type = std::array<float, 2>;

    struct face_t
    {
        std::vector<std::array<unsigned, 3>> vertices;
    };

    struct vertex_group_t
    {
        std::string name;
        std::vector<face_t> faces;
    };

    struct object_t
    {

        std::string name;

        std::vector<vertex_group_t> groups;

        std::vector<vec3_type> positions;
        std::vector<vec3_type> normals;
        std::vector<vec2_type> texcoords;
    };

    std::vector<object_t> load_obj(std::filesystem::path const& obj_file);

    struct triangulated_object_t
    {
        std::vector<unsigned> indices;
        std::vector<vec3_type> positions;
        std::vector<vec3_type> normals;
        std::vector<vec2_type> texcoords;
    };
    std::vector<triangulated_object_t> triangulate(object_t const& object);
}