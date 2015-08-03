#ifndef BUF_ALLOCATOR_HPP
#define BUF_ALLOCATOR_HPP

#include <memory>

template<typename T>
class buf_allocator : public std::allocator<T> {
 public:
  template<typename _Tp1>
  struct rebind {
    typedef buf_allocator<_Tp1> other;
  };

  T* allocate(size_t n, const void *hint = 0) {
    if (buf_ == NULL) {
      return std::allocator<T>::allocate(n, hint);
    }
    return buf_;
  }

  void deallocate(T* p, size_t n) {
    if (buf_ == NULL) {
      std::allocator<T>::deallocate(p, n);
    }
  }

  buf_allocator() throw ()
      : std::allocator<T>() {
    buf_ = NULL;
  }

  buf_allocator(const std::allocator<T> &a) throw ()
      : std::allocator<T>(a) {
    buf_ = NULL;
  }

  buf_allocator(const buf_allocator &a) throw ()
      : std::allocator<T>(a) {
    buf_ = NULL;
  }

  buf_allocator(T* buf) throw ()
      : std::allocator<T>() {
    buf_ = buf;
  }

 private:
  T* buf_;

};

#endif /* BUF_ALLOCATOR_HPP */
