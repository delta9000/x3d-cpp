#ifndef X3D_RUNTIME_CALLBACK_LIFETIME_HPP
#define X3D_RUNTIME_CALLBACK_LIFETIME_HPP

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <utility>

namespace x3d::runtime {

/// Serial callback revocation. The owner never stores node pointers: escaped
/// callbacks hold only a weak lease, so retirement does not visit dead nodes or
/// clear a newer activation's handler. This is NOT a concurrency primitive.
class CallbackLifetime {
  struct State {
    bool live = true;
    std::size_t inFlight = 0;
  };

public:
  CallbackLifetime() = default;
  CallbackLifetime(const CallbackLifetime &) = delete;
  CallbackLifetime &operator=(const CallbackLifetime &) = delete;
  CallbackLifetime(CallbackLifetime &&) = delete;
  CallbackLifetime &operator=(CallbackLifetime &&) = delete;

  /// Destruction from within an owned callback is a contract violation. Fail
  /// closed rather than returning to a callback whose owner has been destroyed.
  ~CallbackLifetime() { if (!retire()) std::terminate(); }

  bool canRetire() const noexcept { return state_->inFlight == 0; }
  bool retired() const noexcept { return !state_->live; }
  [[nodiscard]] bool retire() noexcept {
    if (!canRetire()) return false;
    state_->live = false;
    return true;
  }

  class Invocation {
  public:
    explicit Invocation(std::shared_ptr<State> state) : state_(std::move(state)) {
      ++state_->inFlight;
    }
    Invocation(const Invocation &) = delete;
    Invocation &operator=(const Invocation &) = delete;
    ~Invocation() { --state_->inFlight; }
  private:
    std::shared_ptr<State> state_;
  };

  /// Bracket owner operations that can synchronously call user code as well as
  /// callbacks installed on nodes. The host treats retirement as terminal.
  Invocation enter() const { return Invocation(state_); }

  template <class F> auto guard(F callback) const {
    return [weak = std::weak_ptr<State>(state_),
            callback = std::move(callback)](auto &&...args) mutable {
      auto state = weak.lock();
      if (!state || !state->live) return;
      Invocation invocation(std::move(state));
      std::invoke(callback, std::forward<decltype(args)>(args)...);
    };
  }

  /// For predicates such as cascade input filters, an expired owner is neutral
  /// rather than rejecting unrelated inputs in a still-live context.
  template <class F, class R> auto guardOr(F callback, R fallback) const {
    return [weak = std::weak_ptr<State>(state_), callback = std::move(callback),
            fallback = std::move(fallback)](auto &&...args) mutable -> R {
      auto state = weak.lock();
      if (!state || !state->live) return fallback;
      Invocation invocation(std::move(state));
      return std::invoke(callback, std::forward<decltype(args)>(args)...);
    };
  }

private:
  std::shared_ptr<State> state_ = std::make_shared<State>();
};

} // namespace x3d::runtime
#endif
