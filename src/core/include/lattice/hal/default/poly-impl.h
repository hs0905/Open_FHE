//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2023, NJIT, Duality Technologies Inc. and other contributors
//
// All rights reserved.
//
// Author TPOC: contact@openfhe.org
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==================================================================================

/*
  implementation of the integer lattice
 */

#ifndef LBCRYPTO_INC_LATTICE_HAL_DEFAULT_POLY_IMPL_H
#define LBCRYPTO_INC_LATTICE_HAL_DEFAULT_POLY_IMPL_H

#include "lattice/hal/default/poly.h"

#include "utils/debug.h"
#include "utils/exception.h"
#include "utils/inttypes.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <deque>

/* function for recording memory transfer stats (utils/math_utils.cpp)*/
void inc_copy_from_root_shadow_hbm_real();
void inc_copy_from_inv_root_shadow_hbm_real();

/* function for fhe unit operation (utils/math_utils.cpp)*/
extern void PlainModMul(uint64_t* op1, const uint64_t* op2, uint64_t modulus, size_t size);
extern void PlainModMulScalar(uint64_t* res, const uint64_t* op1, uint64_t modulus, uint64_t scalar, size_t size);
extern void PlainModMulEqScalar(uint64_t* op1, uint64_t modulus, uint64_t scalar, size_t size);
extern void PlainModMulEqScalar_one(uint64_t* op1, uint64_t modulus, uint64_t scalar, uint64_t pq0, uint64_t pq1);
extern void divide_uint192_inplace(uint64_t *numerator, uint64_t denominator, uint64_t *quotient);

extern std::chrono::duration<double, std::milli> elapsed_getwork;
extern std::chrono::duration<double, std::milli> elapsed_work;

#include "utils/custom_task.h"
#include "utils/memory_tracking.h"
#include "utils/logger.h"
extern WorkQueue work_queue;

namespace lbcrypto {

template <typename VecType>
PolyImpl<VecType>::PolyImpl(const DggType& dgg, const std::shared_ptr<PolyImpl::Params>& params, Format format)
    : m_format{Format::COEFFICIENT},
      m_params{params},
      m_values{std::make_unique<VecType>(dgg.GenerateVector(params->GetRingDimension(), params->GetModulus()))} {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType>::SetFormat(format);
}

template <typename VecType>
PolyImpl<VecType>::PolyImpl(DugType& dug, const std::shared_ptr<PolyImpl::Params>& params, Format format)
    : m_format{Format::COEFFICIENT},
      m_params{params},
      m_values{std::make_unique<VecType>(dug.GenerateVector(params->GetRingDimension(), params->GetModulus()))} {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType>::SetFormat(format);
}

template <typename VecType>
PolyImpl<VecType>::PolyImpl(const BugType& bug, const std::shared_ptr<PolyImpl::Params>& params, Format format)
    : m_format{Format::COEFFICIENT},
      m_params{params},
      m_values{std::make_unique<VecType>(bug.GenerateVector(params->GetRingDimension(), params->GetModulus()))} {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType>::SetFormat(format);
}

template <typename VecType>
PolyImpl<VecType>::PolyImpl(const TugType& tug, const std::shared_ptr<PolyImpl::Params>& params, Format format,
                            uint32_t h)
    : m_format{Format::COEFFICIENT},
      m_params{params},
      m_values{std::make_unique<VecType>(tug.GenerateVector(params->GetRingDimension(), params->GetModulus(), h))} {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType>::SetFormat(format);
}

/* Similar with PolyImpl(const PolyType& p)
    copy polynomial rhs -> this */
template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(const PolyImpl& rhs) noexcept {
    m_format = rhs.m_format;
    m_params = rhs.m_params;
    ocb_entries_m.lock();
    rhs.m_values_shadow.ongoing_flag = true;
    ocb_entries_m.unlock();
    if (!rhs.m_values) {
        m_values = nullptr;
        if(rhs.m_values_shadow.m_values){
            this->create_shadow();
            this->copy_from_other_shadow(rhs.m_values_shadow);
            ocb_entries_m.lock();
            rhs.m_values_shadow.ongoing_flag = false;
            ocb_entries_m.unlock();

        }
        return *this;
    }
    if (m_values) {
        *m_values = *rhs.m_values;
        if(rhs.m_values_shadow.m_values){
            this->create_shadow();
            this->copy_from_other_shadow(rhs.m_values_shadow);
            ocb_entries_m.lock();
            rhs.m_values_shadow.ongoing_flag = false;
            ocb_entries_m.unlock();

        }
        return *this;
    }
    m_values = std::make_unique<VecType>(*rhs.m_values);
    if(rhs.m_values_shadow.m_values){
        this->create_shadow();
        this->copy_from_other_shadow(rhs.m_values_shadow);
        ocb_entries_m.lock();
        rhs.m_values_shadow.ongoing_flag = false;
        ocb_entries_m.unlock();
    }
    return *this;
}

// assumes that elements in rhs less than modulus?
template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(std::initializer_list<uint64_t> rhs) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ZERO(0);
    const size_t llen = rhs.size();
    const size_t vlen = m_params->GetRingDimension();
    if (!m_values) {
        VecType temp(vlen);
        temp.SetModulus(m_params->GetModulus());
        PolyImpl<VecType>::SetValues(std::move(temp), m_format);
    }
    for (size_t j = 0; j < vlen; ++j)
        (*m_values)[j] = (j < llen) ? *(rhs.begin() + j) : ZERO;
    return *this;
}

