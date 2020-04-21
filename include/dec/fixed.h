/////////////////////////////////////////////////////////////////////////////
// Name:        fixed.h
// Purpose:     Decimal data type support, for COBOL-like fixed-point
//              operations on currency values.
// Author:      Piotr Likus
// Created:     03/01/2011
// Last change: 21/04/2020
// Version:     1.16
// Licence:     BSD
/////////////////////////////////////////////////////////////////////////////

#ifndef _DECIMAL_FIXED_H__
#define _DECIMAL_FIXED_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file fixed.h
///
/// Decimal value type. Use for capital calculations.
/// Note: maximum handled value is: +9,223,372,036,854,775,807 (divided by prec)
///
/// Sample usage:
///   using namespace dec;
///   decimal<2> value(143125);
///   value = value / decimal_cast<2>(333);
///   cout << "Result is: " << value << endl;

#include <dec/utils.h>

namespace dec {

enum class PrecType {
    STATIC_PRECISION,
    RUNTIME_PRECISION
};

// ----------------------------------------------------------------------------
// decimal with static precision
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
template<int Prec> struct DecimalFactor {
    static const int64 value = 10 * DecimalFactor<Prec - 1>::value;
};

template<> struct DecimalFactor<0> {
    static const int64 value = 1;
};

template<> struct DecimalFactor<1> {
    static const int64 value = 10;
};

template<int Prec, bool positive> struct DecimalFactorDiff_impl {
    static const int64 value = DecimalFactor<Prec>::value;
};

template<int Prec> struct DecimalFactorDiff_impl<Prec, false> {
    static const int64 value = INT64_MIN;
};

template<int Prec> struct DecimalFactorDiff {
    static const int64 value = DecimalFactorDiff_impl<Prec, Prec >= 0>::value;
};

template<int Prec, class RoundPolicy = def_round_policy>
class decimal_st {
public:

    static inline PrecType prec_type = PrecType::STATIC_PRECISION;
    typedef dec_storage_t raw_data_t;
    enum {
        decimal_points = Prec
    };

    decimal_st() {
        init(0);
    }
    decimal_st(const decimal_st &src) {
        init(src);
    }
    explicit decimal_st(uint value) {
        init(value);
    }
    explicit decimal_st(int value) {
        init(value);
    }
    explicit decimal_st(int64 value) {
        init(value);
    }
    explicit decimal_st(xdouble value) {
        init(value);
    }
    explicit decimal_st(double value) {
        init(value);
    }
    explicit decimal_st(float value) {
        init(value);
    }
    explicit decimal_st(int64 value, int64 precFactor) {
        initWithPrec(value, precFactor);
    }
    explicit decimal_st(const std::string &value) {
        fromString(value, *this);
    }

    ~decimal_st() {
    }

    static int64 getPrecFactor() {
        return DecimalFactor<Prec>::value;
    }
    static int getDecimalPoints() {
        return Prec;
    }

