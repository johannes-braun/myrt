#pragma once

namespace myrt {
namespace flags_operators {
  template <typename E, typename = std::underlying_type_t<E>> flags<E> operator|(E flag, E value) {
    return flags<E>(flag) | value;
  }

  template <typename E, typename = std::underlying_type_t<E>> flags<E> operator&(E flag, E value) {
    return flags<E>(flag) | value;
  }

  template <typename E, typename = std::underlying_type_t<E>> flags<E> operator^(E flag, E value) {
    return flags<E>(flag) | value;
  }
} // namespace flags_operators

template <typename TEnum>
flags<TEnum>::flags(TEnum value) : _flags(static_cast<typename flags<TEnum>::base_type>(value)) {}

template <typename TEnum> flags<TEnum> operator|(TEnum flag, flags<TEnum> value) {
  return value | flag;
}

template <typename TEnum> flags<TEnum> operator&(TEnum flag, flags<TEnum> value) {
  return value & flag;
}

template <typename TEnum> flags<TEnum> operator^(TEnum flag, flags<TEnum> value) {
  return value ^ flag;
}

template <typename TEnum> flags<TEnum> flags<TEnum>::operator|(flags<TEnum> value) const {
  return flags<TEnum>(static_cast<TEnum>(_flags | value._flags));
}

template <typename TEnum> flags<TEnum> flags<TEnum>::operator&(flags<TEnum> value) const {
  return flags<TEnum>(static_cast<TEnum>(_flags & value._flags));
}

template <typename TEnum> flags<TEnum> flags<TEnum>::operator^(flags<TEnum> value) const {
  return flags<TEnum>(static_cast<TEnum>(_flags ^ value._flags));
}

template <typename TEnum> bool flags<TEnum>::operator!() const {
  return !static_cast<bool>(_flags);
}

template <typename TEnum> flags<TEnum> flags<TEnum>::operator~() const {
  return flags<TEnum>(static_cast<TEnum>(~_flags));
}

template <typename TEnum> bool flags<TEnum>::operator==(flags flags) const {
  return _flags == flags._flags;
}

template <typename TEnum> bool flags<TEnum>::operator!=(flags flags) const {
  return _flags != flags._flags;
}

template <typename TEnum> flags<TEnum>& flags<TEnum>::operator|=(flags flags) {
  _flags |= flags._flags;
  return *this;
}

template <typename TEnum> flags<TEnum>& flags<TEnum>::operator&=(flags flags) {
  _flags &= flags._flags;
  return *this;
}

template <typename TEnum> flags<TEnum>& flags<TEnum>::operator^=(flags flags) {
  _flags ^= flags._flags;
  return *this;
}

template <typename TEnum> bool flags<TEnum>::has(flags flags) const {
  return (_flags & flags._flags) != 0;
}

template <typename TEnum> flags<TEnum>::operator bool() const {
  return static_cast<bool>(_flags);
}

template <typename TEnum> flags<TEnum>::operator typename flags<TEnum>::enum_type() const {
  return static_cast<enum_type>(_flags);
}

template <typename TEnum> flags<TEnum>::operator typename flags<TEnum>::base_type() const {
  return _flags;
}
} // namespace myrt