// TODO: template with enable_if int64_t/int32_t
template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(const std::vector<int64_t>& rhs) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ZERO(0);
    m_format = Format::COEFFICIENT;
    const size_t llen{rhs.size()};
    const size_t vlen{m_params->GetRingDimension()};
    const auto& m = m_params->GetModulus();
    if (!m_values) {
        VecType tmp(vlen);
        tmp.SetModulus(m);
        PolyImpl<VecType>::SetValues(std::move(tmp), m_format);
    }
    for (size_t j = 0; j < vlen; ++j) {
        if (j < llen)
            (*m_values)[j] =
                (rhs[j] < 0) ? m - Integer(static_cast<uint64_t>(-rhs[j])) : Integer(static_cast<uint64_t>(rhs[j]));
        else
            (*m_values)[j] = ZERO;
    }
    return *this;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(const std::vector<int32_t>& rhs) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ZERO(0);
    m_format = Format::COEFFICIENT;
    const size_t llen{rhs.size()};
    const size_t vlen{m_params->GetRingDimension()};
    const auto& m = m_params->GetModulus();
    if (!m_values) {
        VecType tmp(vlen);
        tmp.SetModulus(m);
        PolyImpl<VecType>::SetValues(std::move(tmp), m_format);
    }
    for (size_t j = 0; j < vlen; ++j) {
        if (j < llen)
            (*m_values)[j] =
                (rhs[j] < 0) ? m - Integer(static_cast<uint64_t>(-rhs[j])) : Integer(static_cast<uint64_t>(rhs[j]));
        else
            (*m_values)[j] = ZERO;
    }
    return *this;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(std::initializer_list<std::string> rhs) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    const size_t vlen = m_params->GetRingDimension();
    if (!m_values) {
        VecType temp(vlen);
        temp.SetModulus(m_params->GetModulus());
        PolyImpl<VecType>::SetValues(std::move(temp), m_format);
    }
    *m_values = rhs;
    return *this;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator=(uint64_t val) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    m_format = Format::EVALUATION;
    if (!m_values) {
        auto d{m_params->GetRingDimension()};
        const auto& m{m_params->GetModulus()};
        m_values = std::make_unique<VecType>(d, m);
    }
    size_t vlen{m_values->GetLength()};
    Integer ival{val};
    for (size_t i = 0; i < vlen; ++i)
        (*m_values)[i] = ival;
    return *this;
}

template <typename VecType>
void PolyImpl<VecType>::SetValues(const VecType& values, Format format) {
    if (m_params->GetRootOfUnity() == Integer(0))
        OPENFHE_THROW(type_error, "Polynomial has a 0 root of unity");
    if (m_params->GetRingDimension() != values.GetLength() || m_params->GetModulus() != values.GetModulus())
        OPENFHE_THROW(type_error, "Parameter mismatch on SetValues for Polynomial");
    m_format = format;
    m_values = std::make_unique<VecType>(values);
    this->indicate_modified_orig();
}

template <typename VecType>
void PolyImpl<VecType>::SetValues(VecType&& values, Format format) {
    if (m_params->GetRootOfUnity() == Integer(0))
        OPENFHE_THROW(type_error, "Polynomial has a 0 root of unity");
    if (m_params->GetRingDimension() != values.GetLength() || m_params->GetModulus() != values.GetModulus())
        OPENFHE_THROW(type_error, "Parameter mismatch on SetValues for Polynomial");
    m_format = format;
    m_values = std::make_unique<VecType>(std::move(values));
    this->indicate_modified_orig();
}

template <typename VecType>
void PolyImpl<VecType>::SetValuesShadow(const VecType& values, Format format) {
    if (m_params->GetRootOfUnity() == Integer(0))
        OPENFHE_THROW(type_error, "Polynomial has a 0 root of unity");
    if (m_params->GetRingDimension() != values.GetLength() || m_params->GetModulus() != values.GetModulus())
        OPENFHE_THROW(type_error, "Parameter mismatch on SetValues for Polynomial");
    m_format = format;
    m_values = std::make_unique<VecType>(values);
    if(m_values_shadow.shadow_sync_state == SHADOW_NOTEXIST) this->create_shadow();
    //need to modify..
    // *m_values_shadow.m_values = values;
    inc_copy_to_shadow_real((uint64_t)&m_values);
    ::memcpy(m_values_shadow.get_ptr(),&(values.m_data[0]),sizeof(uint64_t)*m_params->GetRingDimension());              
    m_values_shadow.shadow_sync_state = SHADOW_SYNCHED;
    if(compute_flag){
        // std::cout << "ORIGIN --> OCB(SetValuesShadow)" << std::endl;
        std::ofstream file("commandrecord.csv", std::ios::app);
        if (file.is_open()) {
            file << "D, " << "PCIE, " << (uint64_t)m_values_shadow.get_ptr() << ", " << (uint64_t)&m_values->m_data[0] << std::endl;
            file.close();
        }
    }
}

template <typename VecType>
void PolyImpl<VecType>::SetValuesShadow(VecType&& values, Format format) {
    if (m_params->GetRootOfUnity() == Integer(0))
        OPENFHE_THROW(type_error, "Polynomial has a 0 root of unity");
    if (m_params->GetRingDimension() != values.GetLength() || m_params->GetModulus() != values.GetModulus())
        OPENFHE_THROW(type_error, "Parameter mismatch on SetValues for Polynomial");
    m_format = format;
    m_values = std::make_unique<VecType>(values);
    if(m_values_shadow.shadow_sync_state == SHADOW_NOTEXIST) this->create_shadow();
    //need to modify..
    // *m_values_shadow.m_values = std::move(values);
    inc_copy_to_shadow_real((uint64_t)&m_values);
    ::memcpy(m_values_shadow.get_ptr(),&(values.m_data[0]),sizeof(uint64_t)*m_params->GetRingDimension());                
    m_values_shadow.shadow_sync_state = SHADOW_SYNCHED;
    if(compute_flag){
        // std::cout << "ORIGIN --> OCB(SetValuesShadow)" << std::endl;
        std::ofstream file("commandrecord.csv", std::ios::app);
        if (file.is_open()) {
            file << "D, " << "PCIE, " << (uint64_t)m_values_shadow.get_ptr() << ", " << (uint64_t)&m_values->m_data[0] << std::endl;
            file.close();
        }
    }
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Plus(const typename VecType::Integer& element) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    this->copy_from_shadow();
    if (m_format == Format::COEFFICIENT)
        tmp.SetValues((*m_values).ModAddAtIndex(0, element), m_format);
    else
        tmp.SetValues((*m_values).ModAdd(element), m_format);
    inc_compute_not_implemented();
    return tmp;
}

//c++ explicit template specialization!
template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::Plus(const typename NativeVector::Integer& element) const {
    PolyImpl<NativeVector> tmp(m_params, m_format);

    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlusScalar);

    item->poly = (void*)this;
    item->poly2 = (void*)&tmp;
    item->param1 = element.ConvertToInt();
    item->modulus = m_params->GetModulus().m_value;

    work_queue.addWork(item);

    delete item;

    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Minus(const typename VecType::Integer& element) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    this->copy_from_shadow();
    tmp.SetValues((*m_values).ModSub(element), m_format);
    inc_compute_not_implemented();
    return tmp;
}

template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::Minus(const typename NativeVector::Integer& element) const {
    PolyImpl<NativeVector> tmp(m_params, m_format);
    
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_MinusScalar);

    item->poly = (void*)this;
    item->poly2 = (void*)&tmp;
    item->param1 = element.ConvertToInt();
    item->modulus = m_params->GetModulus().m_value;

    work_queue.addWork(item);

    delete item;

    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Times(const typename VecType::Integer& element) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    this->copy_from_shadow();
    tmp.SetValues((*m_values).ModMul(element), m_format);
    return tmp;
}

