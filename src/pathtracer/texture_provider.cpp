#pragma once

#include "texture_provider.hpp"

namespace myrt
{
    void texture_provider_t::new_frame()
    {
        for (auto iter = m_allocated_textures.begin(); iter != m_allocated_textures.end();)
        {
            iter->second->m_locked_in_frame = false;
            if (iter->second->m_reserved_in_frame == 0 && iter->second.use_count() <= 1)
            {
                printf("Releasing texture [%d]\n", int(iter->second->m_id));
                glDeleteTextures(1, &iter->second->m_id);
                iter = m_allocated_textures.erase(iter);
            }
            else
            {
                iter->second->m_reserved_in_frame = false;
                ++iter;
            }
        }
    }
    std::shared_ptr<texture_t> texture_provider_t::get_ms(GLenum target, GLenum fmt, int w, int h, int samples)
    {
        return get_from(texture_info_t{
            .target = target,
            .internal_format = fmt,
            .levels = 0,
            .samples = samples,
            .width = w,
            .height = h,
            .depth = 1 });
    }
    std::shared_ptr<texture_t> texture_provider_t::find(std::uint32_t id)
    {
        if (auto const it = std::find_if(m_allocated_textures.begin(), m_allocated_textures.end(), [&](const decltype(m_allocated_textures)::value_type& pair) {
            return pair.second && pair.second->id() == id;
            }); it != m_allocated_textures.end())
            return it->second;
            return std::shared_ptr<texture_t>();
    }
    std::shared_ptr<texture_t> texture_provider_t::get(GLenum target, GLenum fmt, int w, int h, int d, int levels)
    {
        return get_from(texture_info_t{
            .target = target,
            .internal_format = fmt,
            .levels = levels,
            .samples = 0,
            .width = w,
            .height = h,
            .depth = d });
    }

    std::shared_ptr<texture_t> texture_provider_t::get(GLenum target, GLenum fmt, int w, int h, int levels)
    {
        return get_from(texture_info_t{
            .target = target,
            .internal_format = fmt,
            .levels = levels,
            .samples = 0,
            .width = w,
            .height = h,
            .depth = 0 });
    }

    std::shared_ptr<texture_t> texture_provider_t::get(GLenum target, GLenum fmt, int w, int levels)
    {
        return get_from(texture_info_t{
            .target = target,
            .internal_format = fmt,
            .levels = levels,
            .samples = 0,
            .width = w,
            .height = 0,
            .depth = 0 });
    }
    std::shared_ptr<texture_t> texture_provider_t::get_from(texture_info_t info)
    {
        if (auto it = m_allocated_textures.find(info); it != m_allocated_textures.end())
        {
            while (it != m_allocated_textures.end() && it->first == info && (it->second.use_count() > 1 || it->second->m_locked_in_frame))
                ++it;
            if (it == m_allocated_textures.end() || it->first != info)
                return create_new_texture(info);

            it->second->m_reserved_in_frame = true;
            return it->second;
        }
        return create_new_texture(info);
    }
    std::shared_ptr<texture_t> texture_provider_t::create_new_texture(texture_info_t info)
    {
        texture_t tex;
        tex.m_reserved_in_frame = true;
        glCreateTextures(info.target, 1, &tex.m_id);
        switch (info.target)
        {
        case GL_TEXTURE_1D:
            glTextureStorage1D(tex.m_id, info.levels, info.internal_format, info.width);
            break;
        case GL_TEXTURE_2D:
            glTextureStorage2D(tex.m_id, info.levels, info.internal_format, info.width, info.height);
            break;
        case GL_TEXTURE_2D_ARRAY:
            glTextureStorage3D(tex.m_id, info.levels, info.internal_format, info.width, info.height, info.depth);
            break;
        case GL_TEXTURE_CUBE_MAP:
            glTextureStorage2D(tex.m_id, info.levels, info.internal_format, info.width, info.height);
            break;
        case GL_TEXTURE_3D:
            glTextureStorage3D(tex.m_id, info.levels, info.internal_format, info.width, info.height, info.depth);
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
            glTextureStorage2DMultisample(tex.m_id, info.samples, info.internal_format, info.width, info.height, true);
            break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            glTextureStorage3DMultisample(tex.m_id, info.samples, info.internal_format, info.width, info.height, info.depth, true);
            break;
        default:
            break;
        }
        printf("Allocated new texture [%d]\n", int(tex.m_id));
        auto ptr = std::make_shared<texture_t>(tex);
        m_allocated_textures.emplace(std::move(info), ptr);
        return ptr;
    }
}