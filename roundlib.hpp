/* Header‑only library to round and format in publication style
 * the central value of a measurement and its associated uncertainties.
 *
 * Copyright © 2025 Federico Ferri (federico.ferri@cea.fr)
 *
 * SPDX‑License‑Identifier: GPL-3.0-or-later
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// line if you wish to remain header-only also with fmt, compile with `-DFMT_HEADER_ONLY`
#include <fmt/format.h>

namespace rounder {

/*------------------------------------
 * decimal representation of a number
 *------------------------------------*/
struct number {

        std::uint64_t n = 0; // mantissa (always non‑negative)
        int p           = 0; // exponent: value = sgn * n * 10^p
        int sgn         = 0; // -1, +1, 0 (symmetric error)

        constexpr number() noexcept = default;

        template <typename T,
                  typename = std::enable_if_t<
                      std::is_arithmetic_v<T>
                      || std::is_convertible_v<T, std::string_view>>>
        number(const T& v, int sgn = 0)
            : number{from_anything(v, sgn)}
        {}

        // constructor from any numeric type
        // (e.g., int, unsigned long long, float, double, long double, bool, etc.)
        // sng = 1 forces the string to contain a '+' so that the uncertainty is
        // interpreted as asymmetric
        template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
        static number from_numeric(const T v, int sgn = 0)
        {
                char buf[33]{};
                char* ptr = buf;
                if      (sgn > 0) *ptr++ = '+';
                else if (sgn < 0) *ptr++ = '-';
                auto [num_ptr, ec] = std::to_chars(ptr, buf + sizeof(buf), v);

                if (ec != std::errc{}) {
                        fmt::println("# error: cannot convert {}", v);
                        std::exit(1);
                }
                return from_string(std::string_view{buf, static_cast<std::size_t>(num_ptr - buf)});
        }


        // constructor from a string-like object
        static number from_string(std::string_view sv)
        {
                number res{};

                // trim whitespace
                while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
                        sv.remove_prefix(1);
                while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
                        sv.remove_suffix(1);
                if (sv.empty()) {
                        fmt::println("# error: empty number {}", sv);
                        std::exit(1);
                }

                // sign
                if (sv.front() == '+') {
                        res.sgn = +1;
                        sv.remove_prefix(1);
                } else if (sv.front() == '-') {
                        res.sgn = -1;
                        sv.remove_prefix(1);
                } else {
                        res.sgn = 0;
                }

                // scan digits
                std::size_t dot    = std::string_view::npos;
                std::size_t digits = 0;
                for (std::size_t i = 0; i < sv.size(); ++i) {
                        char c = sv[i];
                        if (c == '.') {
                                if (dot != std::string_view::npos) {
                                        fmt::println("# error: multiple " "decimal points in {}", sv);
                                        std::exit(1);
                                }
                                dot = i;
                        } else if (std::isdigit(static_cast<unsigned char>(c))) {
                                ++digits;
                        } else {
                                fmt::println("# error: invalid character in {}", sv);
                                std::exit(1);
                        }
                }
                if (digits == 0) {
                        fmt::println("# error: no digits in {}", sv);
                        std::exit(1);
                }

                // mantissa
                std::uint64_t mant = 0;
                // for (std::size_t i = 0; i < end; ++i) {
                for (std::size_t i = 0; i < sv.size(); ++i) {
                        char c = sv[i];
                        if (c == '.') continue;
                        int d = c - '0';
                        if (mant > (std::numeric_limits<std::uint64_t>::max() - d) / 10) {
                                fmt::println("# error: mantissa overflow for {}", sv);
                                std::exit(1);
                        }
                        mant = mant * 10 + static_cast<std::uint64_t>(d);
                }
                res.n = mant;

                // exponent
                if (dot == std::string_view::npos) {
                        res.p = 0;
                } else {
                        int after = static_cast<int>(sv.size() - dot - 1);
                        res.p     = -after;
                }
                return res;
        }


        // construct from anything: combine the previous two
        template<typename T>
        static number from_anything(const T& v, int sgn = 0)
        {
                if constexpr (std::is_arithmetic_v<T>) {
                        return number::from_numeric(v, sgn);
                } else if constexpr (std::is_convertible_v<T, std::string_view>) {
                        return number::from_string(static_cast<std::string_view>(v));
                } else {
                        static_assert(
                            std::is_arithmetic_v<T> || std::is_convertible_v<T, std::string_view>,
                            "type must be numeric or convertible to "
                            "std::string_view.");
                        // unreachable
                        return {};
                }
        }


