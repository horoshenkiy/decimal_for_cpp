#pragma once

#include <dec/utils.h>

namespace dec {

// ----------------------------------------------------------------------------
// decimal with static precision
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

namespace detail {

template <int Prec>
struct DecimalFactor {
    static const int64 value = 10 * DecimalFactor<Prec - 1>::value;
};

template <>
struct DecimalFactor<0> {
    static const int64 value = 1;
};

template <>
struct DecimalFactor<1> {
    static const int64 value = 10;
};

template <int Prec, bool positive>
struct DecimalFactorDiff_impl {
    static const int64 value = DecimalFactor<Prec>::value;
};

template <int Prec>
struct DecimalFactorDiff_impl<Prec, false> {
    static const int64 value = INT64_MIN;
};

template <int Prec>
struct DecimalFactorDiff {
    static const int64 value = DecimalFactorDiff_impl<Prec, Prec >= 0>::value;
};

}

template<int Prec, class RoundPolicy = def_round_policy>
class decimal_st {

public:

    // Constructors
    // ----------------------------------------------------------------------------

    decimal_st()
        : m_value(0)
    {}

    explicit decimal_st(int64 value)
        : m_value(detail::DecimalFactor<Prec>::value * value)
    {}

    // operator=
    // ----------------------------------------------------------------------------

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
            RoundPolicy::div_rounded(
                    m_value, rhs.getUnbiased(),
                    detail::DecimalFactorDiff<Prec2 - Prec>::value
            );
        }
        else {
            m_value = rhs.getUnbiased() *
                      detail::DecimalFactorDiff<Prec - Prec2>::value;
        }

        return *this;
    }
