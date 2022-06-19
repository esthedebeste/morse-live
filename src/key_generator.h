#include <coroutine>
struct awaitable_keyup {
  bool await_ready();
  void await_suspend(std::coroutine_handle<> h);
  void await_resume();
};
awaitable_keyup wait_for_keyup();
struct awaitable_keydown {
  bool await_ready();
  void await_suspend(std::coroutine_handle<> h);
  void await_resume();
};
awaitable_keydown wait_for_keydown();
void init_key_generator();
void run_key_generator();