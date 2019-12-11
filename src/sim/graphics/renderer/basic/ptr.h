#pragma once
#include <cstdint>
#include <vector>

namespace sim::graphics::renderer::basic {
template<class Type>
struct Ptr {
  static Ptr<Type> add(std::vector<Type> &vec, Type &&element) {
    vec.push_back(std::move(element));
    return Ptr(&vec, uint32_t(vec.size() - 1));
  }
  static Ptr<Type> add(std::vector<Type> &vec, Type &element) {
    vec.push_back(element);
    return Ptr(&vec, uint32_t(vec.size() - 1));
  }

  explicit Ptr(std::vector<Type> *vec = nullptr, uint32_t element = 0)
    : vec{vec}, _index{element} {}

  const Type &get() const { return vec->at(_index); }
  Type &operator*() { return vec->at(_index); }
  Type *operator->() { return &vec->at(_index); }
  explicit operator bool() const { return vec != nullptr; }
  uint32_t index() const { return _index; }
  bool isNull() const { return vec == nullptr; }

private:
  std::vector<Type> *vec;
  uint32_t _index{};
};
}
