#pragma once

#include <compare>
#include <memory>
#include <map>
#include <mygl/mygl.hpp>

namespace myrt {
struct texture_info_t {
  GLenum target;
  GLenum internal_format;
  int levels;
  int samples;
  int width;
  int height;
  int depth;

  auto operator<=>(const texture_info_t&) const = default;
};

class resident_texture_t;

class texture_t {
public:
  friend class texture_provider_t;
  void lock() {
    m_locked_in_frame = true;
    m_reserved_in_frame = true;
  }
  void unlock() {
    m_locked_in_frame = false;
  }
  std::uint32_t id() const {
    return m_id;
  }

private:
  std::uint32_t m_id;
  bool m_reserved_in_frame = false;
  bool m_locked_in_frame = false;
};

class resident_texture_t {
  friend std::shared_ptr<resident_texture_t> make_resident(std::shared_ptr<texture_t> texture);

public:
  ~resident_texture_t() {
    if (glIsTextureHandleResidentARB(m_handle))
      glMakeTextureHandleNonResidentARB(m_handle);
    m_texture->unlock();
  }

protected:
  resident_texture_t(std::shared_ptr<texture_t> texture) : m_texture(std::move(texture)) {
    m_texture->lock();
    m_handle = glGetTextureHandleARB(texture->id());
    glMakeTextureHandleResidentARB(m_handle);
  }

private:
  std::uint64_t m_handle;
  std::shared_ptr<texture_t> m_texture;
};

inline std::shared_ptr<resident_texture_t> make_resident(std::shared_ptr<texture_t> texture) {
  return std::unique_ptr<resident_texture_t>(new resident_texture_t(std::move(texture)));
}

class texture_provider_t {
public:
  void new_frame();
  std::shared_ptr<texture_t> get(GLenum target, GLenum fmt, int w, int layers);
  std::shared_ptr<texture_t> get(GLenum target, GLenum fmt, int w, int h, int layers);
  std::shared_ptr<texture_t> get(GLenum target, GLenum fmt, int w, int h, int d, int layers);
  std::shared_ptr<texture_t> get_ms(GLenum target, GLenum fmt, int w, int h, int samples);
  std::shared_ptr<texture_t> find(std::uint32_t id);

private:
  std::shared_ptr<texture_t> get_from(texture_info_t info);
  std::shared_ptr<texture_t> create_new_texture(texture_info_t info);
  std::multimap<texture_info_t, std::shared_ptr<texture_t>> m_allocated_textures;
};
} // namespace myrt