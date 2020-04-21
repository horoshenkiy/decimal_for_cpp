/////////////////////////////////////////////////////////////////////////////
// Name:        fixed.h
// Purpose:     Decimal data type support, for COBOL-like fixed-point
//              operations on currency values.
// Author:      Ilya Khoroshenkiy
// Created:     21/04/2020
// Last change: 21/04/2020
// Version:     1.16
// Licence:     BSD
/////////////////////////////////////////////////////////////////////////////

#ifndef _DECIMAL_UTILS_H
#define _DECIMAL_UTILS_H

// ----------------------------------------------------------------------------
// Config section
// ----------------------------------------------------------------------------
// - define DEC_EXTERNAL_INT64 if you do not want internal definition of "int64" data type
//   in this case define "DEC_INT64" somewhere
// - define DEC_EXTERNAL_ROUND if you do not want internal "round()" function
// - define DEC_CROSS_DOUBLE if you want to use double (instead of xdouble) for cross-conversions
// - define DEC_EXTERNAL_LIMITS to define by yourself DEC_MAX_INT32
// - define DEC_NO_CPP11 if your compiler does not support C++11
// - define DEC_TYPE_LEVEL as 0 for strong typing (same precision required for both arguments),
//   as 1 for allowing to mix lower or equal precision types
//   as 2 for automatic rounding when different precision is mixed

#include <iosfwd>
#include <iomanip>
#include <sstream>
#include <locale>

#ifndef DEC_TYPE_LEVEL
#define DEC_TYPE_LEVEL 2
#endif

// --> include headers for limits and int64_t

#ifndef DEC_NO_CPP11
#include <cstdint>
#include <limits>

#else

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X) || (__cplusplus >= 201103L)
#include <cstdint>
#else
#include <stdint.h>
#endif // defined
#endif // DEC_NO_CPP11

// <--

// --> define DEC_MAX_INTxx, DEC_MIN_INTxx if required

#ifndef DEC_EXTERNAL_LIMITS
#ifndef DEC_NO_CPP11
//#define DEC_MAX_INT32 ((std::numeric_limits<int32_t>::max)())
#define DEC_MAX_INT64 ((std::numeric_limits<int64_t>::max)())
#define DEC_MIN_INT64 ((std::numeric_limits<int64_t>::min)())
#else
//#define DEC_MAX_INT32 INT32_MAX
#define DEC_MAX_INT64 INT64_MAX
#define DEC_MIN_INT64 INT64_MIN
#endif // DEC_NO_CPP11
#endif // DEC_EXTERNAL_LIMITS

// <--

namespace dec {

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// --> define DEC_INT64 if required
#ifndef DEC_EXTERNAL_INT64
#ifndef DEC_NO_CPP11
typedef int64_t DEC_INT64;
#else
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef signed __int64 DEC_INT64;
#else
typedef signed long long DEC_INT64;
#endif
#endif
#endif // DEC_EXTERNAL_INT64
// <--

#ifdef DEC_NO_CPP11
#define static_assert(a,b)
#endif

typedef DEC_INT64 int64;
// type for storing currency value internally
typedef int64 dec_storage_t;
typedef unsigned int uint;
// xdouble is an "extended double" - can be long double, __float128, _Quad - as you wish
typedef long double xdouble;

#ifdef DEC_CROSS_DOUBLE
typedef double cross_float;
#else
typedef xdouble cross_float;
#endif

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

enum {
    max_decimal_points = 18
};

// ----------------------------------------------------------------------------
// Round
// ----------------------------------------------------------------------------

#ifndef DEC_EXTERNAL_ROUND

// round floating point value and convert to int64
template<class T>
inline int64 round(T value) {
    T val1;

    if (value < 0.0) {
        val1 = value - 0.5;
    } else {
        val1 = value + 0.5;
    }
    int64 intPart = static_cast<int64>(val1);

    return intPart;
}

// calculate output = round(a / b), where output, a, b are int64
inline bool div_rounded(int64 &output, int64 a, int64 b) {
    int64 divisorCorr = std::abs(b) / 2;
    if (a >= 0) {
        if (DEC_MAX_INT64 - a >= divisorCorr) {
            output = (a + divisorCorr) / b;
            return true;
        }
    } else {
        if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
            output = (a - divisorCorr) / b;
            return true;
        }
    }