//c++ explicit template specialization!
template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::Times(const typename NativeVector::Integer& element) const {
    PolyImpl<NativeVector> tmp(m_params, m_format);    
    
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlainModMulScalar);

    item->poly = (void*)this;
    item->poly2 = (void*)&tmp;
    item->param1 = element.m_value;
    item->modulus = m_params->GetModulus().m_value;

    work_queue.addWork(item);
    
    delete item;

    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Times(NativeInteger::SignedNativeInt element) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    Integer q{m_params->GetModulus()};
    this->copy_from_shadow();
    if (element < 0) {
        Integer elementReduced{NativeInteger::Integer(-element)};
        if (elementReduced > q)
            elementReduced.ModEq(q);
        tmp.SetValues((*m_values).ModMul(q - elementReduced), m_format);
        // tmp.SetValuesShadow((*m_values_shadow.m_values).ModMul(q - elementReduced), m_format);
    }
    else {
        Integer elementReduced{NativeInteger::Integer(element)};
        if (elementReduced > q)
            elementReduced.ModEq(q);
        tmp.SetValues((*m_values).ModMul(elementReduced), m_format);
        // tmp.SetValuesShadow((*m_values_shadow.m_values).ModMul(elementReduced), m_format);
    }
    return tmp;
}

//c++ explicit template specialization!
template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::Times(NativeInteger::SignedNativeInt element) const {
    PolyImpl<NativeVector> tmp(m_params, m_format);

    Integer q{m_params->GetModulus()};
    
    if (element < 0) {
        Integer elementReduced{NativeInteger::Integer(-element)};
        if (elementReduced > q)
            elementReduced.ModEq(q);
        
        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlainModMulScalar);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->param1 = -element;
        item->modulus = m_params->GetModulus().m_value;

        work_queue.addWork(item);
        
        delete item;

    }
    else {
        Integer elementReduced{NativeInteger::Integer(element)};
        if (elementReduced > q)
            elementReduced.ModEq(q);

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlainModMulScalar);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->param1 = elementReduced.m_value;
        item->modulus = m_params->GetModulus().m_value;

        work_queue.addWork(item);
        
        delete item;
    }
    return tmp;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator*=(const Integer& element) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    this->copy_from_shadow();
    m_values->ModMulEq(element);
    this->indicate_modified_orig();
    return *this;
}

template <>
PolyImpl<NativeVector>& PolyImpl<NativeVector>::operator*=(const Integer& element) { 
           
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlainModMulEqScalar);

    item->poly = this;
    item->param1 = element.m_value;
    item->modulus = m_params->GetModulus().m_value;

    work_queue.addWork(item);

    delete item;

    return *this;
}


template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Minus(const PolyImpl& rhs) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    this->copy_from_shadow(); 
    rhs.copy_from_shadow(); 
    tmp.SetValues((*m_values).ModSub(*rhs.m_values), m_format);
    inc_compute_not_implemented();
    return tmp;
}

template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::Minus(const PolyImpl& rhs) const {
    PolyImpl<NativeVector> tmp(m_params, m_format);
    
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_Minus);

    item->poly = (void*)this;
    item->poly2 = (void*)&tmp;
    item->poly3 = (void*)&rhs;
    item->modulus = m_params->GetModulus().m_value;

    work_queue.addWork(item);
    
    delete item;

    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::MultiplyAndRound(const typename VecType::Integer& p,
                                                      const typename VecType::Integer& q) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    tmp.SetValues((*m_values).MultiplyAndRound(p, q), m_format);
    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::DivideAndRound(const typename VecType::Integer& q) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    tmp.SetValues((*m_values).DivideAndRound(q), m_format);
    return tmp;
}

// TODO: this will return vec of 0s for BigIntegers
template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Negate() const {
    //  UnitTestBFVrnsCRTOperations.cpp line 316 throws with this uncommented
    //    if (m_format != Format::EVALUATION)
    //        OPENFHE_THROW(not_implemented_error, "Negate for PolyImpl is supported only in Format::EVALUATION format.\n");
    return PolyImpl<VecType>(m_params, m_format, true) -= *this;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator+=(const PolyImpl& element) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    this->copy_from_shadow();
    element.copy_from_shadow();
    if (!m_values)
        m_values = std::make_unique<VecType>(m_params->GetRingDimension(), m_params->GetModulus());
    m_values->ModAddEq(*element.m_values);
    this->indicate_modified_orig();
    return *this;
}

template <>
PolyImpl<NativeVector>& PolyImpl<NativeVector>::operator+=(const PolyImpl& element) {
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_PlusInPlace);

    item->poly = (void*)this;
    item->poly2 = (void*)&element;
    work_queue.addWork(item);
    item->modulus = m_params->GetModulus().m_value;
    
    delete item;
   
    return *this;
}

template <typename VecType>
PolyImpl<VecType>& PolyImpl<VecType>::operator-=(const PolyImpl& element) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    this->copy_from_shadow();
    element.copy_from_shadow();
    if (!m_values)
        m_values = std::make_unique<VecType>(m_params->GetRingDimension(), m_params->GetModulus());
    // m_values->ModSubEq(*element.m_values);
    m_values->ModSubEq(*element.m_values);
    this->indicate_modified_orig();
    inc_compute_not_implemented();
    return *this;
}

template <>
PolyImpl<NativeVector>& PolyImpl<NativeVector>::operator-=(const PolyImpl& element) {
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_MinusInPlace);

    item->poly = (void*)this;
    item->poly2 = (void*)&element;
    item->modulus = m_params->GetModulus().m_value;
    work_queue.addWork(item);
    
    delete item;

    return *this;
}