    decimal_st & operator=(const decimal_st &rhs) {
        if (&rhs != this)
            m_value = rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal_st>::type
    & operator=(const decimal_st<Prec2> &rhs) {
        m_value = rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal_st & operator=(const decimal_st<Prec2> &rhs) {
        if (Prec2 > Prec) {
            RoundPolicy::div_rounded(m_value, rhs.getUnbiased(),
                    DecimalFactorDiff<Prec2 - Prec>::value);
        } else {
            m_value = rhs.getUnbiased()
                    * DecimalFactorDiff<Prec - Prec2>::value;
        }
        return *this;
    }
#endif

    decimal_st & operator=(int64 rhs) {
        m_value = DecimalFactor<Prec>::value * rhs;
        return *this;
    }

    decimal_st & operator=(int rhs) {
        m_value = DecimalFactor<Prec>::value * rhs;
        return *this;
    }

    decimal_st & operator=(double rhs) {
        m_value = fpToStorage(rhs);
        return *this;
    }

    decimal_st & operator=(xdouble rhs) {
        m_value = fpToStorage(rhs);
        return *this;
    }

    bool operator==(const decimal_st &rhs) const {
        return (m_value == rhs.m_value);
    }

    bool operator<(const decimal_st &rhs) const {
        return (m_value < rhs.m_value);
    }

    bool operator<=(const decimal_st &rhs) const {
        return (m_value <= rhs.m_value);
    }

    bool operator>(const decimal_st &rhs) const {
        return (m_value > rhs.m_value);
    }

    bool operator>=(const decimal_st &rhs) const {
        return (m_value >= rhs.m_value);
    }

    bool operator!=(const decimal_st &rhs) const {
        return !(*this == rhs);
    }

    const decimal_st operator+(const decimal_st &rhs) const {
        decimal_st result = *this;
        result.m_value += rhs.m_value;
        return result;
    }

#if DEC_TYPE_LEVEL == 1
template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal_st>::type
    operator+(const decimal_st<Prec2> &rhs) const {
        decimal_st result = *this;
        result.m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal_st operator+(const decimal_st<Prec2> &rhs) const {
        decimal_st result = *this;
        if (Prec2 > Prec) {
            int64 val;
            RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                    DecimalFactorDiff<Prec2 - Prec>::value);
            result.m_value += val;
        } else {
            result.m_value += rhs.getUnbiased()
                    * DecimalFactorDiff<Prec - Prec2>::value;
        }

        return result;
    }
#endif

    decimal_st & operator+=(const decimal_st &rhs) {
        m_value += rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal_st>::type
    & operator+=(const decimal_st<Prec2> &rhs) {
        m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal_st & operator+=(const decimal_st<Prec2> &rhs) {
        if (Prec2 > Prec) {
            int64 val;
            RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                    DecimalFactorDiff<Prec2 - Prec>::value);
            m_value += val;
        } else {
            m_value += rhs.getUnbiased()
                    * DecimalFactorDiff<Prec - Prec2>::value;
        }

        return *this;
    }
#endif

    const decimal_st operator+() const {
        return *this;
    }

    const decimal_st operator-() const {
        decimal_st result = *this;
        result.m_value = -result.m_value;
        return result;
    }

    const decimal_st operator-(const decimal_st &rhs) const {
        decimal_st result = *this;
        result.m_value -= rhs.m_value;
        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal_st>::type
    operator-(const decimal_st<Prec2> &rhs) const {
        decimal_st result = *this;
        result.m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal_st operator-(const decimal_st<Prec2> &rhs) const {
        decimal_st result = *this;
        if (Prec2 > Prec) {
            int64 val;
            RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                    DecimalFactorDiff<Prec2 - Prec>::value);
            result.m_value -= val;
        } else {
            result.m_value -= rhs.getUnbiased()
                    * DecimalFactorDiff<Prec - Prec2>::value;
        }

        return result;
    }
#endif

    decimal_st & operator-=(const decimal_st &rhs) {
        m_value -= rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal_st>::type
    & operator-=(const decimal_st<Prec2> &rhs) {
        m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal_st & operator-=(const decimal_st<Prec2> &rhs) {
        if (Prec2 > Prec) {
            int64 val;
            RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                    DecimalFactorDiff<Prec2 - Prec>::value);
            m_value -= val;
        } else {
            m_value -= rhs.getUnbiased()
                    * DecimalFactorDiff<Prec - Prec2>::value;
        }

        return *this;
    }
#endif

    const decimal_st operator*(int rhs) const {
        decimal_st result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal_st operator*(int64 rhs) const {
        decimal_st result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal_st operator*(const decimal_st &rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.m_value, DecimalFactor<Prec>::value);
        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal_st>::type
    operator*(const decimal_st<Prec2>& rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.getUnbiased(), DecimalFactor<Prec2>::value);
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal_st operator*(const decimal_st<Prec2>& rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.getUnbiased(), DecimalFactor<Prec2>::value);
        return result;
    }
#endif

    decimal_st & operator*=(int rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_st & operator*=(int64 rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_st & operator*=(const decimal_st &rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.m_value,
                DecimalFactor<Prec>::value);
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal_st>::type
    & operator*=(const decimal_st<Prec2>& rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                DecimalFactor<Prec2>::value);
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal_st & operator*=(const decimal_st<Prec2>& rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                DecimalFactor<Prec2>::value);
        return *this;
    }
#endif

    const decimal_st operator/(int rhs) const {
        decimal_st result = *this;
        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs))
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);

        return result;
    }

    const decimal_st operator/(int64 rhs) const {
        decimal_st result = *this;

        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs)) {
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1,
                    rhs);
        }

        return result;
    }

    const decimal_st operator/(const decimal_st &rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec>::value, rhs.m_value);

        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal_st>::type
    operator/(const decimal_st<Prec2>& rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal_st operator/(const decimal_st<Prec2>& rhs) const {
        decimal_st result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());
        return result;
    }
#endif

    decimal_st & operator/=(int rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1,
                    rhs);
        }
        return *this;
    }