        // print internal representation
        void print() const noexcept
        {
                fmt::println("# n={} p10={} sgn={} - {}", n, p, sgn, to_string());
        }


        // number to decimal string
        std::string to_string(bool factorize_powers = false) const
        {
                char mant[21]{};
                auto len = std::to_chars(mant, mant + sizeof(mant), n).ptr - mant;

                std::string out;
                out.reserve(len + std::abs(p) + 2); // sign + dot + possible leading zeros

                if (sgn < 0) out.push_back('-');

                if (p >= 0 || factorize_powers) {
                        out.append(mant, len);
                        if (!factorize_powers) out.append(static_cast<std::size_t>(p), '0');
                } else {
                        int shift = -p;
                        if (static_cast<std::size_t>(shift) >= static_cast<std::size_t>(len)) {
                                // 0.xxx… case
                                out.push_back('0');
                                out.push_back('.');
                                out.append(static_cast<std::size_t>(shift) - len, '0');
                                out.append(mant, len);
                        } else {
                                // insert dot inside mantissa
                                std::size_t int_len = len - shift;
                                out.append(mant, int_len);
                                out.push_back('.');
                                out.append(mant + int_len, shift);
                        }
                }
                return out;
        }


        // convert back to double – mostly useless
        double to_double() const
        {
                int s = sgn;
                if (s == 0) s = 1; // if sgn is zero, the error is symmetric, the number is "absolute", i.e. positive
                return static_cast<double>(s) * static_cast<double>(n) * std::pow(10.0, p);
        }
};


/*------------------
 * helpers to round
 *------------------*/
namespace detail {

// count decimal digits of a non‑negative integer
inline int digit_count(std::uint64_t v)
{
        if (v == 0) return 1;
        int c = 0;
        while (v) {
                v /= 10;
                ++c;
        }
        return c;
}


inline number quadrature_sum(const std::vector<number> &vec)
{
        if (vec.size() == 1) return vec[0];
        double sum = 0;
        double c   = 0;
        size_t cnt = 0;
        double dp  = 1.;
        for (const auto &e : vec) {
                double v = e.to_double();
                // blindly symmetrize errors (approximation)
                if (e.sgn != 0) {
                        ++cnt;
                        v *= 0.5;
                        dp *= v;
                }
                double y = v * v - c;
                // correct for the missing double product in the squared average
                // of asymmetric errors
                if (cnt % 2 == 0) y += 0.5 * dp;
                double t = sum + y;
                c        = (t - sum) - y;
                sum      = t;
        }
        if (cnt % 2 != 0) {
                fmt::println("# warning: asymmetric errors do not seem to come in pairs");
                fmt::println("# warning: the total error computation is wrong.");
        }
        return number::from_numeric(std::sqrt(sum));
};


// keep three most significant digits
inline void keep_three_sig(number& n, bool quiet)
{
        int nd = digit_count(n.n);
        if (nd < 3 && !quiet) {
                fmt::println("# warning: not enough significant digits, padding with zeros");
        }
        if (nd == 1) {
                n.n *= 100;
                n.p -= 2;
        } else if (nd == 2) {
                n.n *= 10;
                n.p -= 1;
        } else {
                int drop = nd - 3;
                while (drop--) {
                        n.n /= 10;
                        ++n.p;
                }
        }
}


// PDG rule for a three‑digit mantissa
inline void pdg_rule(number& n)
{
        int cnt = digit_count(n.n);
        if (cnt != 3) {
                fmt::println("# error: number {} does not have 3 digits", n.n);
                std::exit(1);
        }
        if (100 <= n.n && n.n <= 354) {
                int u = n.n % 10;
                n.n /= 10;
                if (u >= 5) ++n.n;
                ++n.p;
                return;
        }
        if (355 <= n.n && n.n <= 949) {
                int d = (n.n / 10) % 10;
                n.n /= 100;
                n.p += 2;
                if (d >= 5) ++n.n;
                return;
        }
        // > 950 → round to 1000, keep two sig‑digits
        n.n = 10;
        n.p += 2;
        return;
}


// keep three significant digits then apply the PDG rule
inline void pdg_round(number& n, bool quiet)
{
        keep_three_sig(n, quiet);
        pdg_rule(n);
}


// keep three significant digits then apply the PDG rule
inline void twodig_round(number& n, bool quiet)
{
        keep_three_sig(n, quiet);
        int u = n.n % 10;
        n.n /= 10;
        if (u >= 5) ++n.n;
        ++n.p;
}


// round to a given precision
inline void round_to_prec(number& n, int prec)
{
        if (n.p > prec) {
                fmt::println("# error: cannot round {} to precision {}", n.to_string(false), prec);
                std::exit(1);
        }
        int d = 0;
        while (n.p < prec) {
                d = n.n % 10;
                n.n /= 10;
                ++n.p;
        }
        if (d >= 5) ++n.n;
}


/*-----------------------------------------------
 * helpers to check if a template is a container
 *-----------------------------------------------*/
// string-like template (to be excluded)
template<class T>
struct is_std_string_like : std::false_type {};

// std::basic_string specialisations (std::string, std::wstring, …)
template<class C, class T, class A>
struct is_std_string_like<std::basic_string<C, T, A>> : std::true_type {};

// std::basic_string_view specialisations (std::string_view, std::wstring_view,…)
template<class C, class T>
struct is_std_string_like<std::basic_string_view<C, T>> : std::true_type {};

// container-like template – defaults to false
template <class, class = void>
struct is_container : std::false_type {};

// specialisation – true if we can call std::begin/std::end
// and the type exposes a nested value_type, unless it is string-like
template <class T>
struct is_container<T, std::void_t<
                decltype(std::begin(std::declval<T &>())),
                decltype(std::end(std::declval<T &>())),
                typename T::value_type,
                std::enable_if_t<!is_std_string_like<std::remove_cv_t<T>>::value> // exclude string-like
                >> : std::true_type {};

template<class T>
constexpr bool is_container_v = is_container<T>::value;

} // namespace detail