    output = 0;
    return false;
}

#endif // DEC_EXTERNAL_ROUND

template<class RoundPolicy>
class dec_utils {
public:

    // result = (value1 * value2) / divisor
    inline static int64 multDiv(const int64 value1, const int64 value2, int64 divisor) {

        // we don't check for division by zero, the caller should - the next line will throw.
        const int64 value1int = value1 / divisor;
        int64 value1dec = value1 % divisor;
        const int64 value2int = value2 / divisor;
        int64 value2dec = value2 % divisor;

        int64 result = value1 * value2int + value1int * value2dec;

        if (value1dec == 0 || value2dec == 0) {
            return result;
        }

        if (!isMultOverflow(value1dec, value2dec)) { // no overflow
            int64 resDecPart = value1dec * value2dec;
            if (!RoundPolicy::div_rounded(resDecPart, resDecPart, divisor))
                resDecPart = 0;
            result += resDecPart;
            return result;
        }

        // minimalize value1 & divisor
        {
            int64 c = gcd(value1dec, divisor);
            if (c != 1) {
                value1dec /= c;
                divisor /= c;
            }

            // minimalize value2 & divisor
            c = gcd(value2dec, divisor);
            if (c != 1) {
                value2dec /= c;
                divisor /= c;
            }
        }

        if (!isMultOverflow(value1dec, value2dec)) { // no overflow
            int64 resDecPart = value1dec * value2dec;
            if (RoundPolicy::div_rounded(resDecPart, resDecPart, divisor)) {
                result += resDecPart;
                return result;
            }
        }

        // overflow can occur - use less precise version
        result += RoundPolicy::round(
                static_cast<cross_float>(value1dec)
                * static_cast<cross_float>(value2dec)
                / static_cast<cross_float>(divisor));
        return result;
    }

    static bool isMultOverflow(const int64 value1, const int64 value2) {
        if (value1 == 0 || value2 == 0) {
            return false;
        }

        if ((value1 < 0) != (value2 < 0)) { // different sign
            if (value1 == DEC_MIN_INT64) {
                return value2 > 1;
            } else if (value2 == DEC_MIN_INT64) {
                return value1 > 1;
            }
            if (value1 < 0) {
                return isMultOverflow(-value1, value2);
            }
            if (value2 < 0) {
                return isMultOverflow(value1, -value2);
            }
        } else if (value1 < 0 && value2 < 0) {
            if (value1 == DEC_MIN_INT64) {
                return value2 < -1;
            } else if (value2 == DEC_MIN_INT64) {
                return value1 < -1;
            }
            return isMultOverflow(-value1, -value2);
        }

        return (value1 > DEC_MAX_INT64 / value2);
    }

    static int64 pow10(int n) {
        static const int64 decimalFactorTable[] = { 1, 10, 100, 1000, 10000,
                                                    100000, 1000000, 10000000, 100000000, 1000000000, 10000000000,
                                                    100000000000, 1000000000000, 10000000000000, 100000000000000,
                                                    1000000000000000, 10000000000000000, 100000000000000000,
                                                    1000000000000000000 };

        if (n >= 0 && n <= max_decimal_points) {
            return decimalFactorTable[n];
        } else {
            return 0;
        }
    }

    template<class T>
    static int64 trunc(T value) {
        return static_cast<int64>(value);
    }

private:
    // calculate greatest common divisor
    static int64 gcd(int64 a, int64 b) {
        int64 c;
        while (a != 0) {
            c = a;
            a = b % a;
            b = c;
        }
        return b;
    }

};

// no-rounding policy (decimal places stripped)
class null_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        return static_cast<int64>(value);
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        output = a / b;
        return true;
    }
};

// default rounding policy - arithmetic, to nearest integer
class def_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        return dec::round(value);
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        return dec::div_rounded(output, a, b);
    }
};

// round half towards negative infinity
class half_down_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        T val1;
        T decimals;

        if (value >= 0.0) {
            decimals = value - floor(value);
            if (decimals > 0.5) {
                val1 = ceil(value);
            } else {
                val1 = value;
            }
        } else {
            decimals = std::abs(value + floor(std::abs(value)));
            if (decimals < 0.5) {
                val1 = ceil(value);
            } else {
                val1 = value;
            }
        }

        return static_cast<int64>(floor(val1));
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 divisorCorr = std::abs(b) / 2;
        int64 remainder = std::abs(a) % std::abs(b);

        if (a >= 0) {
            if (DEC_MAX_INT64 - a >= divisorCorr) {
                if (remainder > divisorCorr) {
                    output = (a + divisorCorr) / b;
                } else {
                    output = a / b;
                }
                return true;
            }
        } else {
            if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
                output = (a - divisorCorr) / b;
                return true;
            }
        }

        output = 0;
        return false;
    }
};