    decimal_st & operator/=(int64 rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1,
                    rhs);
        }
        return *this;
    }

    decimal_st & operator/=(const decimal_st &rhs) {
        //m_value = (m_value * DecimalFactor<Prec>::value) / rhs.m_value;
        m_value = dec_utils<RoundPolicy>::multDiv(m_value,
                DecimalFactor<Prec>::value, rhs.m_value);

        return *this;
    }

    /// Returns integer indicating sign of value
    /// -1 if value is < 0
    /// +1 if value is > 0
    /// 0  if value is 0
    int sign() const {
        return (m_value > 0) ? 1 : ((m_value < 0) ? -1 : 0);
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal_st>::type
    & operator/=(const decimal_st<Prec2> &rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());

        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal_st & operator/=(const decimal_st<Prec2> &rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());

        return *this;
    }
#endif

    double getAsDouble() const {
        return static_cast<double>(m_value) / getPrecFactorDouble();
    }

    void setAsDouble(double value) {
        m_value = fpToStorage(value);
    }

    xdouble getAsXDouble() const {
        return static_cast<xdouble>(m_value) / getPrecFactorXDouble();
    }

    void setAsXDouble(xdouble value) {
        m_value = fpToStorage(value);
    }

    // returns integer value = real_value * (10 ^ precision)
    // use to load/store decimal_st value in external memory
    int64 getUnbiased() const {
        return m_value;
    }
    void setUnbiased(int64 value) {
        m_value = value;
    }

    decimal_st<Prec> abs() const {
        if (m_value >= 0)
            return *this;
        else
            return (decimal_st<Prec>(0) - *this);
    }

    /// returns value rounded to integer using active rounding policy
    int64 getAsInteger() const {
        int64 result;
        RoundPolicy::div_rounded(result, m_value, DecimalFactor<Prec>::value);
        return result;
    }

    /// overwrites internal value with integer
    void setAsInteger(int64 value) {
        m_value = DecimalFactor<Prec>::value * value;
    }

    /// Returns two parts: before and after decimal_st point
    /// For negative values both numbers are negative or zero.
    void unpack(int64 &beforeValue, int64 &afterValue) const {
        afterValue = m_value % DecimalFactor<Prec>::value;
        beforeValue = (m_value - afterValue) / DecimalFactor<Prec>::value;
    }

    /// Combines two parts (before and after decimal_st point) into decimal_st value.
    /// Both input values have to have the same sign for correct results.
    /// Does not perform any rounding or input validation - afterValue must be less than 10^prec.
    /// \param[in] beforeValue value before decimal_st point
    /// \param[in] afterValue value after decimal_st point multiplied by 10^prec
    /// \result Returns *this
    decimal_st &pack(int64 beforeValue, int64 afterValue) {
        if (Prec > 0) {
            m_value = beforeValue * DecimalFactor<Prec>::value;
            m_value += (afterValue % DecimalFactor<Prec>::value);
        } else
            m_value = beforeValue * DecimalFactor<Prec>::value;
        return *this;
    }

    /// Version of pack() with rounding, sourcePrec specifies precision of source values.
    /// See also @pack.
    template<int sourcePrec>
    decimal_st &pack_rounded(int64 beforeValue, int64 afterValue) {
        decimal_st<sourcePrec> temp;
        temp.pack(beforeValue, afterValue);
        decimal_st<Prec> result(temp.getUnbiased(), temp.getPrecFactor());

        *this = result;
        return *this;
    }

    static decimal_st buildWithExponent(int64 mantissa, int exponent) {
        decimal_st result;
        result.setWithExponent(mantissa, exponent);
        return result;
    }

    static decimal_st &buildWithExponent(decimal_st &output, int64 mantissa, int exponent) {
        output.setWithExponent(mantissa, exponent);
        return output;
    }

    void setWithExponent(int64 mantissa, int exponent) {

        int exponentForPack = exponent + Prec;

        if (exponentForPack < 0) {
            int64 newValue;

            if (!RoundPolicy::div_rounded(newValue, mantissa,
                    dec_utils<RoundPolicy>::pow10(-exponentForPack))) {
                newValue = 0;
            }

            m_value = newValue;
        } else {
            m_value = mantissa * dec_utils<RoundPolicy>::pow10(exponentForPack);
        }
    }

    void getWithExponent(int64 &mantissa, int &exponent) const {
        int64 value = m_value;
        int exp = -Prec;

        if (value != 0) {
            // normalize
            while (value % 10 == 0) {
                value /= 10;
                exp++;
            }
        }

        mantissa = value;
        exponent = exp;
    }