template <typename VecType>
void PolyImpl<VecType>::AddILElementOne() {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ONE(1);
    usint vlen{m_params->GetRingDimension()};
    const auto& m{m_params->GetModulus()};
    for (usint i = 0; i < vlen; ++i)
        (*m_values)[i].ModAddFastEq(ONE, m);
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::AutomorphismTransform(uint32_t k) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    uint32_t n{m_params->GetRingDimension()};
    uint32_t m{m_params->GetCyclotomicOrder()};
    bool bp{n == (m >> 1)};
    bool bf{m_format == Format::EVALUATION};

    // if (!bf && !bp)
    if (!bp)
        OPENFHE_THROW(not_implemented_error, "Automorphism Poly Format not EVALUATION or not power-of-two");
    /*
    // TODO: is this branch ever called?

    PolyImpl<VecType> result(m_params, m_format, true);
    if (bf && !bp) {
        // TODO: Add a test based on the inverse totient hash table?

        // All automorphism operations are performed for k coprime to m
        auto totientList = GetTotientList(m);

        // This step can be eliminated by using a hash table that looks up the
        // ring index (between 0 and n - 1) based on the totient index (between 0 and m - 1)
        VecType expanded(m, m_params->GetModulus());
        for (uint32_t i = 0; i < n; ++i)
            expanded[totientList[i]] = (*m_values)[i];

        for (uint32_t i = 0; i < n; ++i) {
            // determines which power of primitive root unity we should switch to
            (*result.m_values)[i] = expanded[totientList[i] * k % m];
        }
        return result;
    }
*/
    if (k % 2 == 0)
        OPENFHE_THROW(math_error, "Automorphism index not odd\n");

    PolyImpl<VecType> result(m_params, m_format, true);
    uint32_t logm{lbcrypto::GetMSB(m) - 1};
    uint32_t logn{logm - 1};
    uint32_t mask{(uint32_t(1) << logn) - 1};

    if (bf) {
        for (uint32_t j{0}, jk{k}; j < n; ++j, jk += (2 * k)) {
            auto&& jrev{lbcrypto::ReverseBits(j, logn)};
            auto&& idxrev{lbcrypto::ReverseBits((jk >> 1) & mask, logn)};
            (*result.m_values)[jrev] = (*m_values)[idxrev];
        }
        return result;
    }

    auto q{m_params->GetModulus()};
    for (uint32_t j{0}, jk{0}; j < n; ++j, jk += k)
        (*result.m_values)[jk & mask] = ((jk >> logn) & 0x1) ? q - (*m_values)[j] : (*m_values)[j];
    return result;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::AutomorphismTransform(uint32_t k, const std::vector<uint32_t>& precomp) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    if ((m_format != Format::EVALUATION) || (m_params->GetRingDimension() != (m_params->GetCyclotomicOrder() >> 1)))
        OPENFHE_THROW(not_implemented_error, "Automorphism Poly Format not EVALUATION or not power-of-two");
    if (k % 2 == 0)
        OPENFHE_THROW(math_error, "Automorphism index not odd\n");
    PolyImpl<VecType> tmp(m_params, m_format, true);
    uint32_t n = m_params->GetRingDimension();
    this->copy_from_shadow();
    tmp.copy_from_shadow();
    for (uint32_t j = 0; j < n; ++j)
        (*tmp.m_values)[j] = (*m_values)[precomp[j]];
    tmp.indicate_modified_orig();
    return tmp;
}


template <>
PolyImpl<NativeVector> PolyImpl<NativeVector>::AutomorphismTransform(uint32_t k, const std::vector<uint32_t>& precomp) const {
    if ((m_format != Format::EVALUATION) || (m_params->GetRingDimension() != (m_params->GetCyclotomicOrder() >> 1)))
        OPENFHE_THROW(not_implemented_error, "Automorphism Poly Format not EVALUATION or not power-of-two");
    if (k % 2 == 0)
        OPENFHE_THROW(math_error, "Automorphism index not odd\n");
    PolyImpl<NativeVector> tmp(m_params, m_format, true);
    uint32_t n = m_params->GetRingDimension();
    
    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_AutomorphismTransform);

    item->poly = (void*)this;
    item->poly2 = (void*)&tmp;
    item->ptr32_1 = (uint32_t*)&precomp[0];
    item->param1 = n;
    item->modulus = m_params->GetModulus().m_value;
    work_queue.addWork(item);
    
    delete item;

    return tmp;
}


template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::MultiplicativeInverse() const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    tmp.SetValues((*m_values).ModInverse(), m_format);
    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::ModByTwo() const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    tmp.SetValues((*m_values).ModByTwo(), m_format);
    return tmp;
}

template <typename VecType>
PolyImpl<VecType> PolyImpl<VecType>::Mod(const Integer& modulus) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    PolyImpl<VecType> tmp(m_params, m_format);
    tmp.SetValues((*m_values).Mod(modulus), m_format);
    return tmp;
}

template <typename VecType>
void PolyImpl<VecType>::SwitchModulus(const Integer& modulus, const Integer& rootOfUnity, const Integer& modulusArb,
                                      const Integer& rootOfUnityArb) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    this->copy_from_shadow();
    if (m_values != nullptr) {
        // m_values->SwitchModulus(modulus);
        m_values->SwitchModulus(modulus);
        this->indicate_modified_orig();
        auto c{m_params->GetCyclotomicOrder()};
        m_params = std::make_shared<PolyImpl::Params>(c, modulus, rootOfUnity, modulusArb, rootOfUnityArb);
    }
}

template <>
void PolyImpl<NativeVector>::SwitchModulus(const Integer& modulus, const Integer& rootOfUnity, const Integer& modulusArb,
                                      const Integer& rootOfUnityArb) {
    auto size{m_values_shadow.m_values->size()};
    auto halfQ{m_params->GetModulus().m_value >> 1};
    auto om{m_params->GetModulus().m_value};
    if (m_values != nullptr) { 
        m_values->NativeVectorT::SetModulus(modulus);
    }
    auto nm{modulus.m_value};

    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_SwitchModulus);

    item->poly = (void*)this;
    item->param1 = size;
    item->param2 = halfQ;
    item->param3 = om;
    item->param4 = nm;
    item->modulus = m_params->GetModulus().m_value;
    work_queue.addWork(item);
    
    delete item;

    auto c{m_params->GetCyclotomicOrder()};
    m_params = std::make_shared<PolyImpl::Params>(c, modulus, rootOfUnity, modulusArb, rootOfUnityArb);
}

template <>
void PolyImpl<NativeVector>::bconv_pipe(const Integer& modulus, const Integer& rootOfUnity, const Integer& modulusArb,
                                      const Integer& rootOfUnityArb,
                                      const Integer& element,
                                      PolyImpl& element2
                                      ) {
    auto size{m_values_shadow.m_values->size()};
    auto halfQ{m_params->GetModulus().m_value >> 1};
    auto om{m_params->GetModulus().m_value};
    if (m_values != nullptr) { 
        m_values->NativeVectorT::SetModulus(modulus);
    }
    auto nm{modulus.m_value};

    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_BCONV_PIPE);

    // switch
    item->poly = (void*)&element2;
    item->param1 = size;
    item->param2 = halfQ;
    item->param3 = om;
    item->param4 = nm;
    // add_poly
    item->poly2 = (void*)this;
    // mult_scalar
    item->param5 = element.m_value;

    work_queue.addWork(item);
    
    delete item; // Clean up
}