/*----------------------
 * formatting utilities
 *----------------------*/
enum class mode_type { terminal, tex, typst, gnuplot };

struct format_options {
        mode_type mode;
        enum class round_algo {pdg, twodigits};
        round_algo algo;
        const std::vector<std::string_view>* labels;

        // bit-like fields, defaults in the constructor below
        unsigned symmetrize_errors  : 1;
        unsigned prec_to_total_err  : 1;
        unsigned prec_to_larger_err : 1;
        unsigned factorize_powers   : 1;
        unsigned no_utf8            : 1;
        unsigned cdot               : 1;
        unsigned _reserved          : 2; // pad to 1 byte

        constexpr format_options()
        : mode(mode_type::terminal),
          algo(round_algo::pdg),
          labels(),
          symmetrize_errors(0),
          prec_to_total_err(0),
          prec_to_larger_err(1),
          factorize_powers(0),
          no_utf8(0),
          cdot(0),
          _reserved(0) {}
};


class formatter {
      public:
        explicit formatter(const format_options& opt) : opt_(opt) {}

        // produce the final string for a central value and a list of errors
        std::string format(const number& central,
                           const std::vector<number>& errors) const
        {
                std::string out;
                out.reserve(128); // single allocation, should work for most cases

                // leading parenthesis for factorised power (if any)
                if (opt_.factorize_powers && central.p != 0) out += symbols().po;

                // central value
                out += central.to_string(opt_.factorize_powers);

                // errors
                size_t cnt_label = 0;
                for (const auto& e : errors) {
                        out += ' ';
                        if (e.sgn != 0 && (opt_.mode == mode_type::gnuplot || opt_.mode == mode_type::tex || opt_.mode == mode_type::typst)) {
                                if (cnt_label % 2 == 0) out += symbols().cs; // insert a small space before super/subscripts
                                out += e.sgn == 1 ? '^' : '_'; // sgn discriminates upper/lower errors
                                out += symbols().co;
                        }
                        if (e.sgn == 0) { // the “±” token
                                out += symbols().pm;
                                out += ' ';
                                ++cnt_label; // to increment by 2 for symmetric errors
                        } else if (e.sgn == 1) {
                                out += '+'; // explicit + for errors
                        }
                        out += e.to_string(opt_.factorize_powers);
                        if (e.sgn != 0 && (opt_.mode == mode_type::gnuplot || opt_.mode == mode_type::tex || opt_.mode == mode_type::typst)) {
                                out += symbols().cc;
                        }
                        ++cnt_label;
                        // if provided, add labels
                        if (opt_.labels && opt_.labels->size() && cnt_label % 2 == 0) {
                                out += ' ';
                                out += symbols().to;
                                out += (*opt_.labels)[cnt_label / 2 - 1];
                                out += symbols().tc;
                        }
                }

                // trailing factorised power (if any)
                if (opt_.factorize_powers && central.p != 0) {
                        out += symbols().pc;
                        if (opt_.cdot) out += symbols().xa;
                        else           out += symbols().x;
                        out += "10";
                        if (central.p != 1) {
                                out += '^';
                                out += symbols().co;
                                out += std::to_string(central.p);
                                out += symbols().cc;
                        }
                }
                return out;
        }