protected:

    inline xdouble getPrecFactorXDouble() const {
        return static_cast<xdouble>(DecimalFactor<Prec>::value);
    }

    inline double getPrecFactorDouble() const {
        return static_cast<double>(DecimalFactor<Prec>::value);
    }

    void init(const decimal_st &src) {
        m_value = src.m_value;
    }

    void init(uint value) {
        m_value = DecimalFactor<Prec>::value * value;
    }

    void init(int value) {
        m_value = DecimalFactor<Prec>::value * value;
    }

    void init(int64 value) {
        m_value = DecimalFactor<Prec>::value * value;
    }

    void init(xdouble value) {
        m_value = fpToStorage(value);
    }

    void init(double value) {
        m_value = fpToStorage(value);
    }

    void init(float value) {
        m_value = fpToStorage(static_cast<double>(value));
    }

    void initWithPrec(int64 value, int64 precFactor) {
        int64 ownFactor = DecimalFactor<Prec>::value;

        if (ownFactor == precFactor) {
            // no conversion required
            m_value = value;
        } else {
            // conversion
            m_value = RoundPolicy::round(
                    static_cast<cross_float>(value)
                            * (static_cast<cross_float>(ownFactor)
                                    / static_cast<cross_float>(precFactor)));
        }
    }

    template<typename T>
    static dec_storage_t fpToStorage(T value) {
        dec_storage_t intPart = dec_utils<RoundPolicy>::trunc(value);
        T fracPart = value - intPart;
        return RoundPolicy::round(
                static_cast<T>(DecimalFactor<Prec>::value) * fracPart) +
                  DecimalFactor<Prec>::value * intPart;
    }

    template<typename T>
    static T abs(T value) {
        if (value < 0)
            return -value;
        else
            return value;
    }
protected:
    dec_storage_t m_value;
};

template<int Prec, class RoundPolicy = def_round_policy>
using decimal = decimal_st<Prec, RoundPolicy>;

// ----------------------------------------------------------------------------
// Pre-defined types
// ----------------------------------------------------------------------------
typedef decimal<2> decimal2;
typedef decimal<4> decimal4;
typedef decimal<6> decimal6;

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------
template<int Prec, class T>
decimal<Prec> decimal_cast(const T &arg) {
    return decimal<Prec>(arg.getUnbiased(), arg.getPrecFactor());
}

// Example of use:
//   c = dec::decimal_cast<6>(a * b);
template<int Prec>
decimal<Prec> decimal_cast(uint arg) {
    decimal<Prec> result(arg);
    return result;
}

template<int Prec>
decimal<Prec> decimal_cast(int arg) {
    decimal<Prec> result(arg);
    return result;
}

template<int Prec>
decimal<Prec> decimal_cast(int64 arg) {
    decimal<Prec> result(arg);
    return result;
}

template<int Prec>
decimal<Prec> decimal_cast(double arg) {
    decimal<Prec> result(arg);
    return result;
}

template<int Prec>
decimal<Prec> decimal_cast(const std::string &arg) {
    decimal<Prec> result(arg);
    return result;
}

template<int Prec, int N>
decimal<Prec> decimal_cast(const char (&arg)[N]) {
    decimal<Prec> result(arg);
    return result;
}

// with rounding policy
template<int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(uint arg) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

template<int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(int arg) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

template<int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(int64 arg) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

template<int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(double arg) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

template<int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(const std::string &arg) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

template<int Prec, typename RoundPolicy, int N>
decimal<Prec, RoundPolicy> decimal_cast(const char (&arg)[N]) {
    decimal<Prec, RoundPolicy> result(arg);
    return result;
}

// ----------------------------------------------------------------------------
// decimal with runtime precision
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
template<class RoundPolicy = def_round_policy>
class decimal_rt {
public:
    static inline PrecType prec_type = PrecType::RUNTIME_PRECISION;
    typedef dec_storage_t raw_data_t;

