#include "key_generator.h"
#include "Windows.h"
#include <functional>
#include <unordered_set>

std::vector<std::function<void()>> up_callbacks;
std::vector<std::function<void()>> down_callbacks;

bool awaitable_keyup::await_ready() { return false; }
void awaitable_keyup::await_suspend(std::coroutine_handle<> h) {
  size_t idx = up_callbacks.size();
  std::function<void()> callback = [h, idx]() {
    h.resume();
    up_callbacks.erase(up_callbacks.begin() + idx);
  };
  up_callbacks.push_back(callback);
}
void awaitable_keyup::await_resume() {}

bool awaitable_keydown::await_ready() { return false; }
void awaitable_keydown::await_suspend(std::coroutine_handle<> h) {
  size_t idx = down_callbacks.size();
  std::function<void()> callback = [h, idx]() {
    h.resume();
    down_callbacks.erase(down_callbacks.begin() + idx);
  };
  down_callbacks.push_back(callback);
}
void awaitable_keydown::await_resume() {}

awaitable_keyup wait_for_keyup() { return awaitable_keyup{}; }
awaitable_keydown wait_for_keydown() { return awaitable_keydown{}; }

LRESULT key_handler(int code, WPARAM wParam, LPARAM lParam) {
  static bool first_down = true;
  PKBDLLHOOKSTRUCT key = (PKBDLLHOOKSTRUCT)lParam;
  if (key->vkCode == VK_F1 && code == HC_ACTION) {
    if (wParam == WM_KEYDOWN)
      for (auto callback : down_callbacks)
        callback();
    else if (wParam == WM_KEYUP)
      for (auto &callback : up_callbacks)
        callback();
    return 1;
  }

  return CallNextHookEx(NULL, code, wParam, lParam);
}

HHOOK hook;
void init_key_generator() {
  hook = SetWindowsHookEx(WH_KEYBOARD_LL, key_handler, NULL, 0);
}

void run_key_generator() {
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
  }
}