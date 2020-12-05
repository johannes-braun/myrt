#include "obj.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <bit>

namespace myrt::obj
{
    std::unordered_map<std::string, std::shared_ptr<material_t>> load_mtllib(std::filesystem::path const& mtl_file)
    {
        std::unordered_map<std::string, std::shared_ptr<material_t>> result;
        std::shared_ptr<material_t> current_material;

        std::ifstream fstream(mtl_file);

        for (std::string line; std::getline(fstream, line);)
        {
            std::stringstream line_stream(line);

            while ((line_stream.peek() == ' ' || line_stream.peek() == '\t') && !line_stream.eof())
                line_stream.ignore();

            std::string identifier;
            line_stream >> identifier;

            if (identifier == "newmtl")
            {
                std::string name;
                line_stream >> name;
                current_material = result[name] = std::make_shared<material_t>();
                current_material->name = std::move(name);
            }
            else if (identifier == "Ns")
            {
                line_stream >> current_material->specularity;
            }
            else if (identifier == "Ni")
            {
                line_stream >> current_material->ior;
            }
            else if (identifier == "d")
            {
                line_stream >> current_material->dissolve;
            }
            else if (identifier == "illum")
            {
                line_stream >> current_material->illumination_model;
            }
            else if (identifier == "Ka")
            {
                line_stream >> current_material->ambient[0];
                line_stream >> current_material->ambient[1];
                line_stream >> current_material->ambient[2];
            }
            else if (identifier == "Kd")
            {
                line_stream >> current_material->diffuse[0];
                line_stream >> current_material->diffuse[1];
                line_stream >> current_material->diffuse[2];
            }
            else if (identifier == "Ks")
            {
                line_stream >> current_material->specular[0];
                line_stream >> current_material->specular[1];
                line_stream >> current_material->specular[2];
            }
            else if (identifier == "Ke")
            {
                line_stream >> current_material->emissive[0];
                line_stream >> current_material->emissive[1];
                line_stream >> current_material->emissive[2];
            }
        }

        return result;
    }

    std::shared_ptr<material_t> make_default_material() {
        return std::make_shared<material_t>(material_t{
            .name = "Default",
            .specularity = 100,
            .ambient = {1, 1, 1},
            .diffuse = {1, 0, 1},
            .specular = {1, 1, 1},
            .emissive = {0, 0, 0},
            .ior = 1.45,
            .dissolve = 1.0,
            .illumination_model = 1
            });
    }

    std::vector<object_t> load_obj(std::filesystem::path const& obj_file)
    {
        std::vector<object_t> result;

        size_t pos_index_offset = 1;
        size_t tex_index_offset = 1;
        size_t nor_index_offset = 1;
        std::ifstream fstream(obj_file);

        std::unordered_map<std::string, std::shared_ptr<material_t>> mtllib;
        std::shared_ptr<material_t> current_material = make_default_material();

        int object_counter = 0;

        for (std::string line; std::getline(fstream, line);)
        {
            std::stringstream line_stream(line);

            while ((line_stream.peek() == ' ' || line_stream.peek() == '\t') && !line_stream.eof())
                line_stream.ignore();

            std::string identifier;
            line_stream >> identifier;

            if (identifier == "mtllib")
            {
                mtllib.clear();
                while (!line_stream.eof())
                {
                    std::string mtllib_file;
                    line_stream >> mtllib_file;
                    auto x = load_mtllib(obj_file.parent_path() / mtllib_file);
                    mtllib.insert(x.begin(), x.end());
                }
            }
            else if (identifier == "usemtl")
            {
                std::string name;
                line_stream >> name;
                current_material = mtllib[name];
                if (result.back().groups.empty() || !result.back().groups.back().faces.empty())
                {
                    result.back().groups.emplace_back().name = name;
                }
                result.back().groups.back().material = current_material;
            }
            else if (identifier == "o")
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
                auto& next = result.back().groups.emplace_back();
                line_stream >> next.name;
                next.material = current_material;
                continue;
            }
            else if (identifier == "v")
            {
                if (result.empty())
                    result.emplace_back().name = "Default";

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
                if (result.back().normals.empty())
                    result.back().normals.push_back({ 0, 1, 0 });

                if (result.back().groups.empty())
                    result.back().groups.emplace_back().name = "Default";

                auto& face = result.back().groups.back().faces.emplace_back();
                line_stream.ignore();
                for (std::string vertex; std::getline(line_stream, vertex, ' ');)
                {
                    std::stringstream vertex_stream(vertex);
                    auto& v = face.vertices.emplace_back();

                    int p = 0;
                    int t = 0;
                    int n = 0;

                    vertex_stream >> p;
                    vertex_stream.ignore();
                    vertex_stream >> t;
                    vertex_stream.ignore();
                    vertex_stream >> n;

                    if (p < 0) v[0] = result.back().positions.size() + p;
                    else if (p != 0) v[0] = p - pos_index_offset;
                    else v[0] = p;

                    if (t < 0) v[1] = result.back().texcoords.size() + t;
                    else if (t != 0) v[1] = t - tex_index_offset;
                    else v[1] = t;

                    if (n < 0) v[2] = result.back().normals.size() + n;
                    else if (n != 0) v[2] = n - nor_index_offset;
                    else v[2] = n;
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

        std::vector<triangulated_object_t> result;
        for (const auto& group : object.groups)
        {
            std::unordered_map<face_identifier, unsigned, face_hasher> face_singulator;
            auto& triangulated = result.emplace_back();
            triangulated.material = group.material;
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