    // Constructors
    // ----------------------------------------------------------------------------

    // with rounding
    explicit decimal_rt(int64 value, int prec)
        : m_value(value), m_prec(prec)
    {}

    explicit decimal_rt(int value, int prec)
        : m_value(value), m_prec(prec)
    {}

    explicit decimal_rt(uint value, int prec)
        : m_value(value), m_prec(prec)
    {}

    // from floating
    explicit decimal_rt(xdouble value, int prec)
        : m_value(fpToStorage(value, prec)), m_prec(prec)
    {}

    explicit decimal_rt(float value, int prec)
        : m_value(fpToStorage(value, prec)), m_prec(prec)
    {}

    explicit decimal_rt(double value, int prec)
        : m_value(fpToStorage(value, prec)), m_prec(prec)
    {}

    // string
    explicit decimal_rt(const std::string &value) {
        fromString(value, *this);
    }

    // operator=
    // ----------------------------------------------------------------------------

    decimal_rt & operator=(const decimal_rt &rhs) {
        if (&rhs != this) {
            if (rhs.m_prec > m_prec)
                RoundPolicy::div_rounded(m_value, rhs.getUnbiased(), getPrecFactorInt64(rhs.m_prec - m_prec));
            else if (rhs.m_prec == m_prec)
                m_value = rhs.m_value;
            else
                m_value = rhs.getUnbiased() * getPrecFactorInt64(m_prec - rhs.m_prec);
        }

        return *this;
    }

    decimal_rt & operator=(int64 rhs) {
        m_value = getPrecFactorInt64(m_prec) * rhs;
        return *this;
    }

    decimal_rt & operator=(int rhs) {
        m_value = getPrecFactorInt64(m_prec) * rhs;
        return *this;
    }

    decimal_rt & operator=(double rhs) {
        m_value = fpToStorage(rhs, m_prec);
        return *this;
    }

    decimal_rt & operator=(xdouble rhs) {
        m_value = fpToStorage(rhs, m_prec);
        return *this;
    }

    // operator==/!=
    // ----------------------------------------------------------------------------

    bool operator==(const decimal_rt &other) const {

        if (other.m_prec > m_prec) {
            dec_storage_t lhs = this->m_value * getPrecFactorInt64(other.m_prec - this->m_prec);
            dec_storage_t rhs = other.m_value;
            return lhs == rhs;
        }
        else if (other.m_prec == m_prec) {
            return this->m_value == other.m_value;
        }
        else {
            dec_storage_t lhs = this->m_value;
            dec_storage_t rhs = other.m_value * getPrecFactorInt64(this->m_prec - other.m_prec);
            return lhs == rhs;
        }
    }

    bool operator!=(const decimal_rt &rhs) const {
        return !(*this == rhs);
    }

    // operator</<=/>/>=
    // ----------------------------------------------------------------------------

    bool operator<(const decimal_rt &other) const {

        if (other.m_prec > m_prec) {
            dec_storage_t lhs = this->m_value * getPrecFactorInt64(other.m_prec - this->m_prec);
            dec_storage_t rhs = other.m_value;
            return lhs < rhs;
        }
        else if (other.m_prec == m_prec) {
            return this->m_value < other.m_value;
        }
        else {
            dec_storage_t lhs = this->m_value;
            dec_storage_t rhs = other.m_value * getPrecFactorInt64(this->m_prec - other.m_prec);
            return lhs < rhs;
        }
    }

    bool operator>(const decimal_rt &rhs) {
        return rhs < *this;
    }

    bool operator<=(const decimal_rt &rhs) {
        return !(rhs < *this);
    }

    bool operator>=(const decimal_rt &rhs) {
        return !(*this < rhs);
    }

    // operator+/+=
    // ----------------------------------------------------------------------------

