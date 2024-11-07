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

#include <iostream>
#include "math/hal/basicint.h"

extern void divide_uint192_inplace(uint64_t *numerator, uint64_t denominator, uint64_t *quotient);

void PlainModMulScalar(uint64_t* res, const uint64_t* op1, uint64_t modulus, uint64_t scalar, size_t size) {
    uint64_t numerator[3]{ 0, 0, 1 };
    uint64_t quotient[3]{ 0, 0, 0 };
    divide_uint192_inplace(numerator, modulus, quotient);
    uint64_t pq0 = quotient[0];
    uint64_t pq1 = quotient[1];
    
    scalar = scalar >= modulus ? scalar % modulus : scalar;
    
    for(size_t i=0; i<size; i++){
        const uint64_t x = op1[i];
        unsigned long long z[2];

        uint128_t product = static_cast<unsigned uint128_t>(x) * scalar;
        z[0] = static_cast<unsigned long long>(product);                        
        z[1] = static_cast<unsigned long long>(product >> 64);

        unsigned long long tmp1, tmp2[2], tmp3, carry;

        // Multiply input and const_ratio
        // Round 1
        // multiply_uint64_hw64(z[0], pq0, &carry);
        carry = static_cast<unsigned long long>(                                        
        ((static_cast<uint128_t>(z[0])                              
        * static_cast<uint128_t>(pq0)) >> 64));                    

        // multiply_uint64(z[0], pq1, tmp2);
        product = static_cast<uint128_t>(z[0]) * pq1;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);
        tmp1 = tmp2[0] + carry;
        tmp3 = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // Round 2
        // multiply_uint64(z[1], pq0, tmp2);
        product = static_cast<uint128_t>(z[1]) * pq0;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);
        tmp1 = tmp1 + tmp2[0];
        carry = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // This is all we care about
        tmp1 = z[1] * pq1 + tmp3 + carry;

        // Barrett subtraction
        tmp3 = z[0] - tmp1 * modulus;

        // One more subtraction is enough
        res[i] = tmp3 >= modulus ? tmp3 - modulus : tmp3;
    }
}

void PlainModMulEqScalar_one(uint64_t* op1, uint64_t modulus, uint64_t scalar, uint64_t pq0, uint64_t pq1) {
    // uint64_t numerator[3]{ 0, 0, 1 };
    // uint64_t quotient[3]{ 0, 0, 0 };
    // divide_uint192_inplace(numerator, modulus, quotient);
    // uint64_t pq0 = quotient[0];
    // uint64_t pq1 = quotient[1];

    scalar = scalar >= modulus ? scalar % modulus : scalar;

    const uint64_t x = op1[0];
    unsigned long long z[2];

    uint128_t product = static_cast<uint128_t>(x) * scalar;
    z[0] = static_cast<unsigned long long>(product);                        
    z[1] = static_cast<unsigned long long>(product >> 64);

    unsigned long long tmp1, tmp2[2], tmp3, carry;

    // Multiply input and const_ratio
    // Round 1
    // multiply_uint64_hw64(z[0], pq0, &carry);
    carry = static_cast<unsigned long long>(                                        
    ((static_cast<uint128_t>(z[0])                              
    * static_cast<uint128_t>(pq0)) >> 64));                    

    // multiply_uint64(z[0], pq1, tmp2);
    product = static_cast<uint128_t>(z[0]) * pq1;
    tmp2[0] = static_cast<unsigned long long>(product);                        
    tmp2[1] = static_cast<unsigned long long>(product >> 64);

    // tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);
    tmp1 = tmp2[0] + carry;
    tmp3 = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

    // Round 2
    // multiply_uint64(z[1], pq0, tmp2);
    product = static_cast<uint128_t>(z[1]) * pq0;
    tmp2[0] = static_cast<unsigned long long>(product);                        
    tmp2[1] = static_cast<unsigned long long>(product >> 64);

    // carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);
    tmp1 = tmp1 + tmp2[0];
    carry = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

    // This is all we care about
    tmp1 = z[1] * pq1 + tmp3 + carry;

    // Barrett subtraction
    tmp3 = z[0] - tmp1 * modulus;

    // One more subtraction is enough
    op1[0] = tmp3 >= modulus ? tmp3 - modulus : tmp3;
    
}