      private:
        const format_options& opt_;

        // symbol table
        struct symbol_table {
                // mult, mult alternate, plus/minus, parenthesis open/close,
                // curly open/close, curly pre-space, text open/close
                std::string_view x, xa, pm, po, pc, co, cc, cs, to, tc;
        };
        static constexpr std::array<symbol_table, 4> table_{{
            {"×",         "·",        "±",            "(",        ")",         "",  "",  "",          "",        ""},   // terminal
            {" \\times ", "\\cdot",   "\\pm",         "\\left( ", " \\right)", "{", "}", "\\,",       "\\text{", "}"},  // TeX
            {" times ",   " dot.op ", " plus.minus ", "(",        ")",         "(", ")", "#h(0.0em)", "\"",      "\""}, // typst
            {"×",         "· ",       "±",            "(",        ")",         "{", "}", "",          "",        ""}    // gnuplot
        }};


        const symbol_table& symbols() const
        {
                const symbol_table* base = &table_[static_cast<std::size_t>(opt_.mode)];
                if (!opt_.no_utf8) return *base;
                static const symbol_table ascii{"x", ".", "+/-", base->po, base->pc, base->co, base->cc, base->cs, base->to, base->tc};
                return ascii;
        }
};


namespace detail {

// symmetrize asymmetric errors if they differ by less than threshold % (default to 10%)
inline void symmetrize_errors(std::vector<number>& errors, double threshold = 0.1)
{
        for (int i = static_cast<int>(errors.size()) - 1; i > 0; --i) {
                auto &ne1 = errors[i];
                if (ne1.sgn != 0 && i > 0) --i;
                else continue;
                auto &ne2 = errors[i];
                if (ne2.sgn == 0) {
                        fmt::println("# warning: asymmetric errors do not seem to come in pairs");
                }
                double e1 = fabs(ne1.to_double());
                double e2 = fabs(ne2.to_double());
                if (fabs(e1 / e2 - 1.) < threshold) {
                        ne2 = number::from_numeric(0.5 * (e1 + e2));
                        errors.erase(errors.begin() + i + 1);
                }
        }
}


// perform the rounding
inline void round(number& central, std::vector<number>& errors, const format_options& opt)
{
        if (opt.symmetrize_errors) symmetrize_errors(errors);
        bool quiet = !(opt.mode == mode_type::terminal && opt.factorize_powers);
        int prec   = INT_MAX;

        number tote;
        if (opt.prec_to_total_err) {
                tote = detail::quadrature_sum(errors);
                if (opt.algo == format_options::round_algo::pdg) detail::pdg_round(tote, quiet);
                else detail::twodig_round(tote, quiet);
                prec = tote.p;
        }

        if (opt.prec_to_larger_err) {
                // match the precision of the central value to that of the less precise error
                prec = INT_MIN;
                if (opt.algo == format_options::round_algo::pdg) {
                        for (auto &e : errors)
                                detail::pdg_round(e, quiet);
                } else {
                        for (auto &e : errors)
                                detail::twodig_round(e, quiet);
                }
                for (auto &e : errors) prec = std::max(prec, e.p);
        }

        // round everything else to match the precision
        if (prec != INT_MAX) {
                detail::round_to_prec(central, prec);
                for (auto &e : errors) detail::round_to_prec(e, prec);
        } else if (opt.algo == format_options::round_algo::pdg) {
                // or round independently to the chosen algorithms
                detail::pdg_round(central, quiet);
                for (auto &e : errors) detail::pdg_round(e, quiet);
        } else {
                // or round independently to the chosen algorithms
                detail::twodig_round(central, quiet);
                for (auto &e : errors) detail::twodig_round(e, quiet);
        }
}
} // namespace detail
} // namespace rounder