    const decimal_rt operator+(const decimal_rt &other) const {

        decimal_rt result = *this;
        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_value - this->m_value));
            result.m_value += val;
        }
        else if (other.m_prec == m_prec) {
            result.m_value += other.m_value;
        }
        else {
            result.m_value += other.getUnbiased() * getPrecFactorInt64(this->m_value - other.m_value);
        }

        return result;
    }

    decimal_rt & operator+=(const decimal_rt &other) {

        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_value - this->m_value));
            this->m_value += val;
        }
        else if (other.m_prec == m_prec) {
            this->m_value += other.m_value;
        }
        else {
            this->m_value += other.getUnbiased() * getPrecFactorInt64(this->m_value - other.m_value);
        }

        return *this;
    }

    const decimal_rt operator+() const {
        return *this;
    }

    // operator-/-=
    // ----------------------------------------------------------------------------

    const decimal_rt operator-(const decimal_rt &other) const {

        decimal_rt result = *this;
        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_value - this->m_value));
            result.m_value -= val;
        }
        else if (other.m_prec == m_prec) {
            result.m_value -= other.m_value;
        }
        else {
            result.m_value -= other.getUnbiased() * getPrecFactorInt64(this->m_value - other.m_value);
        }

        return result;
    }

    decimal_rt & operator-=(const decimal_rt &other) {

        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_value - this->m_value));
            this->m_value -= val;
        }
        else if (other.m_prec == m_prec) {
            this->m_value -= other.m_value;
        }
        else {
            this->m_value -= other.getUnbiased() * getPrecFactorInt64(this->m_value - other.m_value);
        }

        return *this;
    }

    const decimal_rt operator-() const {
        decimal_rt result = *this;
        result.m_value = -result.m_value;
        return result;
    }

    // operator*/*=
    // ----------------------------------------------------------------------------

    const decimal_rt operator*(int rhs) const {
        decimal_rt result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal_rt operator*(int64 rhs) const {
        decimal_rt result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal_rt operator*(const decimal_rt &other) const {
        decimal_rt result = *this;
        if (this->m_prec == other.m_prec) {
            uint64_t factor = getPrecFactorInt64(this->m_prec);
            result.m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, other.m_value, factor);
        }
        else {
            uint64_t factor = getPrecFactorInt64(other.m_prec);
            result.m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, other.m_value, factor);
        }

        return result;
    }

    decimal_rt & operator*=(int rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_rt & operator*=(int64 rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_rt & operator*=(const decimal_rt &other) {

        if (this->m_prec == other.m_prec) {
            uint64_t factor = getPrecFactorInt64(this->m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, other.m_value, factor);
        }
        else {
            uint64_t factor = getPrecFactorInt64(other.m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, other.m_value, factor);
        }

        return *this;
    }

    // operator/ / operator/=
    // ----------------------------------------------------------------------------

    const decimal_rt operator/(int rhs) const {
        decimal_rt result = *this;
        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs))
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);

        return result;
    }

    const decimal_rt operator/(int64 rhs) const {
        decimal_rt result = *this;
        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs))
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);

        return result;
    }

    const decimal_rt operator/(const decimal_rt &other) const {
        decimal_rt result = *this;
        if (this->m_prec == other.m_prec) {
            uint64_t factor = getPrecFactorInt64(this->m_prec);
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, factor, other.m_value);
        } else {
            uint64_t factor = getPrecFactorInt64(other.m_prec);
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, factor, other.m_value);
        }

        return result;
    }

    decimal_rt & operator/=(int rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs))
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1, rhs);
        return *this;
    }

    decimal_rt & operator/=(int64 rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs))
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1, rhs);
        return *this;
    }

    decimal_rt & operator/=(const decimal_rt &other) {
        if (this->m_prec == other.m_prec) {
            uint64_t factor = getPrecFactorInt64(this->m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, factor, other.m_value);
        } else {
            uint64_t factor = getPrecFactorInt64(other.m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, factor, other.m_value);
        }

        return *this;
    }

    /// Returns integer indicating sign of value
    /// -1 if value is < 0
    /// +1 if value is > 0
    /// 0  if value is 0
    int sign() const {
        return (m_value > 0) ? 1 : ((m_value < 0) ? -1 : 0);
    }

    // Methods
    // ----------------------------------------------------------------------------

    int64 getPrecFactor() {
        return getPrecFactorInt64(m_prec);
    }

    int getDecimalPoints() {
        return m_prec;
    }

    double getAsDouble() const {
        return static_cast<double>(m_value) / getPrecFactorDouble(m_prec);
    }

    void setAsDouble(double value) {
        m_value = fpToStorage(value, m_prec);
    }

    xdouble getAsXDouble() const {
        return static_cast<xdouble>(m_value) / getPrecFactorXDouble(m_prec);
    }

    void setAsXDouble(xdouble value) {
        m_value = fpToStorage(value, m_prec);
    }

    // returns integer value = real_value * (10 ^ precision)
    // use to load/store decimal_rt value in external memory
    int64 getUnbiased() const {
        return m_value;
    }

    void setUnbiased(int64 value) {
        m_value = value;
    }

    decimal_rt abs() const {
        if (m_value >= 0)
            return *this;
        else
            return -(*this);
    }

    /// returns value rounded to integer using active rounding policy
    int64 getAsInteger() const {
        int64 result;
        RoundPolicy::div_rounded(result, m_value, getPrecFactorInt64(m_prec));
        return result;
    }

    /// overwrites internal value with integer
    void setAsInteger(int64 value) {
        m_value = getPrecFactorInt64(m_prec) * value;
    }

    /// Returns two parts: before and after decimal_rt point
    /// For negative values both numbers are negative or zero.
    void unpack(int64 &beforeValue, int64 &afterValue) const {
        uint64_t factor = getPrecFactorInt64(m_prec);
        afterValue = m_value % factor;
        beforeValue = (m_value - afterValue) / factor;
    }

    /// Combines two parts (before and after decimal_rt point) into decimal_rt value.
    /// Both input values have to have the same sign for correct results.
    /// Does not perform any rounding or input validation - afterValue must be less than 10^prec.
    /// \param[in] beforeValue value before decimal_rt point
    /// \param[in] afterValue value after decimal_rt point multiplied by 10^prec
    /// \result Returns *this
    decimal_rt &pack(int64 beforeValue, int64 afterValue) {
        uint64_t factor = getPrecFactorInt64(m_prec);
        if (m_prec > 0) {
            m_value = beforeValue * factor;
            m_value += (afterValue % factor);
        } else
            m_value = beforeValue * factor;
        return *this;
    }

    static decimal_rt buildWithExponent(int64 mantissa, int exponent) {
        decimal_rt result;
        result.setWithExponent(mantissa, exponent);
        return result;
    }

    static decimal_rt &buildWithExponent(decimal_rt &output, int64 mantissa, int exponent) {
        output.setWithExponent(mantissa, exponent);
        return output;
    }

    void setWithExponent(int64 mantissa, int exponent) {

        int exponentForPack = exponent + m_prec;
        if (exponentForPack < 0) {
            int64 newValue;
            if (!RoundPolicy::div_rounded(newValue, mantissa, dec_utils<RoundPolicy>::pow10(-exponentForPack)))
                newValue = 0;
            m_value = newValue;
        } else
            m_value = mantissa * dec_utils<RoundPolicy>::pow10(exponentForPack);
    }

    void getWithExponent(int64 &mantissa, int &exponent) const {
        int64 value = m_value;
        int exp = -m_prec;

        if (value != 0) {
            // normalize
            while (value % 10 == 0) {
                value /= 10;
                exp++;
            }
        }

        mantissa = value;
        exponent = exp;
    }

