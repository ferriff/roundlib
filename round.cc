#include <fmt/base.h>
#include <vector>

#include "roundlib.hpp"


int is_digit(char c)
{
        return '0' <= c && c <= '9';
}


int is_number(const char* s)
{
        size_t sz = strlen(s);
        if (sz < 1) return 0;
        if ('0' <= s[0] && s[0] <= '9') return 1;
        else if ((s[0] == '-' || s[0] == '+')  && is_digit(s[1]))  return 1;
        else if ((s[1] == '.' && sz > 2        && is_digit(s[2]))) return 1;
        return 0;
}


std::vector<std::string_view> parse_list(std::string_view txt)
{
        std::vector<std::string_view> out;
        while (!txt.empty()) {
                size_t pos = txt.find(',');
                // take the piece before the comma (or the whole remainder)
                std::string_view token = txt.substr(0, pos);

                const auto not_space = [](char ch) {
                        return !std::isspace(static_cast<unsigned char>(ch));
                };
                token.remove_prefix(std::distance(token.begin(), std::find_if(token.begin(), token.end(), not_space)));
                token.remove_suffix(std::distance(token.rbegin(), std::find_if(token.rbegin(), token.rend(), not_space)));

                if (!token.empty()) out.emplace_back(token);

                txt.remove_prefix(pos == std::string_view::npos ? txt.size() : pos + 1);
        }
        return out;
}


int main(int argc, char** argv)
{
        int from_stdin = 0;
        rounder::format_options opts;
        std::string_view val;
        std::vector<std::string_view> errors;
        std::vector<std::string_view> labels;
        for (int i = 1; i < argc; ++i) {
                const char* c = argv[i];
                if (is_number(c) && !from_stdin) {
                        if (val.empty()) val = c;
                        else errors.emplace_back(c);
                        continue;
                }
                switch (c[1]) {
                        case 'h': // usage
                                // usage(argv[0]);
                                return 0;
                                break;
                        case '\0': // read from standard input
                                from_stdin = 1;
                                break;
                        case 'c': // combined: 2 digits, rounded to the largest uncertainties
                                opts.algo = rounder::format_options::round_algo::twodigits;
                                opts.prec_to_total_err = true;
                                break;
                        case 'e': // round to the total error (quadrature sum of the others, assuming them uncorrelated)
                                opts.prec_to_total_err = true;
                                break;
                        case 'p': // PDG rounding
                                opts.algo = rounder::format_options::round_algo::pdg;
                                break;
                        case 's': // symmetrize errors when within +/-10%
                                opts.symmetrize_errors = true;
                                break;
                        case 't': // round to two significant digits
                                opts.algo = rounder::format_options::round_algo::twodigits;
                                break;
                        // case 'v': // verbose
                        //         _o |= kVerbose;
                        //         break;
                        case 'w': // round to the larger error
                                opts.prec_to_larger_err = true;
                                break;
                        case 'D': // multiply with cdot instead of times
                                opts.cdot = true;
                                break;
                        case 'F': // factorize powers
                                opts.factorize_powers = true;
                                break;
                        case 'G': // gnuplot
                                opts.mode = rounder::mode_type::gnuplot;
                                break;
                        case 'L': // comma-separated list of labels to display after the corresponding errors
                                labels = parse_list(argv[++i]);
                                opts.labels = &labels;
                                // opts.labels = std::move(parse_list(argv[++i]));
                                break;
                        case 'N': // trailing new line
                                opts.trailing_newline = false;
                                break;
                        case 'T': // typst
                                opts.mode = rounder::mode_type::typst;
                                break;
                        case 'U': // no utf8 characters (when relevant)
                                opts.no_utf8 = true;
                                break;
                        case 'X': // (La)TeX
                                opts.mode = rounder::mode_type::tex;
                                break;
                        default:
                                fmt::println("# warning: option {} not recognized", argv[i]);
                                break;
                }
        }
        fmt::print("{}", rounder::format(val, errors, opts));
        return 0;
}
