#pragma once

#include <type_traits>

namespace myrt {
template <typename TEnum> class flags {
public:
  using base_type = std::underlying_type_t<TEnum>;
  using enum_type = TEnum;
  constexpr static base_type all_set = ~base_type(0);

  flags() = default;
  flags(TEnum value);
  flags(const flags& other) = default;
  flags(flags&& other) noexcept = default;
  flags& operator=(const flags& other) = default;
  flags& operator=(flags&& other) noexcept = default;

  friend flags operator|(enum_type flag, flags value);
  friend flags operator&(enum_type flag, flags value);
  friend flags operator^(enum_type flag, flags value);

  flags operator|(flags value) const;
  flags operator&(flags value) const;
  flags operator^(flags value) const;
  bool operator!() const;
  flags operator~() const;

  bool operator==(flags flags) const;
  bool operator!=(flags flags) const;
  flags& operator|=(flags flags);
  flags& operator&=(flags flags);
  flags& operator^=(flags flags);

  bool has(flags flags) const;

  explicit operator bool() const;
  explicit operator enum_type() const;
  explicit operator base_type() const;

private:
  base_type _flags = 0;
};
} // namespace myrt

#include "flags.inl"