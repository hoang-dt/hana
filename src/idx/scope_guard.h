/**\file
Stolen from the facebook's folly library.
*/

#pragma once

#include "macros.h"
#include <utility>

namespace hana {

template <typename FunctionType>
class ScopeGuard {
  private:
    void* operator new(size_t) = delete;

    void execute() noexcept { function_(); }

    FunctionType function_;
    bool dismissed_ = false;

  public:
    explicit ScopeGuard(const FunctionType& fn)
        : function_(fn) {}

    explicit ScopeGuard(FunctionType&& fn) noexcept
        : function_(std::move(fn)) {}

    ScopeGuard(ScopeGuard&& other) noexcept
        : dismissed_(other.dismissed_)
        , function_(std::move(other.function_))
    { other.dismissed_ = true; }

    void dismiss() noexcept { dismissed_ = true; }
};

namespace detail {
enum class ScopeGuardOnExit {};

template <typename FunctionType>
ScopeGuard<typename std::decay<FunctionType>::type>
operator+(ScopeGuardOnExit, FunctionType&& fn)
{
    return ScopeGuard<typename std::decay<FunctionType>::type>(
        std::forward<FunctionType>(fn));
}
} // end namespace detail

#define HANA_SCOPE_EXIT \
    auto HANA_ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) \
    = ::hana::detail::ScopeGuardOnExit() + [&]()

}