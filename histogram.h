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

#include <atomic>
#include <array>
#include <numeric>
#include <string>

constexpr int max_bar_length = 30;

template <int NBins> class histogram_base
{
  const std::array<std::array<char, 16>, NBins> names;

protected:
  std::array<std::atomic<int>, NBins> bins = {};

  template <int... Indexes>
  histogram_base(float max, float &min, std::integer_sequence<int, Indexes...>)
      : names{[](float &min, int) {
        std::stringstream s;
        std::array<char, 16> r;
        s << std::setw(16) << std::left << std::hexfloat << min;
        min = std::nextafter(min, 0.f);
        s.str().copy(r.data(), 16);
        return r;
      }(min, Indexes)...}
  {
  }

  template <int... Indexes>
  std::array<int, NBins> copy_bins(std::integer_sequence<int, Indexes...>) const
  {
    return {{bins[Indexes]...}};
  }

public:
  std::string printable() const
  {
    const std::array<int, NBins> local_bins =
        copy_bins(std::make_integer_sequence<int, NBins>());
    const int max = std::accumulate(local_bins.begin(), local_bins.end(), 1, std::max<int>);
    auto name_it = names.begin();
    std::string s;
    constexpr int glyphsize = sizeof("\u2588") - 1;
    s.reserve(NBins * (3 + 16 + 1 + glyphsize * max_bar_length + 3 + 8 + 7));
    for (int n : local_bins) {
      s.append("\x1b[s", 3);
      s.append((name_it++)->data(), 16);
      s += '[';

      int bar_len = n * max_bar_length * 8 / max;
      for (int k = 0; k < bar_len / 8; ++k) {
        s.append("\u2588", glyphsize);
      }
      if (bar_len % 8) {
        char partial_block[glyphsize + 1] = "\u2588";
        partial_block[glyphsize - 1] +=
            8 - bar_len % 8;  // adjust last block to x/8 of its width
        s.append(partial_block, glyphsize);
      }
      s.append(max_bar_length - (bar_len + 7) / 8, ' ');
      s.append("] (", 3).append(std::to_string(n)).append(")\x1b[u\x1b[B", 7);
    }
    return s;
  }
};

template <int NBins> class histogram : public histogram_base<NBins>
{
  const float min_value, max_value, epsilon;

public:
  histogram(float max, float min = 0.f)
      : histogram_base<NBins>(max, (min = max, min),
                              std::make_integer_sequence<int, NBins>())
      , min_value(std::nextafter(min, max))
      , max_value(max)
      , epsilon(max - std::nextafter(max, 0.f))
  {
  }

  bool insert(float v)
  {
    if (v >= min_value && v <= max_value) {
      int rr, x;
      std::memcpy(&rr, &v, sizeof(rr));
      std::memcpy(&x, &min_value, sizeof(rr));
      int idx = rr - x;
      if (idx < 0 || idx >= NBins) {
        std::cerr << std::hex << idx << " from " << v << " -> " << std::hex << rr
                  << " min: " << min_value << " max: " << max_value << " eps: " << epsilon << std::endl;
        std::terminate();
      }
      this->bins[NBins - idx - 1].fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    return false;
  }
};