void PlainModMulEqScalar(uint64_t* op1, uint64_t modulus, uint64_t scalar, size_t size) {
    uint64_t numerator[3]{ 0, 0, 1 };
    uint64_t quotient[3]{ 0, 0, 0 };
    divide_uint192_inplace(numerator, modulus, quotient);
    uint64_t pq0 = quotient[0];
    uint64_t pq1 = quotient[1];

    scalar = scalar >= modulus ? scalar % modulus : scalar;

    for(size_t i=0; i<size; i++){
        const uint64_t x = op1[i];
        unsigned long long z[2];

        uint128_t product = static_cast<uint128_t>(x) * scalar;
        z[0] = static_cast<unsigned long long>(product);                        
        z[1] = static_cast<unsigned long long>(product >> 64);

        unsigned long long tmp1, tmp2[2], tmp3, carry;

        // Multiply input and const_ratio
        // Round 1
        // multiply_uint64_hw64(z[0], pq0, &carry);
        carry = static_cast<unsigned long long>(                                        
        ((static_cast<uint128_t>(z[0])                              
        * static_cast<uint128_t>(pq0)) >> 64));                    

        // multiply_uint64(z[0], pq1, tmp2);
        product = static_cast<uint128_t>(z[0]) * pq1;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);
        tmp1 = tmp2[0] + carry;
        tmp3 = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // Round 2
        // multiply_uint64(z[1], pq0, tmp2);
        product = static_cast<uint128_t>(z[1]) * pq0;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);
        tmp1 = tmp1 + tmp2[0];
        carry = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // This is all we care about
        tmp1 = z[1] * pq1 + tmp3 + carry;

        // Barrett subtraction
        tmp3 = z[0] - tmp1 * modulus;

        // One more subtraction is enough
        op1[i] = tmp3 >= modulus ? tmp3 - modulus : tmp3;
    }
}

void PlainModMul(uint64_t* op1, const uint64_t* op2, uint64_t modulus, size_t size) {
    uint64_t numerator[3]{ 0, 0, 1 };
    uint64_t quotient[3]{ 0, 0, 0 };
    divide_uint192_inplace(numerator, modulus, quotient);
    uint64_t pq0 = quotient[0];
    uint64_t pq1 = quotient[1];

    for(size_t i=0; i<size; i++){ // SEAL_ITERATE(iter(operand1, operand2, result), coeff_count, [&](auto I)
        // Reduces z using base 2^64 Barrett reduction
        unsigned long long z[2], tmp1, tmp2[2], tmp3, carry;
        // multiply_uint64(get<0>(I), get<1>(I), z);
        uint128_t product = static_cast<uint128_t>(op1[i]) * op2[i];
        z[0] = static_cast<unsigned long long>(product);                        
        z[1] = static_cast<unsigned long long>(product >> 64);

        // Multiply input and const_ratio
        // Round 1
        // multiply_uint64_hw64(z[0], pq0, &carry);
        carry = static_cast<unsigned long long>(                                        
        ((static_cast<uint128_t>(z[0])                              
        * static_cast<uint128_t>(pq0)) >> 64));  

        // multiply_uint64(z[0], pq1, tmp2);
        product = static_cast<uint128_t>(z[0]) * pq1;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);
        tmp1 = tmp2[0] + carry;
        tmp3 = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // Round 2
        // multiply_uint64(z[1], pq0, tmp2);
        product = static_cast<uint128_t>(z[1]) * pq0;
        tmp2[0] = static_cast<unsigned long long>(product);                        
        tmp2[1] = static_cast<unsigned long long>(product >> 64);

        // carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);
        tmp1 = tmp1 + tmp2[0];
        carry = tmp2[1] + static_cast<unsigned char>(tmp1 < tmp2[0]);

        // This is all we care about
        tmp1 = z[1] * pq1 + tmp3 + carry;

        // Barrett subtraction
        tmp3 = z[0] - tmp1 * modulus;

        // Claim: One more subtraction is enough
        op1[i] = tmp3 >= modulus? tmp3 - modulus: tmp3;
    }
}