template <typename VecType>
void PolyImpl<VecType>::SwitchFormat() {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    const auto& co{m_params->GetCyclotomicOrder()};
    const auto& rd{m_params->GetRingDimension()};
    const auto& ru{m_params->GetRootOfUnity()};

    if (rd != (co >> 1)) {
        PolyImpl<VecType>::ArbitrarySwitchFormat();
        return;
    }

    this->copy_from_shadow();
    if (!m_values)
        OPENFHE_THROW(not_available_error, "Poly switch format to empty values");

    if (m_format != Format::COEFFICIENT) {
        m_format = Format::COEFFICIENT;
        ChineseRemainderTransformFTT<VecType>().InverseTransformFromBitReverseInPlace(ru, co, &(*m_values));
        this->indicate_modified_orig();
        return;
    }
    m_format = Format::EVALUATION;
    // ChineseRemainderTransformFTT<VecType>().ForwardTransformToBitReverseInPlace(ru, co, &(*m_values));
    ChineseRemainderTransformFTT<VecType>().ForwardTransformToBitReverseInPlace(ru, co, &(*m_values));
    this->indicate_modified_orig();
}

template <>
void PolyImpl<NativeVector>::SwitchFormat() {
    const auto& co{m_params->GetCyclotomicOrder()};
    const auto& rd{m_params->GetRingDimension()};
    const auto& ru{m_params->GetRootOfUnity()};

    if (rd != (co >> 1)) {
        PolyImpl<NativeVector>::ArbitrarySwitchFormat();
        return;
    }

    if (m_format != Format::COEFFICIENT) {   
        m_format = Format::COEFFICIENT;

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_SwitchFormatInverseTransform);

        item->poly = (void*)this;
        item->param1 = co;
        item->param2 = ru.ConvertToInt();
        item->param3 = m_params->GetModulus().m_value;
        item->modulus = m_params->GetModulus().m_value;
        work_queue.addWork(item);

        delete item;

        return;
    }

    m_format = Format::EVALUATION;

    CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_SwitchFormatForwardTransform);

    item->poly = (void*)this;
    item->param1 = co;
    item->param2 = ru.ConvertToInt();
    item->param3 = m_params->GetModulus().m_value;
    item->modulus = m_params->GetModulus().m_value;
    work_queue.addWork(item);
    
    delete item;
}

template <typename VecType>
void PolyImpl<VecType>::ArbitrarySwitchFormat() {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    if (m_values == nullptr)
        OPENFHE_THROW(not_available_error, "Poly switch format to empty values");
    const auto& lr = m_params->GetRootOfUnity();
    const auto& bm = m_params->GetBigModulus();
    const auto& br = m_params->GetBigRootOfUnity();
    const auto& co = m_params->GetCyclotomicOrder();
    if (m_format == Format::COEFFICIENT) {
        m_format = Format::EVALUATION;
        auto&& v = ChineseRemainderTransformArb<VecType>().ForwardTransform(*m_values, lr, bm, br, co);
        m_values = std::make_unique<VecType>(v);
    }
    else {
        m_format = Format::COEFFICIENT;
        auto&& v = ChineseRemainderTransformArb<VecType>().InverseTransform(*m_values, lr, bm, br, co);
        m_values = std::make_unique<VecType>(v);
    }
}

template <typename VecType>
std::ostream& operator<<(std::ostream& os, const PolyImpl<VecType>& p) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    if (p.m_values != nullptr) {
        os << *(p.m_values);
        os << " mod:" << (p.m_values)->GetModulus() << std::endl;
    }
    if (p.m_params.get() != nullptr)
        os << " rootOfUnity: " << p.GetRootOfUnity() << std::endl;
    else
        os << " something's odd: null m_params?!" << std::endl;
    os << std::endl;
    return os;
}

template <typename VecType>
void PolyImpl<VecType>::MakeSparse(uint32_t wFactor) {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ZERO(0);
    if (m_values != nullptr) {
        uint32_t vlen{m_params->GetRingDimension()};
        for (uint32_t i = 0; i < vlen; ++i) {
            if (i % wFactor != 0)
                (*m_values)[i] = ZERO;
        }
    }
}

template <typename VecType>
bool PolyImpl<VecType>::InverseExists() const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    static const Integer ZERO(0);
    usint vlen{m_params->GetRingDimension()};
    for (usint i = 0; i < vlen; ++i) {
        if ((*m_values)[i] == ZERO)
            return false;
    }
    return true;
}

template <typename VecType>
double PolyImpl<VecType>::Norm() const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    usint vlen{m_params->GetRingDimension()};
    const auto& q{m_params->GetModulus()};
    const auto& half{q >> 1};
    Integer maxVal{}, minVal{q};
    for (usint i = 0; i < vlen; i++) {
        auto& val = (*m_values)[i];
        if (val > half)
            minVal = val < minVal ? val : minVal;
        else
            maxVal = val > maxVal ? val : maxVal;
    }
    minVal = q - minVal;
    return (minVal > maxVal ? minVal : maxVal).ConvertToDouble();
}

// Write vector x(current value of the PolyImpl object) as \sum\limits{ i = 0
// }^{\lfloor{ \log q / base } \rfloor} {(base^i u_i)} and return the vector of{
// u_0, u_1,...,u_{ \lfloor{ \log q / base } \rfloor } } \in R_base^{ \lceil{
// \log q / base } \rceil }; used as a subroutine in the relinearization
// procedure baseBits is the number of bits in the base, i.e., base = 2^baseBits

// TODO: optimize this
template <typename VecType>
std::vector<PolyImpl<VecType>> PolyImpl<VecType>::BaseDecompose(usint baseBits, bool evalModeAnswer) const {
    usint nBits = m_params->GetModulus().GetLengthForBase(2);

    usint nWindows = nBits / baseBits;
    if (nBits % baseBits > 0)
        nWindows++;

    PolyImpl<VecType> xDigit(m_params);

    std::vector<PolyImpl<VecType>> result;
    result.reserve(nWindows);

    PolyImpl<VecType> x(*this);
    x.SetFormat(Format::COEFFICIENT);

    // TP: x is same for BACKEND 2 and 6
    for (usint i = 0; i < nWindows; ++i) {
        xDigit.SetValues(x.GetValues().GetDigitAtIndexForBase(i + 1, 1 << baseBits), x.GetFormat());

        // TP: xDigit is all zeros for BACKEND=6, but not for BACKEND-2
        // *********************************************************
        if (evalModeAnswer)
            xDigit.SwitchFormat();
        result.push_back(xDigit);
    }
    return result;
}