protected:

    static int64_t getPrecFactorInt64(int prec) {
        static int64_t prec_factor_mas[] = {
                1,
                10,
                100,
                1'000,
                10'000,
                100'000,
                1'000'000,
                10'000'000,
                100'000'000,
                1'000'000'000,
                10'000'000'000,
                100'000'000'000,
                1'000'000'000'000,
                10'000'000'000'000,
                100'000'000'000'000,
                1'000'000'000'000'000,
                10'000'000'000'000'000,
                100'000'000'000'000'000
        };

        return prec_factor_mas[prec];
    }

    static xdouble getPrecFactorXDouble(int prec) {
        return static_cast<xdouble>(getPrecFactorInt64(prec));
    }

    static double getPrecFactorDouble(int prec) {
        return static_cast<double>(getPrecFactorInt64(prec));
    }

    template<typename T>
    static dec_storage_t fpToStorage(T value, int prec) {
        dec_storage_t intPart = dec_utils<RoundPolicy>::trunc(value);
        T fracPart = value - intPart;
        return RoundPolicy::round(
                static_cast<T>(getPrecFactorInt64(prec)) * fracPart) +
                getPrecFactorInt64(prec) * intPart;
    }

    template<typename T>
    static T abs(T value) {
        if (value < 0)
            return -value;
        else
            return value;
    }

protected:
    dec_storage_t m_value;
    const int m_prec;
};

} // namespace

#endif // _DECIMAL_FIXED_H__