// round half towards positive infinity
class half_up_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        T val1;
        T decimals;

        if (value >= 0.0) {
            decimals = value - floor(value);
            if (decimals >= 0.5) {
                val1 = ceil(value);
            } else {
                val1 = value;
            }
        } else {
            decimals = std::abs(value + floor(std::abs(value)));
            if (decimals <= 0.5) {
                val1 = ceil(value);
            } else {
                val1 = value;
            }
        }

        return static_cast<int64>(floor(val1));
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 divisorCorr = std::abs(b) / 2;
        int64 remainder = std::abs(a) % std::abs(b);

        if (a >= 0) {
            if (DEC_MAX_INT64 - a >= divisorCorr) {
                if (remainder >= divisorCorr) {
                    output = (a + divisorCorr) / b;
                } else {
                    output = a / b;
                }
                return true;
            }
        } else {
            if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
                if (remainder < divisorCorr) {
                    output = (a - remainder) / b;
                } else if (remainder == divisorCorr) {
                    output = (a + divisorCorr) / b;
                } else {
                    output = (a + remainder - std::abs(b)) / b;
                }
                return true;
            }
        }

        output = 0;
        return false;
    }
};

// bankers' rounding
class half_even_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        T val1;
        T decimals;

        if (value >= 0.0) {
            decimals = value - floor(value);
            if (decimals > 0.5) {
                val1 = ceil(value);
            } else if (decimals < 0.5) {
                val1 = floor(value);
            } else {
                bool is_even = (static_cast<int64>(value - decimals) % 2 == 0);
                if (is_even) {
                    val1 = floor(value);
                } else {
                    val1 = ceil(value);
                }
            }
        } else {
            decimals = std::abs(value + floor(std::abs(value)));
            if (decimals > 0.5) {
                val1 = floor(value);
            } else if (decimals < 0.5) {
                val1 = ceil(value);
            } else {
                bool is_even = (static_cast<int64>(value + decimals) % 2 == 0);
                if (is_even) {
                    val1 = ceil(value);
                } else {
                    val1 = floor(value);
                }
            }
        }

        return static_cast<int64>(val1);
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 divisorDiv2 = std::abs(b) / 2;
        int64 remainder = std::abs(a) % std::abs(b);

        if (remainder == 0) {
            output = a / b;
        } else {
            if (a >= 0) {

                if (remainder > divisorDiv2) {
                    output = (a - remainder + std::abs(b)) / b;
                } else if (remainder < divisorDiv2) {
                    output = (a - remainder) / b;
                } else {
                    bool is_even = std::abs(a / b) % 2 == 0;
                    if (is_even) {
                        output = a / b;
                    } else {
                        output = (a - remainder + std::abs(b)) / b;
                    }
                }
            } else {
                // negative value
                if (remainder > divisorDiv2) {
                    output = (a + remainder - std::abs(b)) / b;
                } else if (remainder < divisorDiv2) {
                    output = (a + remainder) / b;
                } else {
                    bool is_even = std::abs(a / b) % 2 == 0;
                    if (is_even) {
                        output = a / b;
                    } else {
                        output = (a + remainder - std::abs(b)) / b;
                    }
                }
            }
        }

        return true;
    }
};

// round towards +infinity
class ceiling_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        return static_cast<int64>(ceil(value));
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 remainder = std::abs(a) % std::abs(b);
        if (remainder == 0) {
            output = a / b;
        } else {
            if (a >= 0) {
                output = (a + std::abs(b)) / b;
            } else {
                output = a / b;
            }
        }
        return true;
    }
};

// round towards -infinity
class floor_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        return static_cast<int64>(floor(value));
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 remainder = std::abs(a) % std::abs(b);
        if (remainder == 0) {
            output = a / b;
        } else {
            if (a >= 0) {
                output = (a - remainder) / b;
            } else {
                output = (a + remainder - std::abs(b)) / b;
            }
        }
        return true;
    }
};

