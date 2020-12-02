#include "obj.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <bit>

namespace myrt::obj
{
    std::vector<object_t> load_obj(std::filesystem::path const& obj_file)
    {
        std::vector<object_t> result;

        size_t pos_index_offset = 1;
        size_t tex_index_offset = 1;
        size_t nor_index_offset = 1;
        std::ifstream fstream(obj_file);
        for (std::string line; std::getline(fstream, line);)
        {
            std::stringstream line_stream(line);
            std::string identifier;
            line_stream >> identifier;

            if (identifier == "o")
            {
                if (!result.empty())
                {
                    pos_index_offset += result.back().positions.size();
                    tex_index_offset += result.back().texcoords.size();
                    nor_index_offset += result.back().normals.size();
                }

                line_stream >> result.emplace_back().name;
                continue;
            }
            else if (identifier == "g")
            {
                line_stream >> result.back().groups.emplace_back().name;
                continue;
            }
            else if (identifier == "v")
            {
                auto& arr = result.back().positions.emplace_back();
                line_stream >> arr[0];
                line_stream >> arr[1];
                line_stream >> arr[2];
                continue;
            }
            else if (identifier == "vn")
            {
                auto& arr = result.back().normals.emplace_back();
                line_stream >> arr[0];
                line_stream >> arr[1];
                line_stream >> arr[2];
                continue;
            }
            else if (identifier == "vt")
            {
                auto& arr = result.back().texcoords.emplace_back();
                line_stream >> arr[0];
                line_stream >> arr[1];
                continue;
            }
            else if (identifier == "f")
            {
                auto& face = result.back().groups.back().faces.emplace_back();
                line_stream.ignore();
                for (std::string vertex; std::getline(line_stream, vertex, ' ');)
                {
                    std::stringstream vertex_stream(vertex);
                    auto& v = face.vertices.emplace_back();
                    vertex_stream >> v[0];
                    vertex_stream.ignore();
                    vertex_stream >> v[1];
                    vertex_stream.ignore();
                    vertex_stream >> v[2];

                    v[0]-= pos_index_offset;
                    v[1]-= tex_index_offset;
                    v[2]-= nor_index_offset;
                }
            }
        }

        return result;
    }

    using face_identifier = std::tuple<vec3_type, vec3_type, vec2_type>;

    struct face_hasher
    {
        constexpr static size_t prime_01 = 7'591'916'504'200'993'751;
        constexpr static size_t prime_02 = 8'774'419'696'331'388'221;
        constexpr static size_t prime_03 = 9'880'948'031'181'248'789;
        constexpr static size_t prime_04 = 18'400'857'061'147'049'977;
        constexpr static size_t prime_05 = 260'456'530'330'231'337;
        constexpr static size_t prime_06 = 12'155'843'046'296'014'969;
        constexpr static size_t prime_07 = 15'778'340'322'080'424'971;
        constexpr static size_t prime_08 = 9'127'590'151'023'692'563;


        size_t operator()(face_identifier const& tup) const
        {
            const auto h01 = std::rotl(std::hash<float>{}(std::get<0>(tup)[0]) ^ prime_01, 0);
            const auto h02 = std::rotl(std::hash<float>{}(std::get<0>(tup)[1]) ^ prime_02, 4);
            const auto h03 = std::rotl(std::hash<float>{}(std::get<0>(tup)[2]) ^ prime_03, 8);
            const auto h04 = std::rotl(std::hash<float>{}(std::get<1>(tup)[0]) ^ prime_04, 16);
            const auto h05 = std::rotl(std::hash<float>{}(std::get<1>(tup)[1]) ^ prime_05, 24);
            const auto h06 = std::rotl(std::hash<float>{}(std::get<1>(tup)[2]) ^ prime_06, 32);
            const auto h07 = std::rotl(std::hash<float>{}(std::get<2>(tup)[0]) ^ prime_07, 48);
            const auto h08 = std::rotl(std::hash<float>{}(std::get<2>(tup)[1]) ^ prime_08, 56);
            return h01 ^ h02 ^ h03 ^ h04 ^ h05 ^ h06 ^ h07 ^ h08;
        }
    };

    std::vector<triangulated_object_t> triangulate(object_t const& object)
    {
        std::unordered_map<face_identifier, unsigned, face_hasher> face_singulator;

        std::vector<triangulated_object_t> result;
        for (const auto& group : object.groups)
        {
            auto& triangulated = result.emplace_back();

            for (const auto& face : group.faces)
            {
                if (face.vertices.size() <= 2)
                    continue;

                vec3_type pos0 = object.positions[face.vertices[0][0]];
                vec2_type tex0 = object.texcoords[face.vertices[0][1]];
                vec3_type nor0 = object.normals[face.vertices[0][2]];

                const auto [iter0, success0] = face_singulator.emplace(face_identifier{ pos0, nor0, tex0 }, unsigned(triangulated.positions.size()));
                if (success0)
                {
                    triangulated.positions.push_back(pos0);
                    triangulated.normals.push_back(nor0);
                    triangulated.texcoords.push_back(tex0);
                }

                vec3_type pos1 = object.positions[face.vertices[1][0]];
                vec2_type tex1 = object.texcoords[face.vertices[1][1]];
                vec3_type nor1 = object.normals[face.vertices[1][2]];

                const auto [iter1, success1] = face_singulator.emplace(face_identifier{ pos1, nor1, tex1 }, unsigned(triangulated.positions.size()));
                if (success1)
                {
                    triangulated.positions.push_back(pos1);
                    triangulated.normals.push_back(nor1);
                    triangulated.texcoords.push_back(tex1);
                }

                unsigned last_vertex_index = iter1->second;

                // For each following vertex, fan them out by using the first and last vertices.
                for (size_t i = 2; i < face.vertices.size(); ++i)
                {

                    vec3_type pos = object.positions[face.vertices[i][0]];
                    vec2_type tex = object.texcoords[face.vertices[i][1]];
                    vec3_type nor = object.normals[face.vertices[i][2]];

                    const auto [iter, success] = face_singulator.emplace(face_identifier{ pos, nor, tex }, unsigned(triangulated.positions.size()));
                    triangulated.indices.push_back(iter0->second);
                    triangulated.indices.push_back(last_vertex_index);
                    triangulated.indices.push_back(iter->second);
                    if (success)
                    {
                        triangulated.positions.push_back(pos);
                        triangulated.normals.push_back(nor);
                        triangulated.texcoords.push_back(tex);
                    }
                    last_vertex_index = iter->second;
                }
            }
        }
        return result;
    }
}