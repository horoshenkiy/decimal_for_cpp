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
class decimal {
public:
    typedef dec_storage_t raw_data_t;
    enum {
        decimal_points = Prec
    };

    decimal() {
        init(0);
    }
    decimal(const decimal &src) {
        init(src);
    }
    explicit decimal(uint value) {
        init(value);
    }
    explicit decimal(int value) {
        init(value);
    }
    explicit decimal(int64 value) {
        init(value);
    }
    explicit decimal(xdouble value) {
        init(value);
    }
    explicit decimal(double value) {
        init(value);
    }
    explicit decimal(float value) {
        init(value);
    }
    explicit decimal(int64 value, int64 precFactor) {
        initWithPrec(value, precFactor);
    }
    explicit decimal(const std::string &value) {
        fromString(value, *this);
    }

    ~decimal() {
    }

    static int64 getPrecFactor() {
        return DecimalFactor<Prec>::value;
    }
    static int getDecimalPoints() {
        return Prec;
    }

    decimal & operator=(const decimal &rhs) {
        if (&rhs != this)
            m_value = rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal>::type
    & operator=(const decimal<Prec2> &rhs) {
        m_value = rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal & operator=(const decimal<Prec2> &rhs) {
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

    decimal & operator=(int64 rhs) {
        m_value = DecimalFactor<Prec>::value * rhs;
        return *this;
    }

    decimal & operator=(int rhs) {
        m_value = DecimalFactor<Prec>::value * rhs;
        return *this;
    }

    decimal & operator=(double rhs) {
        m_value = fpToStorage(rhs);
        return *this;
    }

    decimal & operator=(xdouble rhs) {
        m_value = fpToStorage(rhs);
        return *this;
    }

    bool operator==(const decimal &rhs) const {
        return (m_value == rhs.m_value);
    }

    bool operator<(const decimal &rhs) const {
        return (m_value < rhs.m_value);
    }

    bool operator<=(const decimal &rhs) const {
        return (m_value <= rhs.m_value);
    }

    bool operator>(const decimal &rhs) const {
        return (m_value > rhs.m_value);
    }

    bool operator>=(const decimal &rhs) const {
        return (m_value >= rhs.m_value);
    }

    bool operator!=(const decimal &rhs) const {
        return !(*this == rhs);
    }

    const decimal operator+(const decimal &rhs) const {
        decimal result = *this;
        result.m_value += rhs.m_value;
        return result;
    }

#if DEC_TYPE_LEVEL == 1
template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal>::type
    operator+(const decimal<Prec2> &rhs) const {
        decimal result = *this;
        result.m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal operator+(const decimal<Prec2> &rhs) const {
        decimal result = *this;
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

    decimal & operator+=(const decimal &rhs) {
        m_value += rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal>::type
    & operator+=(const decimal<Prec2> &rhs) {
        m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal & operator+=(const decimal<Prec2> &rhs) {
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

    const decimal operator+() const {
        return *this;
    }

    const decimal operator-() const {
        decimal result = *this;
        result.m_value = -result.m_value;
        return result;
    }

    const decimal operator-(const decimal &rhs) const {
        decimal result = *this;
        result.m_value -= rhs.m_value;
        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal>::type
    operator-(const decimal<Prec2> &rhs) const {
        decimal result = *this;
        result.m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal operator-(const decimal<Prec2> &rhs) const {
        decimal result = *this;
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

    decimal & operator-=(const decimal &rhs) {
        m_value -= rhs.m_value;
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal>::type
    & operator-=(const decimal<Prec2> &rhs) {
        m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal & operator-=(const decimal<Prec2> &rhs) {
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

    const decimal operator*(int rhs) const {
        decimal result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal operator*(int64 rhs) const {
        decimal result = *this;
        result.m_value *= rhs;
        return result;
    }

    const decimal operator*(const decimal &rhs) const {
        decimal result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.m_value, DecimalFactor<Prec>::value);
        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal>::type
    operator*(const decimal<Prec2>& rhs) const {
        decimal result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.getUnbiased(), DecimalFactor<Prec2>::value);
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal operator*(const decimal<Prec2>& rhs) const {
        decimal result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                rhs.getUnbiased(), DecimalFactor<Prec2>::value);
        return result;
    }
#endif

    decimal & operator*=(int rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal & operator*=(int64 rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal & operator*=(const decimal &rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.m_value,
                DecimalFactor<Prec>::value);
        return *this;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    typename std::enable_if<Prec >= Prec2, decimal>::type
    & operator*=(const decimal<Prec2>& rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                DecimalFactor<Prec2>::value);
        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal & operator*=(const decimal<Prec2>& rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                DecimalFactor<Prec2>::value);
        return *this;
    }
#endif

    const decimal operator/(int rhs) const {
        decimal result = *this;

        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs)) {
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1,
                    rhs);
        }

        return result;
    }

    const decimal operator/(int64 rhs) const {
        decimal result = *this;

        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs)) {
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1,
                    rhs);
        }

        return result;
    }

    const decimal operator/(const decimal &rhs) const {
        decimal result = *this;
        //result.m_value = (result.m_value * DecimalFactor<Prec>::value) / rhs.m_value;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec>::value, rhs.m_value);

        return result;
    }

#if DEC_TYPE_LEVEL == 1
    template<int Prec2>
    const typename std::enable_if<Prec >= Prec2, decimal>::type
    operator/(const decimal<Prec2>& rhs) const {
        decimal result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());
        return result;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    const decimal operator/(const decimal<Prec2>& rhs) const {
        decimal result = *this;
        result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());
        return result;
    }
#endif

    decimal & operator/=(int rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1,
                    rhs);
        }
        return *this;
    }

    decimal & operator/=(int64 rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1,
                    rhs);
        }
        return *this;
    }

    decimal & operator/=(const decimal &rhs) {
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
    typename std::enable_if<Prec >= Prec2, decimal>::type
    & operator/=(const decimal<Prec2> &rhs) {
        m_value = dec_utils<RoundPolicy>::multDiv(m_value,
                DecimalFactor<Prec2>::value, rhs.getUnbiased());

        return *this;
    }
#elif DEC_TYPE_LEVEL > 1
    template<int Prec2>
    decimal & operator/=(const decimal<Prec2> &rhs) {
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
    // use to load/store decimal value in external memory
    int64 getUnbiased() const {
        return m_value;
    }
    void setUnbiased(int64 value) {
        m_value = value;
    }

    decimal<Prec> abs() const {
        if (m_value >= 0)
            return *this;
        else
            return (decimal<Prec>(0) - *this);
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

    /// Returns two parts: before and after decimal point
    /// For negative values both numbers are negative or zero.
    void unpack(int64 &beforeValue, int64 &afterValue) const {
        afterValue = m_value % DecimalFactor<Prec>::value;
        beforeValue = (m_value - afterValue) / DecimalFactor<Prec>::value;
    }

    /// Combines two parts (before and after decimal point) into decimal value.
    /// Both input values have to have the same sign for correct results.
    /// Does not perform any rounding or input validation - afterValue must be less than 10^prec.
    /// \param[in] beforeValue value before decimal point
    /// \param[in] afterValue value after decimal point multiplied by 10^prec
    /// \result Returns *this
    decimal &pack(int64 beforeValue, int64 afterValue) {
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
    decimal &pack_rounded(int64 beforeValue, int64 afterValue) {
        decimal<sourcePrec> temp;
        temp.pack(beforeValue, afterValue);
        decimal<Prec> result(temp.getUnbiased(), temp.getPrecFactor());

        *this = result;
        return *this;
    }

    static decimal buildWithExponent(int64 mantissa, int exponent) {
        decimal result;
        result.setWithExponent(mantissa, exponent);
        return result;
    }

    static decimal &buildWithExponent(decimal &output, int64 mantissa,
            int exponent) {
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

    void init(const decimal &src) {
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

} // namespace

#endif // _DECIMAL_FIXED_H__
