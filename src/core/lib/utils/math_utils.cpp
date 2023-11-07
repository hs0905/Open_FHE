#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>

#include <memory.h>
#include <math.h>
#include <time.h>
#include <mutex>
#include <vector>


////////////////////////////////////////////////////////////
// make 192bit barrett parameter
#define SEAL_MSB_INDEX_UINT64(result, value)                                 \
    {                                                                        \
        *result = 63UL - static_cast<unsigned long>(__builtin_clzll(value)); \
    }                                                                        \

int get_significant_bit_count(std::uint64_t value)
{
    if (value == 0)
    {
        return 0;
    }

    unsigned long result = 0;
    SEAL_MSB_INDEX_UINT64(&result, value);
    return static_cast<int>(result + 1);
}

inline int get_significant_bit_count_uint(
            const std::uint64_t *value, std::size_t uint64_count)
{
    if (!uint64_count)
    {
        return 0;
    }

    value += uint64_count - 1;
    for (; *value == 0 && uint64_count > 1; uint64_count--)
    {
        value--;
    }

    return static_cast<int>(uint64_count - 1) * 64 + get_significant_bit_count(*value);
}

inline void left_shift_uint192(const std::uint64_t *operand, int shift_amount, std::uint64_t *result)
{
    const std::size_t bits_per_uint64_sz = static_cast<std::size_t>(64);
    const std::size_t shift_amount_sz = static_cast<std::size_t>(shift_amount);

    if (shift_amount_sz & (bits_per_uint64_sz << 1))
    {
        result[2] = operand[0];
        result[1] = 0;
        result[0] = 0;
    }
    else if (shift_amount_sz & bits_per_uint64_sz)
    {
        result[2] = operand[1];
        result[1] = operand[0];
        result[0] = 0;
    }
    else
    {
        result[2] = operand[2];
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    std::size_t bit_shift_amount = shift_amount_sz & (bits_per_uint64_sz - 1);

    if (bit_shift_amount)
    {
        std::size_t neg_bit_shift_amount = bits_per_uint64_sz - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[2] = (result[2] << bit_shift_amount) | (result[1] >> neg_bit_shift_amount);
        result[1] = (result[1] << bit_shift_amount) | (result[0] >> neg_bit_shift_amount);
        result[0] = result[0] << bit_shift_amount;
    }
}

inline void set_zero_uint(std::size_t uint64_count, std::uint64_t *result)
{
    for(size_t i=0; i<uint64_count; i++){
        result[i] = 0;
    }
}

inline void right_shift_uint192(const std::uint64_t *operand, int shift_amount, std::uint64_t *result)
{
    const std::size_t bits_per_uint64_sz = static_cast<std::size_t>(64);
    const std::size_t shift_amount_sz = static_cast<std::size_t>(shift_amount);

    if (shift_amount_sz & (bits_per_uint64_sz << 1))
    {
        result[0] = operand[2];
        result[1] = 0;
        result[2] = 0;
    }
    else if (shift_amount_sz & bits_per_uint64_sz)
    {
        result[0] = operand[1];
        result[1] = operand[2];
        result[2] = 0;
    }
    else
    {
        result[2] = operand[2];
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    std::size_t bit_shift_amount = shift_amount_sz & (bits_per_uint64_sz - 1);

    if (bit_shift_amount)
    {
        std::size_t neg_bit_shift_amount = bits_per_uint64_sz - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[0] = (result[0] >> bit_shift_amount) | (result[1] << neg_bit_shift_amount);
        result[1] = (result[1] >> bit_shift_amount) | (result[2] << neg_bit_shift_amount);
        result[2] = result[2] >> bit_shift_amount;
    }
}

unsigned char sub_uint64(
    uint64_t operand1, uint64_t operand2, unsigned char borrow, unsigned long long *result)
{
    *result = operand1 - operand2 - borrow;
    return static_cast<unsigned char>(operand1 < operand2 + borrow);
}

unsigned char sub_uint64(
    uint64_t operand1, uint64_t operand2, uint64_t *result)
{
    *result = operand1 - operand2;
    return static_cast<unsigned char>(operand2 > operand1);
}

inline unsigned char sub_uint(
            const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t uint64_count,
            std::uint64_t *result)
{
    // Unroll first iteration of loop. We assume uint64_count > 0.
    unsigned char borrow = sub_uint64(*operand1++, *operand2++, result++);

    // Do the rest
    for (; --uint64_count; operand1++, operand2++, result++)
    {
        unsigned long long temp_result;
        borrow = sub_uint64(*operand1, *operand2, borrow, &temp_result);
        *result = temp_result;
    }
    return borrow;
}

unsigned char add_uint64(
    uint64_t operand1, uint64_t operand2, unsigned char carry, unsigned long long *result)
{
    *result = operand1 + operand2 + carry;
    return static_cast<unsigned char>(*result < operand1);
}

unsigned char add_uint64(
    uint64_t operand1, uint64_t operand2, uint64_t *result)
{
    return add_uint64(operand1, operand2, 0, (unsigned long long *)result);
}


unsigned char add_uint(
    const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t uint64_count,
    std::uint64_t *result)
{
    // Unroll first iteration of loop. We assume uint64_count > 0.
    unsigned char carry = add_uint64(*operand1++, *operand2++, result++);

    // Do the rest
    for (; --uint64_count; operand1++, operand2++, result++)
    {
        unsigned long long temp_result;
        carry = add_uint64(*operand1, *operand2, carry, &temp_result);
        *result = temp_result;
    }
    return carry;
}

void divide_uint192_inplace(uint64_t *numerator, uint64_t denominator, uint64_t *quotient)
{
    // We expect 192-bit input
    size_t uint64_count = 3;
    int bits_per_uint64 = 64;

    // Clear quotient. Set it to zero.
    quotient[0] = 0;
    quotient[1] = 0;
    quotient[2] = 0;

    // Determine significant bits in numerator and denominator.
    int numerator_bits = get_significant_bit_count_uint(numerator, uint64_count);
    int denominator_bits = get_significant_bit_count(denominator);

    // If numerator has fewer bits than denominator, then done.
    if (numerator_bits < denominator_bits)
    {
        return;
    }

    // Only perform computation up to last non-zero uint64s.
   
    uint64_count = static_cast<size_t>((numerator_bits + bits_per_uint64 - 1) / bits_per_uint64);
    
    // Handle fast case.
    if (uint64_count == 1)
    {
        *quotient = *numerator / denominator;
        *numerator -= *quotient * denominator;
        return;
    }

    // Create temporary space to store mutable copy of denominator.
    std::vector<uint64_t> shifted_denominator(uint64_count, 0);
    shifted_denominator[0] = denominator;

    // Create temporary space to store difference calculation.
    std::vector<uint64_t> difference(uint64_count);

    // Shift denominator to bring MSB in alignment with MSB of numerator.
    int denominator_shift = numerator_bits - denominator_bits;

    left_shift_uint192(&shifted_denominator[0], denominator_shift, &shifted_denominator[0]);
    denominator_bits += denominator_shift;

    // Perform bit-wise division algorithm.
    int remaining_shifts = denominator_shift;
    while (numerator_bits == denominator_bits)
    {
        // NOTE: MSBs of numerator and denominator are aligned.

        // Even though MSB of numerator and denominator are aligned,
        // still possible numerator < shifted_denominator.
        if (sub_uint(numerator, &shifted_denominator[0], uint64_count, &difference[0]))
        {
            // numerator < shifted_denominator and MSBs are aligned,
            // so current quotient bit is zero and next one is definitely one.
            if (remaining_shifts == 0)
            {
                // No shifts remain and numerator < denominator so done.
                break;
            }

            // Effectively shift numerator left by 1 by instead adding
            // numerator to difference (to prevent overflow in numerator).
            add_uint(&difference[0], numerator, uint64_count, &difference[0]);

            // Adjust quotient and remaining shifts as a result of shifting numerator.
            left_shift_uint192(quotient, 1, quotient);
            remaining_shifts--;
        }
        // Difference is the new numerator with denominator subtracted.

        // Update quotient to reflect subtraction.
        quotient[0] |= 1;

        // Determine amount to shift numerator to bring MSB in alignment with denominator.
        numerator_bits = get_significant_bit_count_uint(&difference[0], uint64_count);
        int numerator_shift = denominator_bits - numerator_bits;
        if (numerator_shift > remaining_shifts)
        {
            // Clip the maximum shift to determine only the integer
            // (as opposed to fractional) bits.
            numerator_shift = remaining_shifts;
        }

        // Shift and update numerator.
        if (numerator_bits > 0)
        {
            left_shift_uint192(&difference[0], numerator_shift, numerator);
            numerator_bits += numerator_shift;
        }
        else
        {
            // Difference is zero so no need to shift, just set to zero.
            set_zero_uint(uint64_count, numerator);
        }

        // Adjust quotient and remaining shifts as a result of shifting numerator.
        left_shift_uint192(quotient, numerator_shift, quotient);
        remaining_shifts -= numerator_shift;
    }

    // Correct numerator (which is also the remainder) for shifting of
    // denominator, unless it is just zero.
    if (numerator_bits > 0)
    {
        right_shift_uint192(numerator, denominator_shift, numerator);
    }
}
///////////////////////////////////////////////////////////////////////