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
#include <numeric>
#include <random>
#include <string>

// configuration
constexpr int hist_size = 30;
constexpr int bar_length = 100;
constexpr int lo_res = 1 << 4;  // bin resolution at the low end
constexpr int round_mode = FE_DOWNWARD;  // DOWNWARD: no problem, NEAREST: half a problem,
                                         // UPWARD: full problem ;-)
using RNE = std::default_random_engine;  // std::mt19937;

void print_histogram(const std::array<int, hist_size> &hist,
                     const std::array<float, hist_size> &names)
{
  const int max = std::accumulate(hist.begin(), hist.end(), 1, std::max<int>);
  auto name_it = names.begin();
  for (int n : hist) {
    int bar_len = n * bar_length / max;
    std::cout << std::setw(16) << std::hexfloat << *name_it++ << ": ["
              << std::setw(bar_length) << std::left << std::string(bar_len, '#') << "] ("
              << n << ")\n";
  }
}

int hi_bin_for(float r)
{
  if (r < .5f) {
    return -1;
  }
  int rr;
  std::memcpy(&rr, &r, sizeof(rr));
  rr &= 0x00ffffff;
  return rr - (0x00800000 - hist_size + 1);
}

int lo_bin_for(float r)
{
  int n = std::floor(r / (std::numeric_limits<float>::epsilon() / 2 / lo_res));
  if (n >= hist_size) {
    return -1;
  }
  return n;
}

int main()
{
  std::fesetround(round_mode);
  std::array<float, hist_size> lo_names, hi_names;
  lo_names[0] = 0.f;
  for (int i = 1; i < hist_size; ++i) {
    lo_names[i] = lo_names[i - 1] + (std::numeric_limits<float>::epsilon() / 2 / lo_res);
  }
  hi_names[hist_size - 1] = 1.f;
  for (int i = hist_size - 2; i >= 0; --i) {
    hi_names[i] = std::nextafter(hi_names[i + 1], 0.f);
  }

  std::random_device rd;
  RNE e(rd());
  std::array<int, hist_size> lo_histogram = {}, hi_histogram = {};

  auto &&refresh_screen = [&]() {
    static int n = 0;
    if (++n == 10) {
      n = 1;
      std::cout << "\x1b[2J\x1b[H";
      print_histogram(lo_histogram, lo_names);
      std::cout << '\n';
      print_histogram(hi_histogram, hi_names);
      std::cout << std::flush;
    }
  };
  for (;;) {
    const float r = std::generate_canonical<float, std::numeric_limits<float>::digits>(e);
    const int lo = lo_bin_for(r);
    if (lo >= 0) {
      ++lo_histogram[lo];
      refresh_screen();
    } else {
      const int hi = hi_bin_for(r);
      if (hi >= 0) {
        ++hi_histogram[hi];
        refresh_screen();
      }
    }
  }
  return 0;
}