/*-----
 * API
 *-----*/
namespace rounder {

// value + multiple errors (signed +/- for upper/lower, unsigned for symmetric)
inline std::string format_numbers(number value, std::vector<number>& errors,
                          const format_options& opt = {})
{
        detail::round(value, errors, opt);
        formatter fmt(opt);
        return fmt.format(value, errors);
}


// format with rounding the following inputs:
// - value, single error
// - value, container of errors
// where value and error(s) can be anything convertible to arithmetic types or to string_view 
template<typename V, typename E>
inline std::string format(const V& val, const E& err,
                          const format_options &opt = {})
{
        number v = number::from_anything(val);
        std::vector<number> e;
        if constexpr (detail::is_container_v<E>) {
                e.reserve(err.size());
                for (const auto& el : err) e.emplace_back(number::from_anything(el));
        } else {
                e.reserve(1);
                e.emplace_back(number::from_anything(err));
        }
        return format_numbers(v, e, opt);
}
} // namespace rounder


// fmt::formatter specialization for `measurement`.
// The format‑specifier is a sequence of single characters that map directly to the fields
// of `rounder::format_options` (see the table in the prompt). Unrecognised chars are ignored.
//
// Example usage:
//     fmt::print("{:csF}", meas);   // combined‑style, symmetrize, factorise powers
//
namespace rounder {
struct measurement {
        rounder::number central;
        std::vector<rounder::number> errors;
        std::vector<std::string_view> labels;
};
} // namespace rounder

#include <fmt/core.h>
#include <string_view>

template <>
struct fmt::formatter<rounder::measurement> {
        mutable rounder::format_options opts_{};

        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
                auto i  = ctx.begin();
                auto end = ctx.end();

                opts_.algo = rounder::format_options::round_algo::twodigits;
                opts_.prec_to_total_err = true;
                while (i != end && *i != '}') {
                        switch (*i) {
                        case 'c': // 2‑digit precision, rounded to total (quadrature) error
                                opts_.algo = rounder::format_options::round_algo::twodigits;
                                opts_.prec_to_total_err = true;
                                break;
                        case 'e': // round to the total (quadrature) error
                                opts_.prec_to_total_err = true;
                                break;
                        case 'l': // round to the larger error
                                opts_.prec_to_larger_err = true;
                                break;
                        case 'p': // PDG rounding
                                opts_.algo = rounder::format_options::round_algo::pdg;
                                break;
                        case 's': // symmetrise errors when within ±10 %
                                opts_.symmetrize_errors = true;
                                break;
                        case 't': // round to two significant digits
                                opts_.algo = rounder::format_options::round_algo::twodigits;
                                break;
                        case 'D': // use “·” (cdot) instead of “×”
                                opts_.cdot = true;
                                break;
                        case 'F': // factorise powers
                                opts_.factorize_powers = true;
                                break;
                        case 'G': // gnuplot mode
                                opts_.mode = rounder::mode_type::gnuplot;
                                break;
                        case 'L': // label list – not supported in the single‑char API;
                                break;
                        case 'N': // suppress trailing newline
                                // always false - delegate newline to fmt
                                break;
                        case 'T': // typst mode
                                opts_.mode = rounder::mode_type::typst;
                                break;
                        case 'U': // no UTF‑8 characters
                                opts_.no_utf8 = true;
                                break;
                        case 'X': // LaTeX mode
                                opts_.mode = rounder::mode_type::tex;
                                break;
                        default:
                                // unknown flag – ignore
                                break;
                        }
                        ++i;
                }
                if (i != end && *i != '}') {
                        throw format_error("invalid format");
                }
                return i;
        }

        template<typename fmt_context>
        auto format(const rounder::measurement& m, fmt_context& ctx) const -> decltype(ctx.out())
        {
                if (m.labels.size()) opts_.labels = &m.labels;
                rounder::measurement mm(m);
                std::string txt = rounder::format_numbers(m.central, mm.errors, opts_);
                return fmt::format_to(ctx.out(), "{}", txt);
        }
};
