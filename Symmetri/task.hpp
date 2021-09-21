#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

namespace symmetri {

using namespace std;

template <class> class task;

template <class R, class... Args> class task<R(Args...)> {
  struct concept;

  template <class F, bool Small> struct model;
  static constexpr size_t small_size = sizeof(void *) * 4;

  static constexpr concept empty{[](void *) noexcept {}};

  const concept *_concept = &empty;
  aligned_storage_t<small_size> _model;

public:
  template <class F> task(F &&f) {
    constexpr bool is_small = sizeof(model<decay_t<F>, true>) <= small_size;
    new (&_model) model<decay_t<F>, is_small>(forward<F>(f));
    _concept = &model<decay_t<F>, is_small>::vtable;
  }

  ~task() { _concept->_dtor(&_model); }

  task(task &&x) noexcept : _concept(x._concept) {
    _concept->_move(&x._model, &_model);
  }
  task &operator=(task &&x) noexcept {
    if (this == &x)
      return *this;
    _concept->_dtor(&_model);
    _concept = x._concept;
    _concept->_move(&x._model, &_model);
    return *this;
  }
  R operator()(Args... args) {
    return _concept->_invoke(&_model, forward<Args>(args)...);
  }
};

template <class R, class... Args> struct task<R(Args...)>::concept {
  void (*_dtor)(void *) noexcept;
  void (*_move)(void *, void *) noexcept;
  R (*_invoke)(void *, Args &&...);
};

template <class R, class... Args>
template <class F>
struct task<R(Args...)>::model<F, true> {
  template <class G> model(G &&f) : _f(forward<G>(f)) {}

  static void _dtor(void *self) noexcept {
    static_cast<model *>(self)->~model();
  }
  static void _move(void *self, void *p) noexcept {
    new (p) model(move(*static_cast<model *>(self)));
  }
  static R _invoke(void *self, Args &&... args) {
    return invoke(static_cast<model *>(self)->_f, forward<Args>(args)...);
  }

  static constexpr concept vtable{_dtor, _move, _invoke};

  F _f;
};

template <class R, class... Args>
template <class F>
struct task<R(Args...)>::model<F, false> {
  template <class G> model(G &&f) : _p(make_unique<F>(forward<F>(f))) {}

  static void _dtor(void *self) noexcept {
    static_cast<model *>(self)->~model();
  }
  static void _move(void *self, void *p) noexcept {
    new (p) model(move(*static_cast<model *>(self)));
  }
  static R _invoke(void *self, Args &&... args) {
    return invoke(*static_cast<model *>(self)->_p, forward<Args>(args)...);
  }

  static constexpr concept vtable{_dtor, _move, _invoke};

  unique_ptr<F> _p;
};

} // namespace symmetri
