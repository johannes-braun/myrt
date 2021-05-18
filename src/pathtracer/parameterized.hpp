#pragma once

#include "parameter.hpp"
#include <map>
#include <unordered_map>
#include <memory>

namespace myrt {
  struct parameterized_type {
    std::map<std::string, std::shared_ptr<parameter_type>> parameter_types;
  };

  template<typename Type>
  struct parameterized {
    parameterized(std::shared_ptr<Type> type)
      : m_type(std::move(type)) {
      for (auto const& [name, ty] : m_type->parameter_types)
      {
        m_parameters[name] = std::make_shared<parameter>(ty);
      }
    }

    std::shared_ptr<Type>const& type() const { return m_type; }
    std::map<std::string, std::shared_ptr<parameter>> const& parameters() const { return m_parameters; }

  private:
    std::shared_ptr<Type> m_type;
    std::map<std::string, std::shared_ptr<parameter>> m_parameters;
  };

  template<typename Base>
  struct basic_type {
    template<typename... Args>
    basic_type(std::map<std::string, std::shared_ptr<parameter_type>> parameter_types, Args&&... ctor_args)
    {
      m_type = std::make_shared<Base>(std::forward<Args>(ctor_args)...);
      m_type->parameter_types = std::move(parameter_types);
    }

    operator std::shared_ptr<Base> const& () const {
      return m_type;
    }

  protected:
    std::shared_ptr<Base> m_type;
  };

  template<typename Base, typename Type>
  struct basic_object {
    template<typename... Args>
    basic_object(std::shared_ptr<Type> type, Args&&... ctor_args)
    {
      m_ptr = std::make_shared<Base>(std::move(type), std::forward<Args>(ctor_args)...);
    }

    operator std::shared_ptr<Base> const& () const {
      return m_ptr;
    }

    std::shared_ptr<parameter> at(std::string const& pname) const {
      if (m_ptr->parameters().contains(pname))
        return m_ptr->parameters().at(pname);
      return nullptr;
    }

    template<typename T>
    void set(std::string const& name, T&& value) const {
      myrt::parameter_buffer_description{}.
        set_value(nullptr, at(name), std::forward<T>(value));
    }

  protected:
    std::shared_ptr<Base> m_ptr;
  };
}