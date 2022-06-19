#include "key_generator.h"
#include <chrono>
#include <coroutine>
#include <iostream>
#include <tuple>
#include <unordered_map>

struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

using secdur = std::chrono::duration<double, std::ratio<1, 1>>;
using timept =
    std::chrono::time_point<std::chrono::high_resolution_clock, secdur>;
auto nulltime = timept(secdur(0));

auto closest(auto matching, auto first, auto second) {
  if (std::abs(matching - first) < std::abs(matching - second))
    return first;
  else
    return second;
}

std::unordered_map<std::string, char> morse_map = {
    {".-", 'a'},      {"-...", 'b'},   {"-.-.", 'c'},   {"-..", 'd'},
    {".", 'e'},       {"..-.", 'f'},   {"--.", 'g'},    {"....", 'h'},
    {"..", 'i'},      {".---", 'j'},   {"-.-", 'k'},    {".-..", 'l'},
    {"--", 'm'},      {"-.", 'n'},     {"---", 'o'},    {".--.", 'p'},
    {"--.-", 'q'},    {".-.", 'r'},    {"...", 's'},    {"-", 't'},
    {"..-", 'u'},     {"...-", 'v'},   {".--", 'w'},    {"-..-", 'x'},
    {"-.--", 'y'},    {"--..", 'z'},   {"-----", '0'},  {".----", '1'},
    {"..---", '2'},   {"...--", '3'},  {"....-", '4'},  {".....", '5'},
    {"-....", '6'},   {"--...", '7'},  {"---..", '8'},  {"----.", '9'},
    {".-.-.-", '.'},  {"--..--", ','}, {"..--..", '?'}, {".----.", '\''},
    {"-.-.--", '!'},  {"-..-.", '/'},  {"-.--.", '('},  {"-.--.-", ')'},
    {".-...", '&'},   {"---...", ':'}, {"-.-.-.", ';'}, {"-...-", '='},
    {".-.-.", '+'},   {"-....-", '-'}, {"..--.-", '_'}, {".-..-.", '"'},
    {"...-..-", '$'}, {".--.-.", '@'},
};

task start_live_morse() {
  double short_press_length;
  double long_press_length;
  double letter_pause_length; // time between letters
  double word_pause_length;   // time between words
  std::string init_morse = "";
  std::string curr_morsechar = "";
  std::string decoded = "";
  timept up = nulltime;
  {
    // first phase, keep going until we find a longer/shorter press, and a
    // longer/shorter pause, and configure estimated lengths
    bool got_pause_lens = false;
    bool got_press_lens = false;

    double total_pause_length = 0;
    double total_press_length = 0;
    int count = 0;
    std::vector<std::pair<double, double>> morse_times;
    while (!(got_pause_lens && got_press_lens)) {
      co_await wait_for_keydown();
      timept down = std::chrono::high_resolution_clock::now();

      double pause_length = secdur(down - up).count();
      if (!got_pause_lens && up != nulltime) {
        if (count > 1) {
          // there's always one less pause than presses
          double avg_pause_length = total_pause_length / (count - 1);
          if (pause_length > avg_pause_length * 1.75) {
            // longer pause, so avg is short pause
            word_pause_length = pause_length;
            letter_pause_length = avg_pause_length;
            got_pause_lens = true;
          } else if (pause_length < avg_pause_length / 1.75) {
            // shorter pause, so avg is long pause
            letter_pause_length = pause_length;
            word_pause_length = avg_pause_length;
            got_pause_lens = true;
          }
        }
        total_pause_length += pause_length;
      }

      co_await wait_for_keyup();
      up = std::chrono::high_resolution_clock::now();
      double press_length = secdur(up - down).count();

      if (!got_press_lens) {
        if (count > 1) {
          double avg_press_length = total_press_length / count;
          if (press_length > avg_press_length * 1.75) {
            // longer press, so avg is short press
            long_press_length = press_length;
            short_press_length = avg_press_length;
            got_press_lens = true;
          } else if (press_length < avg_press_length / 1.75) {
            // shorter press, so avg is long press
            short_press_length = press_length;
            long_press_length = avg_press_length;
            got_press_lens = true;
          }
        }
        total_press_length += press_length;
      }

      morse_times.push_back({pause_length, press_length});
      count++;
    }

    for (auto &[pause_length, press_length] : morse_times) {
      auto pause =
          closest(pause_length, letter_pause_length, word_pause_length);
      auto press = closest(press_length, short_press_length, long_press_length);
      if (pause == word_pause_length) {
        init_morse += " ";
        decoded += " ";
        curr_morsechar = "";
      } else
        decoded += morse_map[curr_morsechar];
      if (press == short_press_length)
        curr_morsechar += ".";
      else
        curr_morsechar += "-";
    }
  }
  std::cout << "short_press_length: " << short_press_length << " seconds\n";
  std::cout << "long_press_length: " << long_press_length << " seconds\n";
  std::cout << "letter_pause_length: " << letter_pause_length << " seconds\n";
  std::cout << "word_pause_length: " << word_pause_length << " seconds\n";
  std::cout << "init_morse: " << init_morse << std::flush;
  std::cout << "decoded: " << decoded << std::flush;

  while (true) {
    co_await wait_for_keydown();
    timept down = std::chrono::high_resolution_clock::now();

    double pause_length = secdur(down - up).count();
    auto pause = closest(pause_length, letter_pause_length, word_pause_length);
    if (pause == word_pause_length) {
      word_pause_length = word_pause_length * 0.5 + pause_length * 0.5;
      char c = morse_map[curr_morsechar];
      if (c == '\0')
        std::cout << "Unknown morse code: " << curr_morsechar << std::endl;
      else
        std::cout << c;
      curr_morsechar = "";
    } else
      letter_pause_length = letter_pause_length * 0.5 + pause_length * 0.5;

    co_await wait_for_keyup();
    up = std::chrono::high_resolution_clock::now();
    double press_length = secdur(up - down).count();
    auto press = closest(press_length, short_press_length, long_press_length);
    if (press == short_press_length) {
      short_press_length = short_press_length * 0.5 + press_length * 0.5;
      curr_morsechar += ".";
    } else {
      long_press_length = long_press_length * 0.5 + press_length * 0.5;
      curr_morsechar += "-";
    }
  }
  std::cout << std::endl;
  exit(0);
}
int main() {
  init_key_generator();
  start_live_morse();
  run_key_generator();
  return 0;
}