// Generate a vector of PolyImpl's as {x, base*x, base^2*x, ..., base^{\lfloor
// {\log q/base} \rfloor}*x, where x is the current PolyImpl object; used as a
// subroutine in the relinearization procedure to get powers of a certain "base"
// for the secret key element baseBits is the number of bits in the base, i.e.,
// base = 2^baseBits

template <typename VecType>
std::vector<PolyImpl<VecType>> PolyImpl<VecType>::PowersOfBase(usint baseBits) const {
    static const Integer TWO(2);
    const auto& m{m_params->GetModulus()};
    usint nBits{m.GetLengthForBase(2)};
    usint nWindows{nBits / baseBits};
    if (nBits % baseBits > 0)
        ++nWindows;
    std::vector<PolyImpl<VecType>> result(nWindows);
    Integer shift{0}, bbits{baseBits};
    for (usint i = 0; i < nWindows; ++i, shift += bbits)
        result[i] = (*this) * TWO.ModExp(shift, m);
    return result;
}

template <typename VecType>
typename PolyImpl<VecType>::PolyNative PolyImpl<VecType>::DecryptionCRTInterpolate(PlaintextModulus ptm) const {
    OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
    const PolyImpl<VecType> smaller(PolyImpl<VecType>::Mod(ptm));
    smaller.copy_from_shadow();
    usint vlen{m_params->GetRingDimension()};
    auto c{m_params->GetCyclotomicOrder()};
    auto params{std::make_shared<ILNativeParams>(c, NativeInteger(ptm), 1)};
    typename PolyImpl<VecType>::PolyNative tmp(params, m_format, true);
    for (usint i = 0; i < vlen; ++i)
        tmp[i] = NativeInteger((*smaller.m_values)[i]);
    return tmp;
}

template <>
inline PolyImpl<NativeVector> PolyImpl<NativeVector>::ToNativePoly() const {
    return *this;
}

/* Below is the 'consumer part' that handles tasks that were put into the working queue
    in the previous unit operation code part.
    Always check whether a task is in the working queue, and if it is,
    take it out of the queue one by one and perform the operation.
    The implementation of the unit operation is in the lower part and poly-impl.cpp.
*/
#include <chrono>

