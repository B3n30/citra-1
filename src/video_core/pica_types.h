// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cmath>
#include <cstring>
#include "common/common_types.h"

namespace Pica {

/**
 * Template class for converting arbitrary Pica float types to IEEE 754 32-bit single-precision
 * floating point.
 *
 * When decoding, format is as follows:
 *  - The first `M` bits are the mantissa
 *  - The next `E` bits are the exponent
 *  - The last bit is the sign bit
 *
 * @todo Verify on HW if this conversion is sufficiently accurate.
 */
template <unsigned M, unsigned E>
struct Float {
public:
    static Float<M, E> FromFloat32(float val) {
        u32 hex;
        std::memcpy(&hex, &val, sizeof(u32));

        // Take care of special case 0.0
        if (val == 0.f) {
            // Since Pica has no -0, we don't have to check for the sign
            return Float<M, E>::Zero();
        }
        const s32 bias = 127 - EXPONENT_BIAS;
        const s32 sign = hex & 1 << 31;
        const s32 exponent = ((hex >> 23) & ((1 << 8) - 1)) - bias;

        // Take care of the special cases NAN and INF
        if (std::isnan(val)) {
            Float<M, E> res;
            res.value = val;
            return res;
        } else if (exponent > EXPONENT_MASK) {
            // exponent is bigger then the maximum value for E, thus infinity
            if (sign) {
                Float<M, E> res;
                res.value = -std::numeric_limits<float>::infinity();
                return res;
            } else {
                Float<M, E> res;
                res.value = std::numeric_limits<float>::infinity();
                return res;
            }
        }

        // calculate with a M+1-bit mantissa and round down to M bit
        u32 mantissa = (hex & ((1 << 23) - 1)) >> (23 - M - 1);

        if (mantissa & 1) {
            // TODO(B3N30): Check that this is the correct PICA rounding
            mantissa = ((mantissa >> 1) + 1);
        } else {
            mantissa = (mantissa >> 1);
        }

        // Take care of the denormalized values. Are the limits correct?
        if ((exponent < -EXPONENT_BIAS) && (exponent > -(s32)(EXPONENT_BIAS + M))) {
            // Mask out the least significant bits that would get lost due to normalization
            mantissa &= ~((1 << (-exponent - EXPONENT_BIAS)) - 1);
        } else if (exponent <= -(s32)(EXPONENT_BIAS + M)) {
            // It's even to small for denoermalized values
            return Float<M, E>::Zero();
        }

        // TODO(B3N30): Make this faster. Compared to the original hex only the mantissa is changed
        // slightly
        u32 res = sign | ((exponent + bias) << 23) | (mantissa << (23 - M));
        Float<M, E> result;
        std::memcpy(&result.value, &res, sizeof(float));
        return result;
    }

    static Float<M, E> FromRaw(u32 hex) {
        Float<M, E> res;

        const s32 width = M + E + 1;
        const s32 bias = 127 - EXPONENT_BIAS;
        const s32 exponent = (hex >> M) & ((1 << E) - 1);
        const unsigned mantissa = hex & ((1 << M) - 1);

        if (hex & ((1 << (width - 1)) - 1))
            hex = ((hex >> (E + M)) << 31) | (mantissa << (23 - M)) | ((exponent + bias) << 23);
        else
            hex = ((hex >> (E + M)) << 31);

        std::memcpy(&res.value, &hex, sizeof(float));

        return res;
    }

    static Float<M, E> Zero() {
        Float<M, E> res;
        res.value = 0.f;
        return res;
    }

    float ToFloat32() const {
        return value;
    }

    Float<M, E> operator*(const Float<M, E>& flt) const {
        if ((ToFloat32() == 0.f && !std::isnan(flt.ToFloat32())) ||
            (flt.ToFloat32() == 0.f && !std::isnan(ToFloat32())))
            // PICA gives 0 instead of NaN when multiplying by inf
            return Zero();
        return Float<M, E>::FromFloat32(ToFloat32() * flt.ToFloat32());
    }

    Float<M, E> operator/(const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() / flt.ToFloat32());
    }

    Float<M, E> operator+(const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() + flt.ToFloat32());
    }

    Float<M, E> operator-(const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() - flt.ToFloat32());
    }

    Float<M, E>& operator*=(const Float<M, E>& flt) {
        *this = *this * flt;
        return *this;
    }

    Float<M, E>& operator/=(const Float<M, E>& flt) {
        *this = *this / flt;
        return *this;
    }

    Float<M, E>& operator+=(const Float<M, E>& flt) {
        *this = *this + flt;
        return *this;
    }

    Float<M, E>& operator-=(const Float<M, E>& flt) {
        *this = *this - flt;
        return *this;
    }

    Float<M, E> operator-() const {
        return Float<M, E>::FromFloat32(-ToFloat32());
    }

    bool operator<(const Float<M, E>& flt) const {
        return ToFloat32() < flt.ToFloat32();
    }

    bool operator>(const Float<M, E>& flt) const {
        return ToFloat32() > flt.ToFloat32();
    }

    bool operator>=(const Float<M, E>& flt) const {
        return ToFloat32() >= flt.ToFloat32();
    }

    bool operator<=(const Float<M, E>& flt) const {
        return ToFloat32() <= flt.ToFloat32();
    }

    bool operator==(const Float<M, E>& flt) const {
        return ToFloat32() == flt.ToFloat32();
    }

    bool operator!=(const Float<M, E>& flt) const {
        return ToFloat32() != flt.ToFloat32();
    }

private:
    static_assert(M + E + 1 <= 32, "Maximum bitsize is 32");
    static const unsigned EXPONENT_MASK = (1 << E) - 1;
    static const s32 EXPONENT_BIAS = (1 << (E - 1)) - 1;

    // Stored as a regular float, merely for convenience
    // TODO: Perform proper arithmetic on this!
    float value;
};

using float24 = Float<16, 7>;
using float20 = Float<12, 7>;
using float16 = Float<10, 5>;

} // namespace Pica