// round towards zero = truncate
class round_down_round_policy: public null_round_policy {
};

// round away from zero
class round_up_round_policy {
public:
    template<class T>
    static int64 round(T value) {
        if (value >= 0.0) {
            return static_cast<int64>(ceil(value));
        } else {
            return static_cast<int64>(floor(value));
        }
    }

    static bool div_rounded(int64 &output, int64 a, int64 b) {
        int64 remainder = std::abs(a) % std::abs(b);
        if (remainder == 0) {
            output = a / b;
        } else {
            if (a >= 0) {
                output = (a + std::abs(b)) / b;
            } else {
                output = (a - std::abs(b)) / b;
            }
        }
        return true;
    }
};

// ----------------------------------------------------------------------------
// Input/Output
// ----------------------------------------------------------------------------

/// Exports decimal to stream
/// Used format: {-}bbbb.aaaa where
/// {-} is optional '-' sign character
/// '.' is locale-dependent decimal point character
/// bbbb is stream of digits before decimal point
/// aaaa is stream of digits after decimal point
template<class decimal_type, typename StreamType>
void toStream(const decimal_type &arg, StreamType &output) {
    using namespace std;

    int64 before, after;
    int sign;

    arg.unpack(before, after);
    sign = 1;

    if (before < 0) {
        sign = -1;
        before = -before;
    }

    if (after < 0) {
        sign = -1;
        after = -after;
    }

    if (sign < 0)
        output << "-";

    const char dec_point =
            use_facet<numpunct<char> >(output.getloc()).decimal_point();
    output << before;
    if (arg.getDecimalPoints() > 0) {
        output << dec_point;
        output << setw(arg.getDecimalPoints()) << setfill('0') << right
               << after;
    }
}

namespace details {

/// Extract values from stream ready to be packed to decimal
template<typename StreamType>
bool parse_unpacked(StreamType &input, int &sign, int64 &before, int64 &after, int &decimalDigits) {
    using namespace std;

    enum StateEnum {
        IN_SIGN, IN_BEFORE_FIRST_DIG, IN_BEFORE_DEC, IN_AFTER_DEC, IN_END
    } state = IN_SIGN;
    const numpunct<char> *facet =
            has_facet<numpunct<char> >(input.getloc()) ?
            &use_facet<numpunct<char> >(input.getloc()) : NULL;
    const char dec_point = (facet != NULL) ? facet->decimal_point() : '.';
    const bool thousands_grouping =
            (facet != NULL) ? (!facet->grouping().empty()) : false;
    const char thousands_sep = (facet != NULL) ? facet->thousands_sep() : ',';
    enum ErrorCodes {
        ERR_WRONG_CHAR = -1,
        ERR_NO_DIGITS = -2,
        ERR_WRONG_STATE = -3,
        ERR_STREAM_GET_ERROR = -4
    };

    before = after = 0;
    sign = 1;

    int error = 0;
    int digitsCount = 0;
    int afterDigitCount = 0;
    char c;

    while ((input) && (state != IN_END)) // loop while extraction from file is possible
    {
        c = static_cast<char>(input.get());

        switch (state) {
            case IN_SIGN:
                if (c == '-') {
                    sign = -1;
                    state = IN_BEFORE_FIRST_DIG;
                } else if (c == '+') {
                    state = IN_BEFORE_FIRST_DIG;
                } else if ((c >= '0') && (c <= '9')) {
                    state = IN_BEFORE_DEC;
                    before = static_cast<int>(c - '0');
                    digitsCount++;
                } else if (c == dec_point) {
                    state = IN_AFTER_DEC;
                } else if ((c != ' ') && (c != '\t')) {
                    state = IN_END;
                    error = ERR_WRONG_CHAR;
                }
                // else ignore char
                break;
            case IN_BEFORE_FIRST_DIG:
                if ((c >= '0') && (c <= '9')) {
                    before = 10 * before + static_cast<int>(c - '0');
                    state = IN_BEFORE_DEC;
                    digitsCount++;
                } else if (c == dec_point) {
                    state = IN_AFTER_DEC;
                } else {
                    state = IN_END;
                    error = ERR_WRONG_CHAR;
                }
                break;
            case IN_BEFORE_DEC:
                if ((c >= '0') && (c <= '9')) {
                    before = 10 * before + static_cast<int>(c - '0');
                    digitsCount++;
                } else if (c == dec_point) {
                    state = IN_AFTER_DEC;
                } else if (thousands_grouping && c == thousands_sep) {
                    ; // ignore the char
                } else {
                    state = IN_END;
                }
                break;
            case IN_AFTER_DEC:
                if ((c >= '0') && (c <= '9')) {
                    after = 10 * after + static_cast<int>(c - '0');
                    afterDigitCount++;
                    if (afterDigitCount >= dec::max_decimal_points)
                        state = IN_END;
                } else {
                    state = IN_END;
                    if (digitsCount == 0) {
                        error = ERR_NO_DIGITS;
                    }
                }
                break;
            default:
                error = ERR_WRONG_STATE;
                state = IN_END;
                break;
        } // switch state
    } // while stream good & not end

    decimalDigits = afterDigitCount;

    if (error >= 0) {

        if (sign < 0) {
            before = -before;
            after = -after;
        }

    } else {
        before = after = 0;
    }

    return (error >= 0);
} // function

}; // details

