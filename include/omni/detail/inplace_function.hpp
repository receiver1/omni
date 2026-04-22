#pragma once

#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "omni/detail/config.hpp"

namespace omni::detail {

  template <typename T, typename... Args>
  class inplace_function;

  template <typename T, typename... Args>
  class inplace_function<T(Args...)> {
   public:
    using function_ptr_t = T (*)(void*, Args&&...);
    using copy_ptr_t = void (*)(void*, const void*);
    using move_ptr_t = void (*)(void*, void*);
    using destructor_ptr_t = void (*)(void*);

    constexpr static std::size_t storage_size = 64;
    constexpr static std::size_t storage_alignment = alignof(std::max_align_t);

    inplace_function() = default;

    template <typename F, typename DecayedF = std::decay_t<F>>
      requires(!std::same_as<DecayedF, inplace_function> && std::is_invocable_r_v<T, DecayedF&, Args...> &&
               std::is_copy_constructible_v<DecayedF>)
    explicit(false) inplace_function(F&& func) {
      emplace<DecayedF>(std::forward<F>(func));
    }

    inplace_function(const inplace_function& other) {
      copy_from(other);
    }

    inplace_function(inplace_function&& other) noexcept {
      move_from(std::move(other));
    }

    inplace_function& operator=(const inplace_function& other) {
      if (this != &other) {
        inplace_function temp(other);
        reset();
        move_from(std::move(temp));
      }

      return *this;
    }

    inplace_function& operator=(inplace_function&& other) noexcept {
      if (this != &other) {
        reset();
        move_from(std::move(other));
      }

      return *this;
    }

    ~inplace_function() {
      reset();
    }

    void swap(inplace_function& other) noexcept {
      if (this == &other) {
        return;
      }

      inplace_function temp;
      temp.move_from(std::move(*this));
      move_from(std::move(other));
      other.move_from(std::move(temp));
    }

    [[nodiscard]] explicit operator bool() const noexcept {
      return invoker_ != nullptr;
    }

    T operator()(Args... args) {
      if (invoker_ == nullptr) {
#ifdef OMNI_HAS_EXCEPTIONS
        throw std::bad_function_call();
#else
        std::abort();
#endif
      }

      return invoker_(storage_ptr(), std::forward<Args>(args)...);
    }

   private:
    template <typename F>
    static F* storage_as(void* ptr) noexcept {
      return std::launder(reinterpret_cast<F*>(ptr));
    }

    template <typename F>
    static const F* storage_as(const void* ptr) noexcept {
      return std::launder(reinterpret_cast<const F*>(ptr));
    }

    [[nodiscard]] void* storage_ptr() noexcept {
      return static_cast<void*>(storage_);
    }

    [[nodiscard]] const void* storage_ptr() const noexcept {
      return static_cast<const void*>(storage_);
    }

    template <typename F, typename... CtorArgs>
    void emplace(CtorArgs&&... args) {
      static_assert(sizeof(F) <= storage_size, "Function object too large");
      static_assert(alignof(F) <= storage_alignment, "Function object alignment too large");

      new (storage_ptr()) F(std::forward<CtorArgs>(args)...);

      invoker_ = [](void* ptr, Args&&... args) -> T {
        return (*storage_as<F>(ptr))(std::forward<Args>(args)...);
      };

      copier_ = [](void* dst, const void* src) {
        new (dst) F(*storage_as<F>(src));
      };

      mover_ = [](void* dst, void* src) {
        if constexpr (std::is_move_constructible_v<F>) {
          new (dst) F(std::move(*storage_as<F>(src)));
        } else {
          new (dst) F(*storage_as<F>(src));
        }

        std::destroy_at(storage_as<F>(src));
      };

      destroyer_ = [](void* ptr) {
        std::destroy_at(storage_as<F>(ptr));
      };
    }

    void copy_from(const inplace_function& other) {
      if (!other) {
        return;
      }

      other.copier_(storage_ptr(), other.storage_ptr());
      invoker_ = other.invoker_;
      copier_ = other.copier_;
      mover_ = other.mover_;
      destroyer_ = other.destroyer_;
    }

    // NOLINTNEXTLINE(*-param-not-moved)
    void move_from(inplace_function&& other) {
      if (!other) {
        return;
      }

      other.mover_(storage_ptr(), other.storage_ptr());
      invoker_ = other.invoker_;
      copier_ = other.copier_;
      mover_ = other.mover_;
      destroyer_ = other.destroyer_;
      other.clear();
    }

    void reset() {
      if (destroyer_ != nullptr) {
        destroyer_(storage_ptr());
      }

      clear();
    }

    void clear() noexcept {
      invoker_ = nullptr;
      copier_ = nullptr;
      mover_ = nullptr;
      destroyer_ = nullptr;
    }

    alignas(storage_alignment) std::byte storage_[storage_size]{};
    function_ptr_t invoker_{nullptr};
    copy_ptr_t copier_{nullptr};
    move_ptr_t mover_{nullptr};
    destructor_ptr_t destroyer_{nullptr};
  };

} // namespace omni::detail