#endif

    decimal_st & operator=(int64 rhs) {
        m_value = detail::DecimalFactor<Prec>::value * rhs;
        return *this;
    }

    // operator==/!=
    // ----------------------------------------------------------------------------

    bool operator==(const decimal_st &rhs) const {
        return (m_value == rhs.m_value);
    }

    bool operator!=(const decimal_st &rhs) const {
        return !(*this == rhs);
    }

    // operator</<=/>/>=
    // ----------------------------------------------------------------------------

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

    // operator+/+=
    // ----------------------------------------------------------------------------

    decimal_st operator+(const decimal_st &rhs) const {
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
    decimal_st operator+(const decimal_st<Prec2> &rhs) const {
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

    decimal_st& operator*=(int rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_st& operator*=(int64 rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_st& operator*=(const decimal_st &rhs) {
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
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);
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

    /// Getters for precision
    static int64 getPrecFactor() {
        return DecimalFactor<Prec>::value;
    }

    static int getDecimalPoints() {
        return Prec;
    }

    /// returns integer value = real_value * (10 ^ precision)
    [[nodiscard]]
    int64 getUnbiased() const {
        return m_value;
    }

    /// returns value as double
    [[nodiscard]]
    double getAsDouble() const {
        return static_cast<double>(m_value) / getPrecFactorDouble();
    }

    /// Returns integer indicating sign of value
    /// -1 if value is < 0
    /// +1 if value is > 0
    /// 0  if value is 0
    [[nodiscard]]
    int sign() const {
        return (m_value > 0) ? 1 : ((m_value < 0) ? -1 : 0);
    }

    /// Returns absolute value
    [[nodiscard]]
    decimal_st<Prec> abs() const {
        if (m_value >= 0)
            return *this;
        else
            return (decimal_st<Prec>(0) - *this);
    }

private:

    static double getPrecFactorDouble() {
        return static_cast<double>(DecimalFactor<Prec>::value);
    }

    static dec_storage_t intWithPrecToStorage(int64 value, int64 precFactor) {
        dec_storage_t result;
        int64 ownFactor = DecimalFactor<Prec>::value;

        if (ownFactor == precFactor) {
            // no conversion required
            result = value;
        } else {
            // conversion
            result = RoundPolicy::round(
                    static_cast<cross_float>(value) *
                    (static_cast<cross_float>(ownFactor) / static_cast<cross_float>(precFactor))
            );
        }

        return result;
    }

    static dec_storage_t doubleToStorage(double value) {
        dec_storage_t intPart = dec_utils<RoundPolicy>::trunc(value);
        double fracPart = value - intPart;
        return RoundPolicy::round(
                static_cast<double>(DecimalFactor<Prec>::value) * fracPart) +
                  DecimalFactor<Prec>::value * intPart;
    }

    dec_storage_t m_value;
};

// ----------------------------------------------------------------------------
// decimal with runtime precision
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
template<class RoundPolicy = def_round_policy>
class decimal_rt {
public:

    // Constructors
    // ----------------------------------------------------------------------------

    explicit decimal_rt(int64 value, int prec)
        : m_value(value), m_prec(prec)
    {}

    // operator=
    // ----------------------------------------------------------------------------

    decimal_rt& operator=(const decimal_rt &rhs) {
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

    decimal_rt& operator=(int64 rhs) {
        m_value = getPrecFactorInt64(m_prec) * rhs;
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

    bool operator>(const decimal_rt &rhs) const {
        return rhs < *this;
    }

    bool operator<=(const decimal_rt &rhs) const {
        return !(rhs < *this);
    }

    bool operator>=(const decimal_rt &rhs) const {
        return !(*this < rhs);
    }

    // operator+/+=
    // ----------------------------------------------------------------------------

    decimal_rt operator+(const decimal_rt &other) const {

        decimal_rt result = *this;
        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_prec - this->m_prec));
            result.m_value += val;
        }
        else if (other.m_prec == m_prec) {
            result.m_value += other.m_value;
        }
        else {
            result.m_value += other.getUnbiased() * getPrecFactorInt64(this->m_prec - other.m_prec);
        }

        return result;
    }

    decimal_rt& operator+=(const decimal_rt &other) {

        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_prec - this->m_prec));
            this->m_value += val;
        }
        else if (other.m_prec == m_prec) {
            this->m_value += other.m_value;
        }
        else {
            this->m_value += other.getUnbiased() * getPrecFactorInt64(this->m_prec - other.m_prec);
        }

        return *this;
    }

    decimal_rt operator+() const {
        return *this;
    }

    // operator-/-=
    // ----------------------------------------------------------------------------

    decimal_rt operator-(const decimal_rt &other) const {

        decimal_rt result = *this;
        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_prec - this->m_prec));
            result.m_value -= val;
        }
        else if (other.m_prec == m_prec) {
            result.m_value -= other.m_value;
        }
        else {
            result.m_value -= other.getUnbiased() * getPrecFactorInt64(this->m_prec - other.m_prec);
        }

        return result;
    }

    decimal_rt & operator-=(const decimal_rt &other) {

        if (other.m_prec > m_prec) {
            int64 val;
            RoundPolicy::div_rounded(val, other.getUnbiased(), getPrecFactorInt64(other.m_prec - this->m_prec));
            this->m_value -= val;
        }
        else if (other.m_prec == m_prec) {
            this->m_value -= other.m_value;
        }
        else {
            this->m_value -= other.getUnbiased() * getPrecFactorInt64(this->m_prec - other.m_prec);
        }

        return *this;
    }

    decimal_rt operator-() const {
        decimal_rt result = *this;
        result.m_value = -result.m_value;
        return result;
    }

    // operator*/*=
    // ----------------------------------------------------------------------------

    decimal_rt operator*(int64 rhs) const {
        decimal_rt result = *this;
        result.m_value *= rhs;
        return result;
    }

    decimal_rt operator*(const decimal_rt &other) const {
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

    decimal_rt& operator*=(int64 rhs) {
        m_value *= rhs;
        return *this;
    }

    decimal_rt& operator*=(const decimal_rt &other) {

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

    decimal_rt operator/(int64 rhs) const {
        decimal_rt result = *this;
        if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs))
            result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);

        return result;
    }

    decimal_rt operator/(const decimal_rt &other) const {
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

    decimal_rt& operator/=(int64 rhs) {
        if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs))
            this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1, rhs);
        return *this;
    }

    decimal_rt& operator/=(const decimal_rt &other) {
        if (this->m_prec == other.m_prec) {
            uint64_t factor = getPrecFactorInt64(this->m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, factor, other.m_value);
        } else {
            uint64_t factor = getPrecFactorInt64(other.m_prec);
            m_value = dec_utils<RoundPolicy>::multDiv(m_value, factor, other.m_value);
        }

        return *this;
    }

    // Methods
    // ----------------------------------------------------------------------------

    /// Getters for precision
    int64 getPrecFactor() const {
        return getPrecFactorInt64(m_prec);
    }

    int getDecimalPoints() const {
        return m_prec;
    }

    /// returns integer value = real_value * (10 ^ precision)
    int64 getUnbiased() const {
        return m_value;
    }

    /// returns value as double
    double getAsDouble() const {
        return static_cast<double>(m_value) / getPrecFactorDouble(m_prec);
    }

    /// Returns integer indicating sign of value
    /// -1 if value is < 0
    /// +1 if value is > 0
    /// 0  if value is 0
    int sign() const {
        return (m_value > 0) ? 1 : ((m_value < 0) ? -1 : 0);
    }

    decimal_rt abs() const {
        if (m_value >= 0)
            return *this;
        else
            return -(*this);
    }

private:

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

    static double getPrecFactorDouble(int prec) {
        return static_cast<double>(getPrecFactorInt64(prec));
    }

    dec_storage_t m_value;

    const int m_prec;
};

} // namespace
