#pragma once

#include <cstdint>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LD_HALF_FLOAT_DEFINED
#define LD_HALF_FLOAT_DEFINED

struct ld_half {
	uint16_t u;
};

/* adapted from DirectXMath XMConvertFloatToHalf */
static inline struct ld_half ld_half_from_float(float f)
{
	uint32_t Result;

	uint32_t IValue;
	std::memcpy(&IValue, &f, sizeof(IValue));
	uint32_t Sign = (IValue & 0x80000000U) >> 16U;
	IValue = IValue & 0x7FFFFFFFU; // Hack off the sign

	if (IValue > 0x477FE000U) {
		// The number is too large to be represented as a half. Saturate to infinity.
		if (((IValue & 0x7F800000) == 0x7F800000) && ((IValue & 0x7FFFFF) != 0)) {
			Result = 0x7FFF; // NAN
		} else {
			Result = 0x7C00U; // INF
		}
	} else if (!IValue) {
		Result = 0;
	} else {
		if (IValue < 0x38800000U) {
			// The number is too small to be represented as a normalized half.
			// Convert it to a denormalized value.
			uint32_t Shift = 113U - (IValue >> 23U);
			IValue = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
		} else {
			// Rebias the exponent to represent the value as a normalized half.
			IValue += 0xC8000000U;
		}

		Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
	}

	struct ld_half h;
	h.u = static_cast<uint16_t>(Result | Sign);
	return h;
}

static inline struct ld_half ld_half_from_bits(uint16_t u)
{
	struct ld_half h;
	h.u = u;
	return h;
}

#endif // LD_HALF_FLOAT_DEFINED

#ifdef __cplusplus
}
#endif
