/*{{{
    Copyright © 2018 Matthias Kretz <kretz@kde.org>

    Permission to use, copy, modify, and distribute this software
    and its documentation for any purpose and without fee is hereby
    granted, provided that the above copyright notice appear in all
    copies and that both that the copyright notice and this
    permission notice and warranty disclaimer appear in supporting
    documentation, and that the name of the author not be used in
    advertising or publicity pertaining to distribution of the
    software without specific, written prior permission.

    The author disclaim all warranties with regard to this
    software, including all implied warranties of merchantability
    and fitness.  In no event shall the author be liable for any
    special, indirect or consequential damages or any damages
    whatsoever resulting from loss of use, data or profits, whether
    in an action of contract, negligence or other tortious action,
    arising out of or in connection with the use or performance of
    this software.

}}}*/

#include <array>
#include <cfenv>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <string>
#include <sstream>

#include "histogram.h"

// configuration
constexpr int hist_size = 40;
constexpr int lo_res = 1 << 8;  // bin resolution at the low end
constexpr int round_mode =
    FE_TONEAREST;  // DOWNWARD: no problem, TONEAREST: half a problem,
                   // UPWARD: full problem ;-)
using RNE = std::mt19937;  // std::mt19937;

constexpr unsigned long long popcnt(unsigned long long n)
{
    n = (n & 0x5555555555555555ULL) + ((n >> 1) & 0x5555555555555555ULL);
    n = (n & 0x3333333333333333ULL) + ((n >> 2) & 0x3333333333333333ULL);
    n = (n & 0x0f0f0f0f0f0f0f0fULL) + ((n >> 4) & 0x0f0f0f0f0f0f0f0fULL);
    n = (n & 0x00ff00ff00ff00ffULL) + ((n >> 8) & 0x00ff00ff00ff00ffULL);
    n = (n & 0x0000ffff0000ffffULL) + ((n >>16) & 0x0000ffff0000ffffULL);
    n = (n & 0x00000000ffffffffULL) + ((n >>32) & 0x00000000ffffffffULL);
    return n;
}

template <class T, std::size_t Bits, class G> float my_canonical(G &engine)
{
  // work with 32 bits for simplicity
  static_assert(G::min() == 0 && G::max() == 0xffffffffu, "");
  static_assert(std::is_same<T, float>::value, "");
  //constexpr std::size_t engine_bits = popcnt(G::max());
  unsigned bits = engine();
  if (bits == 0) {
    return 0.f;
  }
  const unsigned exponent_adj = __builtin_clz(bits);
  const unsigned exponent = 0x3f000000 - exponent_adj * 0x00800000;
  if (exponent_adj > 32 - std::numeric_limits<T>::digits + 1) {
      // need new randomness for the mantissa
      bits = engine();
  }
  constexpr unsigned mantissa_mask = (1u << (std::numeric_limits<T>::digits - 1)) - 1;
  const unsigned mantissa = bits & mantissa_mask;
  const unsigned value = exponent | mantissa;
  T result;
  std::memcpy(&result, &value, sizeof(T));
    asm volatile("");
  return result;
}

int main()
{
  std::fesetround(round_mode);

  histogram<hist_size> hists[] = {
      {0x1.000018p-8f}, {0x1.000018p-7f}, {0x1.000018p-6f}, {0x1.000018p-5f},
      {0x1.000018p-4f}, {0x1.000018p-3f}, {0x1.000018p-2f}, {1.f}};

  std::random_device rd;

  for (int i = 0; i < 8; ++i) {
    new std::thread([&]() {
      RNE e(rd());
      for (;;) {
        const float r = std::generate_canonical<float, std::numeric_limits<float>::digits>(e);
        //const float r = my_canonical<float, std::numeric_limits<float>::digits>(e);
        for (auto &h : hists) {
          h.insert(r);
        }
      }
    });
  }

  for (;;) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(50ms);
    std::cout.write("\x1b[2J\x1b[H", 7);
    int i = 0;
    for (const auto &h: hists) {
      std::cout << h.printable();
      ++i;
      std::cout << "\x1b[" << (i / 4 * (hist_size + 1)) + 1 << ";" << (i % 4) * 60 << 'H';
    }
    std::cout << std::flush;
  }

  return 0;
}