/// Converts stream of chars to decimal
/// Handles the following formats ('.' is selected from locale info):
/// \code
/// 123
/// -123
/// 123.0
/// -123.0
/// 123.
/// .123
/// 0.
/// -.123
/// \endcode
/// Spaces and tabs on the front are ignored.
/// Performs rounding when provided value has higher precision than in output type.
/// \param[in] input input stream
/// \param[out] output decimal value, 0 on error
/// \result Returns true if conversion succeeded
template<typename decimal_type, typename StreamType>
bool fromStream(StreamType &input, decimal_type &output) {
    int sign, afterDigits;
    int64 before, after;
    bool result = details::parse_unpacked(input, sign, before, after,
                                          afterDigits);
    if (result) {
        if (afterDigits <= decimal_type::decimal_points) {
            // direct mode
            int corrCnt = decimal_type::decimal_points - afterDigits;
            while (corrCnt > 0) {
                after *= 10;
                --corrCnt;
            }
            output.pack(before, after);
        } else {
            // rounding mode
            int corrCnt = afterDigits;
            int64 decimalFactor = 1;
            while (corrCnt > 0) {
                before *= 10;
                decimalFactor *= 10;
                --corrCnt;
            }
            decimal_type temp(before + after, decimalFactor);
            output = temp;
        }
    } else {
        output = decimal_type(0);
    }
    return result;
}

/// Exports decimal to string
/// Used format: {-}bbbb.aaaa where
/// {-} is optional '-' sign character
/// '.' is locale-dependent decimal point character
/// bbbb is stream of digits before decimal point
/// aaaa is stream of digits after decimal point
template<typename decimal_type>
std::string &toString(const decimal_type &arg, std::string &output) {
    using namespace std;

    ostringstream out;
    toStream(arg, out);
    output = out.str();
    return output;
}

/// Exports decimal to string
/// Used format: {-}bbbb.aaaa where
/// {-} is optional '-' sign character
/// '.' is locale-dependent decimal point character
/// bbbb is stream of digits before decimal point
/// aaaa is stream of digits after decimal point
template<typename decimal_type>
std::string toString(const decimal_type &arg) {
    std::string res;
    toString(arg, res);
    return res;
}

/// Imports decimal from string
/// Used format: {-}bbbb.aaaa where
/// {-} is optional '-' sign character
/// '.' is locale-dependent decimal point character
/// bbbb is stream of digits before decimal point
/// aaaa is stream of digits after decimal point
template<typename T>
T fromString(const std::string &str) {
    std::istringstream is(str);
    T t;
    is >> t;
    return t;
}

template<typename T>
void fromString(const std::string &str, T &out) {
    std::istringstream is(str);
    is >> out;
}

// output
template<class charT, class traits, typename decimal_type>
std::basic_ostream<charT, traits> &
operator<<(std::basic_ostream<charT, traits> & os, const decimal_type & d) {
    toStream(d, os);
    return os;
}

// input
template<class charT, class traits, typename decimal_type>
std::basic_istream<charT, traits> &
operator>>(std::basic_istream<charT, traits> & is, decimal_type & d) {
    if (!fromStream(is, d))
        d.setUnbiased(0);
    return is;
}

}

#endif //_DECIMAL_UTILS_H