void consumer(WorkQueue& queue) {
    std::vector<CustomTaskItem*> items;
    
    std::cout << "consumer thread: started" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();

    bool success = queue.getWork(items) ;

    // Logger logger("log.csv");
    // logger.logFunctionStart("exampleFunction");
    // logger.logFunctionEnd();

    std::set<uint64_t> ntt_mod_set;
    std::set<uint64_t> intt_mod_set;
    std::deque<uint64_t> ntt_mod_deque;
    std::deque<uint64_t> intt_mod_deque;
    
    while(success)    
    {
        auto start_work = std::chrono::high_resolution_clock::now();    

        for(auto item:items)
        {
            // Process the item

            PolyImpl<NativeVector>* poly = (PolyImpl<NativeVector>*)item->poly;
            PolyImpl<NativeVector>* poly2 = (PolyImpl<NativeVector>*)item->poly2;
            PolyImpl<NativeVector>* poly3 = (PolyImpl<NativeVector>*)item->poly3;

            switch(item->task_type) {
                case TASK_TYPE_PlainModMulEqScalar: {
                        // if(compute_flag) std::cout << "TASK_TYPE_PlainModMulEqScalar" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                    
                        PlainModMulEqScalar(
                            poly->m_values_shadow.get_ptr(),
                            poly->m_params->GetModulus().m_value,
                            item->param1,
                            poly->m_values_shadow.m_values->size()
                        );    

                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                
                        inc_compute_implemented();
                        inc_mult();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Mult, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                << "0" << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_PlusScalar: {
                        // if(compute_flag) std::cout << "TASK_TYPE_PlusScalar" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->create_shadow();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        
                        uint64_t* op1       = poly->m_values_shadow.get_ptr();
                        uint64_t* result    = poly2->m_values_shadow.get_ptr();
                        uint64_t modulus    = poly->m_params->GetModulus().m_value;

                        for(size_t i=0; i<poly->m_values_shadow.m_values->size(); i++){
                            uint64_t x = op1[i];
                            x += item->param1;
                            result[i] = x >= modulus ? x - modulus : x;
                        }

                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        
                        inc_compute_implemented();
                        inc_add();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Add, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_MinusScalar: {
                        // if(compute_flag) std::cout << "TASK_TYPE_MinusScalar" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->create_shadow();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        
                        uint64_t* op1       = poly->m_values_shadow.get_ptr();
                        uint64_t* result    = poly2->m_values_shadow.get_ptr();
                        uint64_t modulus    = poly->m_params->GetModulus().m_value;

                        for(size_t i=0; i<poly->m_values_shadow.m_values->size(); i++){
                            uint64_t x = op1[i];
                            unsigned long long temp = x - item->param1;
                            std::int64_t borrow = (temp > x);
                            result[i] = static_cast<std::uint64_t>(temp) + (modulus & static_cast<std::uint64_t>(-borrow));
                        }

                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        
                        inc_compute_implemented();
                        inc_sub();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Sub, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_PlainModMulScalar:   {
                        // if(compute_flag) std::cout << "TASK_TYPE_PlainModMulScalar" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->create_shadow();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();

                        PlainModMulScalar(
                            poly2->m_values_shadow.get_ptr(),
                            poly->m_values_shadow.get_ptr(),
                            poly->m_params->GetModulus().m_value,
                            item->param1,
                            poly->m_values_shadow.m_values->size()
                        );
                        
                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_mult();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Mult, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_Minus: {
                        // if(compute_flag) std::cout << "TASK_TYPE_Minus" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly->m_values_shadow.ongoing_flag = true;
                        poly3->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->create_shadow();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow(); 
                        // if(compute_flag) std::cout << "poly3" << std::endl;
                        poly3->copy_to_shadow(); 

                        uint64_t* op1       = poly->m_values_shadow.get_ptr();
                        uint64_t* op2       = poly3->m_values_shadow.get_ptr();
                        uint64_t* result    = poly2->m_values_shadow.get_ptr();
                        uint64_t modulus    = poly->m_params->GetModulus().m_value;

                        for(size_t i=0; i<poly->m_values_shadow.m_values->size(); i++){
                            uint64_t x = op1[i];
                            unsigned long long temp = x - op2[i];
                            std::int64_t borrow = (temp > x);
                            result[i] = static_cast<std::uint64_t>(temp) + (modulus & static_cast<std::uint64_t>(-borrow));
                        }

                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly->m_values_shadow.ongoing_flag = false;
                        poly3->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_sub();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Sub, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly->m_values_shadow.get_ptr() << ", " << (uint64_t)poly3->m_values_shadow.get_ptr() << std::endl;
                                file.close();
                            }
                        }   
                    }   
                    break;
                case TASK_TYPE_PlusInPlace: {
                        // if(compute_flag) std::cout << "TASK_TYPE_PlusInPlace" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        poly2->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow();

                        uint64_t* op1       = poly->m_values_shadow.get_ptr();
                        const uint64_t* op2 = poly2->m_values_shadow.get_ptr();
                        
                        uint64_t modulus = poly->m_params->GetModulus().m_value;

                        for(size_t i=0; i<poly->m_values_shadow.m_values->size(); i++){
                            uint64_t sum = op1[i] + op2[i];
                            op1[i] = sum >= modulus ? sum - modulus : sum;
                        }
                        
                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        poly2->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();

                        inc_compute_implemented();
                        inc_add();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Add, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly2->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_MinusInPlace: {
                        // if(compute_flag) std::cout << "TASK_TYPE_MinusInPlace" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        poly2->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow();

                        uint64_t* op1       = poly->m_values_shadow.get_ptr();
                        const uint64_t* op2 = poly2->m_values_shadow.get_ptr();
                        uint64_t modulus = poly->m_params->GetModulus().m_value;

                        unsigned long long temp_result;
                        for(size_t i=0; i<poly->m_values_shadow.m_values->size(); i++){
                            temp_result = op1[i] - op2[i];
                            std::int64_t borrow = static_cast<unsigned char>(op2[i] > op1[i]);
                            op1[i] = temp_result + (modulus & static_cast<std::uint64_t>(-borrow));
                        }

                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        poly2->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();

                        inc_compute_implemented();
                        inc_sub();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Sub, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly2->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_AutomorphismTransform: {
                        // if(compute_flag) std::cout << "TASK_TYPE_AutomorphismTransform" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        poly2->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow(); //////////////
                        
                        for (uint32_t j = 0; j < item->param1; ++j)
                            (*poly2->m_values_shadow.m_values)[j] = (*poly->m_values_shadow.m_values)[item->ptr32_1[j]];

                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        poly2->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();

                        inc_compute_implemented();
                        inc_auto();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Auto, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_SwitchModulus: {
                        // if(compute_flag) std::cout << "TASK_TYPE_SwitchModulus" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();

                        uint64_t* data = poly->m_values_shadow.get_ptr();

                        uint64_t size = item->param1;
                        uint64_t halfQ = item->param2;
                        uint64_t om = item->param3;
                        uint64_t nm = item->param4;


                        if (nm > om) {
                            auto diff{nm - om};
                            for (size_t i = 0; i < size; ++i) {
                                auto& v = data[i];
                                if (v > halfQ)
                                    v = v + diff;
                            }
                            inc_add();
                            if(compute_flag){
                                std::ofstream file("commandrecord.csv", std::ios::app);
                                if (file.is_open()) {
                                    file << "C, " << "Add, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << "0" << ", " << "0" << std::endl;
                                        file.close();
                                }
                            }
                        }
                        else {
                            auto diff{nm - (om % nm)};
                            for (size_t i = 0; i < size; ++i) {
                                auto& v = data[i];
                                if (v > halfQ)
                                    v = v + diff;
                                if (v >= nm)
                                    v = v % nm;
                            }
                            inc_mult();
                            if(compute_flag){
                                std::ofstream file("commandrecord.csv", std::ios::app);
                                if (file.is_open()) {
                                    file << "C, " << "Mult, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << "0" << ", " << "0" << std::endl;
                                    file.close();
                                }
                            }
                        }

                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();

                        inc_compute_implemented();
                    }
                    break;
                case TASK_TYPE_SwitchFormatInverseTransform: {
                        // if(compute_flag) std::cout << "TASK_TYPE_SwitchFormatInverseTransform" << std::endl;
                        uint64_t co = item->param1;
                        uint64_t ru = item->param2;
                        uint64_t modulus = item->param3;
                        /* We basically calculate the data transfer of the root table
                            because custom_task.h allows us to operate operations with the same modulus(p) at the same time.
                            If the modulus previously calculated is different, data transfer to the root table occurs.*/
                        if(intt_mod_set.find(modulus) == intt_mod_set.end()){
                            if(intt_mod_set.size() > IROOT_ENTRIES_NUM){
                                intt_mod_set.erase(intt_mod_deque.back());
                                intt_mod_deque.pop_back();
                            }
                            intt_mod_deque.push_front(modulus);
                            intt_mod_set.insert(modulus);
                            inc_copy_from_inv_root_shadow_hbm_real();
                        }

                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        ChineseRemainderTransformFTT<NativeVector>().InverseTransformFromBitReverseInPlace(NativeInteger::Integer(ru), co, poly->m_values_shadow.get_ptr(),poly->m_values_shadow.m_values->size(),modulus);
                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_intt();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "INTT, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << "0" << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_SwitchFormatForwardTransform: {
                        // if(compute_flag) std::cout << "TASK_TYPE_SwitchFormatForwardTransform" << std::endl;
                        uint64_t co = item->param1;
                        uint64_t ru = item->param2;
                        uint64_t modulus = item->param3;
                        /* We basically calculate the data transfer of the root table
                            because custom_task.h allows us to operate operations with the same modulus(p) at the same time.
                            If the modulus previously calculated is different, data transfer to the root table occurs.*/
                        if(ntt_mod_set.find(modulus) == ntt_mod_set.end()){
                            if(ntt_mod_set.size() > ROOT_ENTRIES_NUM){
                                ntt_mod_set.erase(ntt_mod_deque.back());
                                ntt_mod_deque.pop_back();
                            }
                            ntt_mod_deque.push_front(modulus);
                            ntt_mod_set.insert(modulus);
                            inc_copy_from_root_shadow_hbm_real();
                        }

                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        ChineseRemainderTransformFTT<NativeVector>().ForwardTransformToBitReverseInPlace(NativeInteger::Integer(ru), co, poly->m_values_shadow.get_ptr(),poly->m_values_shadow.m_values->size(),modulus);
                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_ntt();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "NTT, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << "0" << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_Plus: {
                        // if(compute_flag) std::cout << "TASK_TYPE_Plus" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly3->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow(); /////////////////////
                        // if(compute_flag) std::cout << "poly3" << std::endl;
                        poly3->copy_to_shadow();

                        uint64_t* op1       = poly2->m_values_shadow.get_ptr();
                        const uint64_t* op2 = poly3->m_values_shadow.get_ptr();

                        uint64_t modulus = poly->m_params->GetModulus().ConvertToInt();

                        for(size_t i=0; i<poly2->m_values_shadow.m_values->size(); i++){
                            uint64_t sum = op1[i] + op2[i];
                            op1[i] = sum >= modulus ? sum - modulus : sum;
                        }

                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly3->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_add();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Add, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                    << (uint64_t)poly3->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_Times: 
                case TASK_TYPE_TimesNoCheck: {
                        // if(compute_flag) std::cout << "TASK_TYPE_TimesNoCheck" << std::endl;
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = true;
                        poly3->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow();
                        // if(compute_flag) std::cout << "poly3" << std::endl;
                        poly3->copy_to_shadow();

                        uint64_t* op1 = poly2->m_values_shadow.get_ptr();
                        const uint64_t* op2 = poly3->m_values_shadow.get_ptr();

                        // check_evk_map((uint64_t)&(poly3->m_values));

                        PlainModMul(
                            op1,
                            op2,
                            poly->m_params->GetModulus().ConvertToInt(),
                            poly3->m_values_shadow.m_values->size()
                        );
                        
                        poly2->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly2->m_values_shadow.ongoing_flag = false;
                        poly3->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_mult();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Mult, " << (uint64_t)poly2->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly3->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }         
                    }
                    break;
                case TASK_TYPE_TimesInPlace: {
                        // if(compute_flag) std::cout << "TASK_TYPE_TimesInPlace" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        poly2->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow();
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow();

                        uint64_t* op1 = poly->m_values_shadow.get_ptr();
                        const uint64_t* op2 = poly2->m_values_shadow.get_ptr();

                        // check_evk_map((uint64_t)&(poly2->m_values));

                        PlainModMul(
                            op1,
                            op2,
                            poly->m_params->GetModulus().ConvertToInt(),
                            poly2->m_values_shadow.m_values->size()
                        );
                        
                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        poly2->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();
                        inc_compute_implemented();
                        inc_mult();
                        if(compute_flag){
                            std::ofstream file("commandrecord.csv", std::ios::app);
                            if (file.is_open()) {
                                file << "C, " << "Mult, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                << (uint64_t)poly2->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                file.close();
                            }
                        }
                    }
                    break;
                case TASK_TYPE_BCONV_PIPE: {
                        // if(compute_flag) std::cout << "TASK_TYPE_BCONV_PIPE" << std::endl;
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = true;
                        poly2->m_values_shadow.ongoing_flag = true;
                        ocb_entries_m.unlock();
                        // if(compute_flag) std::cout << "poly" << std::endl;
                        poly->copy_to_shadow_(); /////////////
                        // if(compute_flag) std::cout << "poly2" << std::endl;
                        poly2->copy_to_shadow();
                        
                        uint64_t* op1  = poly->m_values_shadow.get_ptr();
                        const uint64_t* data = poly2->m_values_shadow.get_ptr();

                        uint64_t size = item->param1;
                        uint64_t halfQ = item->param2;
                        uint64_t om = item->param3;
                        uint64_t nm = item->param4;
                        uint64_t mult_scalar = item->param5;

                        uint64_t modulus = poly->m_params->GetModulus().m_value;

                        uint64_t numerator[3]{ 0, 0, 1 };
                        uint64_t quotient[3]{ 0, 0, 0 };
                        divide_uint192_inplace(numerator, nm, quotient);
                        uint64_t pq0 = quotient[0];
                        uint64_t pq1 = quotient[1];

                        if (nm > om) {
                            auto diff{nm - om};
                            for (size_t i = 0; i < size; ++i) {
                                uint64_t v[2];
                                v[0] = data[i];
                                v[1] = data[i];
                                if (v[0] > halfQ)
                                    v[0] = v[0] + diff;
                                PlainModMulEqScalar_one(
                                    &v[0],
                                    nm,
                                    mult_scalar,
                                    pq0,
                                    pq1
                                );    
                                uint64_t sum = op1[i] + v[0];
                                op1[i] = sum >= modulus ? sum - modulus : sum;
                            }
                            inc_bconv_up();
                            if(compute_flag){
                                std::ofstream file("commandrecord.csv", std::ios::app);
                                if (file.is_open()) {
                                    file << "C, " << "Bconvup, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << (uint64_t)poly2->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                    file.close();
                                }
                            }
                        }
                        else {
                            auto diff{nm - (om % nm)};
                            for (size_t i = 0; i < size; ++i) {
                                uint64_t v[2];
                                v[0] = data[i];
                                v[1] = data[i];
                                if (v[0] > halfQ)
                                    v[0] = v[0] + diff;
                                if (v[0] >= nm)
                                    v[0] = v[0] % nm;
                                PlainModMulEqScalar_one(
                                    &v[0],
                                    nm,
                                    mult_scalar,
                                    pq0,
                                    pq1
                                );    
                                uint64_t sum = op1[i] + v[0];
                                op1[i] = sum >= modulus ? sum - modulus : sum;
                            }
                            inc_bconv_down();
                            if(compute_flag){
                                std::ofstream file("commandrecord.csv", std::ios::app);
                                if (file.is_open()) {
                                    file << "C, " << "Bconvdown, " << (uint64_t)poly->m_values_shadow.get_ptr() << ", "
                                    << (uint64_t)poly2->m_values_shadow.get_ptr() << ", " << "0" << std::endl;
                                    file.close();
                                }
                            }
                        }
                        
                        poly->indicate_modified_shadow();
                        ocb_entries_m.lock();
                        poly->m_values_shadow.ongoing_flag = false;
                        poly2->m_values_shadow.ongoing_flag = false;
                        ocb_entries_m.unlock();

                        inc_compute_implemented(); 
                    }
                    break;
                default:
                    break;
            }

            // std::cout << item->task_type << " ";
            // Mark as processed and notify the producer
            queue.workProcessed(item);
        }

        auto end_work = std::chrono::high_resolution_clock::now();
        elapsed_work += end_work - start_work;        

        auto start_getwork = std::chrono::high_resolution_clock::now();
        items.clear();
        success = queue.getWork(items) ;        
        auto end_getwork = std::chrono::high_resolution_clock::now();
        elapsed_getwork += end_getwork - start_getwork;
        if(!success) break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "consumer thread: queue.getWork returned False" << std::endl;

    std::cout << "Elapsed time: " << elapsed.count() << " ms\n";
    std::cout << "Elapsed time (getwork): " << elapsed_getwork.count() << " ms\n";
    std::cout << "Elapsed time (work): " << elapsed_work.count() << " ms\n";
    
}


}  // namespace lbcrypto

#endif
