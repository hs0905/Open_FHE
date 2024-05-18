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
  Creates Represents integer lattice elements
 */

#ifndef LBCRYPTO_INC_LATTICE_HAL_DEFAULT_POLY_H
#define LBCRYPTO_INC_LATTICE_HAL_DEFAULT_POLY_H

#include "lattice/hal/poly-interface.h"
#include "lattice/ildcrtparams.h"
#include "lattice/ilparams.h"

#include "math/distrgen.h"
#include "math/math-hal.h"
#include "math/nbtheory.h"

#include "utils/exception.h"
#include "utils/inttypes.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "utils/memory_tracking.h"

/* function for recording memory transfer stats (utils/math_utils.cpp)*/
void inc_copy_from_shadow();
void inc_copy_from_shadow_ocb_real();
void inc_copy_from_shadow_hbm_real();
void inc_copy_to_shadow();
void inc_copy_to_shadow_real();
void inc_copy_from_other_shadow();
void inc_copy_from_other_shadow1();
void inc_copy_from_other_shadow2();
void inc_copy_from_other_shadow3();
void inc_copy_from_other_shadow4();
void inc_create_shadow();
void inc_discard_shadow();
void inc_compute_implemented();
void inc_compute_not_implemented();

/* fucntion for managing memory tracking (for recognizing buffer locations) (utils/memory_tracking.cpp) */
bool check_full_ocb_entries();
bool check_full_hbm_entries();
void insert_shadow_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr, bool &ongoing_flag);
void insert_shadow_hbm_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr);
std::tuple<uint64_t,uint64_t,bool*> evict_shadow_tracking_array();
std::tuple<uint64_t,uint64_t> evict_shadow_hbm_tracking_array();
std::tuple<uint64_t,uint64_t,bool*> select_shadow_tracking_array(uint64_t m_values_shadow_addr);
std::tuple<uint64_t,uint64_t> select_shadow_hbm_tracking_array(uint64_t m_values_shadow_addr);
void clean_shadow_tracking_array(uint64_t m_values_shadow_addr);

bool check_evk_set(uint64_t evk_addr); 

/* working queue for off-load FHE tasks */
#include "utils/custom_task.h"
extern WorkQueue work_queue;

extern std::mutex ocb_entries_m;

namespace lbcrypto {

/* Shadow refers to a buffer that has been transferred to hardware (FPGA)
    not the original buffer on the host side. 
    After checking the following shadow state, it is decided whether to move the shadow
    or not when an arbitrary requests the corresponding polinomial. */
#define SHADOW_NOTEXIST 0 
#define SHADOW_SYNCHED 1 
#define SHADOW_IS_AHEAD 2
#define SHADOW_IS_BEHIND 3

/* Since shadow is assumed to be in the on-chip buffer or HBM of the FPGA or ASIC,
    the location of the shadow should be indicated as below. */
#define SHADOW_ON_OCB 1
#define SHADOW_ON_HBM 2

/**
 * @class PolyImpl
 * @file poly.h
 * @brief Ideal lattice using a vector representation
 */

/* The class below is the polynomial's shadow (off-load buffer) class to be used for the fhe operation.
    m_values : Polynomial data in on-chip buffer
    m_values_hbm : Polynomial data in HBM
    shadow_sync_state : SHADOW_NOTEXIST, SHADOW_SYNCHED,
                        SHADOW_IS_AHEAD(need to transfer Host <- Shadow),
                        SHADOW_IS_BEHIND (need to transfer Host -> Shadow)
    shadow_location : SHADOW_ON_OCB, SHADOW_ON_HBM
    ongoing_flag : Flag that indicate whether the polinomial is currently being used for fhe operation.
                   To prevent memory from being moved by the evict policy when it is being used for fhe operation.
    get_ptr : Method to get a on-chip shadow address
    get_hbm_ptr : Method to get a HBM shadow address
*/

template <typename VecType>
class ShadowType {
    public:
        mutable std::shared_ptr<std::vector<uint64_t>> m_values{nullptr};
        mutable std::shared_ptr<std::vector<uint64_t>> m_values_hbm{nullptr};
        mutable usint shadow_sync_state{SHADOW_NOTEXIST};  
        mutable usint shadow_location{SHADOW_NOTEXIST};
        mutable bool ongoing_flag{false};
        uint64_t* get_ptr() {return &(*m_values)[0];}
        uint64_t* get_hbm_ptr() {return &(*m_values_hbm)[0];}
}; 

template <typename VecType>
class PolyImpl final : public PolyInterface<PolyImpl<VecType>, VecType, PolyImpl> {
public:
    using Vector            = VecType;
    using Integer           = typename VecType::Integer;
    using Params            = ILParamsImpl<Integer>;
    using PolyNative        = PolyImpl<NativeVector>;
    using PolyType          = PolyImpl<VecType>;
    using PolyLargeType     = PolyImpl<VecType>;
    using PolyInterfaceType = PolyInterface<PolyImpl<VecType>, VecType, PolyImpl>;
    using DggType           = typename PolyInterfaceType::DggType;
    using DugType           = typename PolyInterfaceType::DugType;
    using TugType           = typename PolyInterfaceType::TugType;
    using BugType           = typename PolyInterfaceType::BugType;

    constexpr PolyImpl() = default;

    /* Openfhe automatically releases memory because it uses smart pointers.
        Therefore, when the memory for the polynomial is released,
        Remove the memory information from the memory tracking data structure
        ( clean_shadow_tracking_array ) */
    ~PolyImpl() noexcept{
        clean_shadow_tracking_array((uint64_t)&m_values_shadow);
    }

    /* Data trasfer : Host(Origin) <- Hardware(on-chip buffer or HBM)
        Check the shadoww_sync_state and shadow_location
        and transfer data to the host if you need to move the data */
    void copy_from_shadow() const {
        inc_copy_from_shadow();
        if(m_values == nullptr) {
            usint r{m_params->GetRingDimension()};
            m_values = std::make_unique<VecType>(r, m_params->GetModulus());
        }

        if(m_values_shadow.shadow_location==SHADOW_ON_OCB && m_values_shadow.shadow_sync_state == SHADOW_IS_AHEAD) {
            inc_copy_from_shadow_ocb_real();
            ::memcpy((char*)&m_values->m_data[0],m_values_shadow.get_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
            m_values_shadow.shadow_sync_state = SHADOW_SYNCHED;
        }

        if(m_values_shadow.shadow_location==SHADOW_ON_HBM && m_values_shadow.shadow_sync_state == SHADOW_IS_AHEAD) {
            inc_copy_from_shadow_hbm_real();
            ::memcpy((char*)&m_values->m_data[0],m_values_shadow.get_hbm_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
            m_values_shadow.shadow_sync_state = SHADOW_SYNCHED;
        }
    }

    /* Data trasfer : Host(Origin) <- Hardware(on-chip buffer or HBM)
        Special method for discard (evcit policy)
        to move a specific memory discarded by an evict policy to an original buffer
        Discarded memory initializes to its initial state */
    void copy_from_shadow_for_discard(std::unique_ptr<VecType>& tmp_m_values ,ShadowType<VecType>& tmp_m_values_shadow) const {
        if(tmp_m_values_shadow.m_values == nullptr){
            return;
        }
        else{
            if(tmp_m_values == nullptr) {
                usint r{m_params->GetRingDimension()};
                tmp_m_values = std::make_unique<VecType>(r, m_params->GetModulus());
            }

            inc_copy_from_shadow_hbm_real();
            ::memcpy((char*)&tmp_m_values->m_data[0],tmp_m_values_shadow.get_hbm_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());

            inc_discard_shadow();
            tmp_m_values_shadow.m_values = nullptr;
            tmp_m_values_shadow.m_values_hbm = nullptr;
            tmp_m_values_shadow.shadow_sync_state = SHADOW_NOTEXIST;
            tmp_m_values_shadow.shadow_location = SHADOW_NOTEXIST;
        }
    }

    /* Data trasfer : Host(Origin) -> Hardware(on-chip buffer or HBM)
        Check the shadoww_sync_state and shadow_location
        and transfer data to the shadow if you need to move the data
        It's important to note that, when the shadow is in the HBM and the on-chip buffer is full
        we need to swap data (on-chip buffer <-> HBM)
        select hbm shadow buffer, swap hbm shadow & victim on-chip buffer shadow
    */
    void copy_to_shadow() const {
        inc_copy_to_shadow();
        ocb_entries_m.lock();
        if(m_values_shadow.shadow_location==SHADOW_ON_HBM){ // When shadow in HBM, need to swap OCB <-> HBM
            std::tuple<uint64_t,uint64_t> hbm_tmp = select_shadow_hbm_tracking_array(uint64_t(&m_values_shadow));
            if(check_full_ocb_entries()){
                std::tuple<uint64_t,uint64_t,bool*> tmp = evict_shadow_tracking_array();
                if(std::get<1>(hbm_tmp)) insert_shadow_tracking_array(std::get<0>(hbm_tmp),std::get<1>(hbm_tmp),m_values_shadow.ongoing_flag);
                if(std::get<1>(tmp)) insert_shadow_hbm_tracking_array(std::get<0>(tmp),std::get<1>(tmp));
                
                
                if(std::get<1>(hbm_tmp)) copy_from_hbm_shadow(m_values_shadow);
                if(std::get<1>(tmp)){
                    ShadowType<VecType>* tmp_m_values_shadow_addr = (ShadowType<VecType>*)std::get<1>(tmp);
                    copy_to_hbm_shadow(*tmp_m_values_shadow_addr);
                }
            }
            else{
                if(std::get<1>(hbm_tmp)){
                    insert_shadow_tracking_array(std::get<0>(hbm_tmp),std::get<1>(hbm_tmp),m_values_shadow.ongoing_flag);
                    copy_from_hbm_shadow(m_values_shadow);
                }
            }
        }
        ocb_entries_m.unlock();
        if(m_values == nullptr) {
            if(m_values_shadow.shadow_sync_state != SHADOW_IS_AHEAD) {
                OPENFHE_THROW(not_available_error, "m_values not created");
            }
            return;
        }

        if(m_values_shadow.shadow_sync_state == SHADOW_NOTEXIST) {
            create_shadow();   
        }            
        if(m_values_shadow.shadow_sync_state == SHADOW_IS_BEHIND) {   
            if(!check_evk_set((uint64_t)&m_values)) inc_copy_to_shadow_real();
            ::memcpy(m_values_shadow.get_ptr(),(char*)&m_values->m_data[0],sizeof(uint64_t)*m_params->GetRingDimension());
            m_values_shadow.shadow_sync_state = SHADOW_SYNCHED;
        }
    }

    /* Data trasfer : Hardware(on-chip buffer(OCB) or HBM) -> Hardware(on-chip buffer or HBM)
        OCB -> OCB, OCB -> HBM, HBM -> OCB, HBM -> HBM
        polynomial copy operation in FHE 
    */
    void copy_from_other_shadow(ShadowType<VecType>& other) {
        if(!m_values_shadow.m_values) {
            OPENFHE_THROW(not_available_error, "shadow not created");
        }
        if(!other.m_values){
            OPENFHE_THROW(not_available_error, "other shadow not created");
        }
        else {
            inc_copy_from_other_shadow();
            if(m_values_shadow.shadow_location==SHADOW_ON_OCB && other.shadow_location==SHADOW_ON_OCB){
                if(!check_evk_set((uint64_t)&m_values)) inc_copy_from_other_shadow1();
                ::memcpy(m_values_shadow.get_ptr(),other.get_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
                m_values_shadow.shadow_sync_state = other.shadow_sync_state;
            }
            else if(m_values_shadow.shadow_location==SHADOW_ON_OCB && other.shadow_location==SHADOW_ON_HBM){
                inc_copy_from_other_shadow2();
                ::memcpy(m_values_shadow.get_ptr(),other.get_hbm_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
                m_values_shadow.shadow_sync_state = other.shadow_sync_state;
            }
            else if(m_values_shadow.shadow_location==SHADOW_ON_HBM && other.shadow_location==SHADOW_ON_OCB){
                inc_copy_from_other_shadow3();
                ::memcpy(m_values_shadow.get_hbm_ptr(),other.get_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
                m_values_shadow.shadow_sync_state = other.shadow_sync_state;
            }
            else if(m_values_shadow.shadow_location==SHADOW_ON_HBM && other.shadow_location==SHADOW_ON_HBM){
                inc_copy_from_other_shadow4();
                ::memcpy(m_values_shadow.get_hbm_ptr(),other.get_hbm_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
                m_values_shadow.shadow_sync_state = other.shadow_sync_state;
            }
            else{
                OPENFHE_THROW(not_available_error, "wrong both location");
            }
        }
    }

    /* Specialized method for OCB -> HBM
        Using in copy_to_shadow, discard_shadow
    */
    void copy_to_hbm_shadow(ShadowType<VecType>& tmp_m_values_shadow) const{
        if(!tmp_m_values_shadow.m_values) {
            return;
        }
        else{
            if(!tmp_m_values_shadow.m_values_hbm){
                tmp_m_values_shadow.m_values_hbm = std::make_shared<std::vector<uint64_t>>(m_params->GetRingDimension());
            }
            inc_copy_from_other_shadow3();
            ::memcpy(tmp_m_values_shadow.get_hbm_ptr(),tmp_m_values_shadow.get_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
            tmp_m_values_shadow.shadow_location = SHADOW_ON_HBM;
        }
    }

    /* Specialized method for HBM -> OCB
        Using in copy_to_shadow
    */
    void copy_from_hbm_shadow(ShadowType<VecType>& tmp_m_values_shadow) const{
        if(!tmp_m_values_shadow.m_values) {
            OPENFHE_THROW(not_available_error, "shadow not created");
        }
        else{
            inc_copy_from_other_shadow2();
            ::memcpy(tmp_m_values_shadow.get_ptr(),tmp_m_values_shadow.get_hbm_ptr(),sizeof(uint64_t)*m_params->GetRingDimension());
            tmp_m_values_shadow.shadow_location = SHADOW_ON_OCB;
        }
    }

    /* When the shadow class corresponding to the origin buffer(Host) is not generated,
        it serves to allocate shadow memory.
        By default, the created shadow is located in an on-chip buffer because it was created for use right now.
        If the on-chip buffer is full,
        discard_shadow is used by the evict policy to manage additional data transfers
    */
    void create_shadow() const {
        if(m_values_shadow.m_values) return;

        ocb_entries_m.lock();
        if(!check_evk_set((uint64_t)&m_values) && check_full_ocb_entries()) discard_shadow();

        inc_create_shadow();
        m_values_shadow.m_values = std::make_shared<std::vector<uint64_t>>(m_params->GetRingDimension());
        m_values_shadow.shadow_sync_state = SHADOW_IS_BEHIND;
        m_values_shadow.shadow_location = SHADOW_ON_OCB;

        if(!check_evk_set((uint64_t)&m_values)) insert_shadow_tracking_array((uint64_t)&m_values,(uint64_t)&m_values_shadow,m_values_shadow.ongoing_flag);
        ocb_entries_m.unlock();
    }

    /* Discard shadow is a function created to manage situations where on-chip buffers and HBMs will be full.
        To get the memory space required for the operation,
        Move the buffer occupying the space to the HBM or import it back to the host according to the evict policy.
    */
    void discard_shadow() const{        
        if(check_full_hbm_entries()){ // When HBM is full
            std::tuple<uint64_t,uint64_t> tmp = evict_shadow_hbm_tracking_array();
            if(std::get<0>(tmp)!=0){
                std::unique_ptr<VecType>* tmp_m_values_addr = (std::unique_ptr<VecType>*)std::get<0>(tmp);
                ShadowType<VecType>* tmp_m_values_shadow_addr = (ShadowType<VecType>*)std::get<1>(tmp);
                copy_from_shadow_for_discard(*tmp_m_values_addr,*tmp_m_values_shadow_addr);
            }
        }
        std::tuple<uint64_t,uint64_t,bool*> tmp = evict_shadow_tracking_array();
        if(std::get<0>(tmp)!=0){ // When OCB is full
            ShadowType<VecType>* tmp_m_values_shadow_addr = (ShadowType<VecType>*)std::get<1>(tmp);
            copy_to_hbm_shadow(*tmp_m_values_shadow_addr);
            if((*tmp_m_values_shadow_addr).m_values){
                insert_shadow_hbm_tracking_array(std::get<0>(tmp),std::get<1>(tmp));
            }
        }
    }

    /* If polynomial data is changed by fhe operation in hardware(shadow),
        indicate modified flag in shadow buffer */
    void indicate_modified_shadow() {        
        if( m_values_shadow.shadow_sync_state != SHADOW_NOTEXIST)
           if( m_values_shadow.m_values == nullptr ) 
               OPENFHE_THROW(not_available_error, "m_values_shadow.m_values is null");

        m_values_shadow.shadow_sync_state = SHADOW_IS_AHEAD; 
    }
    /* If polynomial data is changed by fhe operation in host(origin),
        indicate modified flag in origin buffer */
    void indicate_modified_orig() {        
        if( m_values_shadow.shadow_sync_state != SHADOW_NOTEXIST) {
            if( m_values_shadow.m_values == nullptr ) 
               OPENFHE_THROW(not_available_error, "m_values_shadow.m_values is null");
            m_values_shadow.shadow_sync_state = SHADOW_IS_BEHIND;  
        }      
        else {
            if( m_values_shadow.m_values != nullptr ) 
               OPENFHE_THROW(not_available_error, "m_values_shadow.m_values is not null");
        }   
    }


    PolyImpl(const std::shared_ptr<Params>& params, Format format = Format::EVALUATION,
             bool initializeElementToZero = false)
        : m_format{format}, m_params{params} {
        if (initializeElementToZero)
            PolyImpl::SetValuesToZero();
    }
    PolyImpl(const std::shared_ptr<ILDCRTParams<Integer>>& params, Format format = Format::EVALUATION,
             bool initializeElementToZero = false);

    PolyImpl(bool initializeElementToMax, const std::shared_ptr<Params>& params, Format format = Format::EVALUATION)
        : m_format{format}, m_params{params} {
        if (initializeElementToMax)
            PolyImpl::SetValuesToMax();
    }
    PolyImpl(const DggType& dgg, const std::shared_ptr<Params>& params, Format format = Format::EVALUATION);
    PolyImpl(DugType& dug, const std::shared_ptr<Params>& params, Format format = Format::EVALUATION);
    PolyImpl(const BugType& bug, const std::shared_ptr<Params>& params, Format format = Format::EVALUATION);
    PolyImpl(const TugType& tug, const std::shared_ptr<Params>& params, Format format = Format::EVALUATION,
             uint32_t h = 0);

    template <typename T = VecType>
    PolyImpl(const PolyNative& rhs, Format format,
             typename std::enable_if_t<std::is_same_v<T, NativeVector>, bool> = true)
        : m_format{rhs.m_format},
          m_params{rhs.m_params},
          m_values{rhs.m_values ? std::make_unique<VecType>(*rhs.m_values) : nullptr} {
        OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
        if(rhs.m_values_shadow.shadow_sync_state != SHADOW_NOTEXIST) {
            this->create_shadow();
            this->copy_from_other_shadow(rhs.m_values_shadow);
        }
        else {
            this->m_values_shadow.shadow_sync_state = SHADOW_NOTEXIST;
        }
        PolyImpl<VecType>::SetFormat(format);
    }

    template <typename T = VecType>
    PolyImpl(const PolyNative& rhs, Format format,
             typename std::enable_if_t<!std::is_same_v<T, NativeVector>, bool> = true)
        : m_format{rhs.GetFormat()} {
        auto c{rhs.GetParams()->GetCyclotomicOrder()};
        auto m{rhs.GetParams()->GetModulus().ConvertToInt()};
        auto r{rhs.GetParams()->GetRootOfUnity().ConvertToInt()};
        m_params = std::make_shared<PolyImpl::Params>(c, m, r);

        const auto& v{rhs.GetValues()};
        uint32_t vlen{m_params->GetRingDimension()};
        VecType tmp(vlen);
        tmp.SetModulus(m_params->GetModulus());
        for (uint32_t i{0}; i < vlen; ++i)
            tmp[i] = Integer(v[i]);
        m_values = std::make_unique<VecType>(tmp);
        this->indicate_modified_orig();
        PolyImpl<VecType>::SetFormat(format);
    }

    /*Allocate temporary buffer and copy buffer operation in Openfhe
        This information should also be recorded in the memory tracking structure
        Create new shadow and copy the data to this shadow. */
    PolyImpl(const PolyType& p) noexcept
        : m_format{p.m_format},
          m_params{p.m_params},
          m_values{p.m_values ? std::make_unique<VecType>(*p.m_values) : nullptr} {
            ocb_entries_m.lock();
            p.m_values_shadow.ongoing_flag = true;
            ocb_entries_m.unlock();
            if(p.m_values_shadow.shadow_sync_state != SHADOW_NOTEXIST) {
                this->create_shadow();
                this->copy_from_other_shadow(p.m_values_shadow);
            }
            else {
                this->m_values_shadow.shadow_sync_state = SHADOW_NOTEXIST;
            }
            ocb_entries_m.lock();
            p.m_values_shadow.ongoing_flag = false;
            ocb_entries_m.unlock();
          }

    PolyImpl(PolyType&& p) noexcept
        : m_format{p.m_format}, m_params{std::move(p.m_params)}, m_values{std::move(p.m_values)}, m_values_shadow{p.m_values_shadow} {}

    PolyType& operator=(const PolyType& rhs) noexcept override;
    PolyType& operator=(PolyType&& rhs) noexcept override {
        m_format = std::move(rhs.m_format);
        m_params = std::move(rhs.m_params);
        m_values = std::move(rhs.m_values);
        m_values_shadow = rhs.m_values_shadow;
        return *this;
    }
    PolyType& operator=(const std::vector<int32_t>& rhs);
    PolyType& operator=(const std::vector<int64_t>& rhs);
    PolyType& operator=(std::initializer_list<uint64_t> rhs) override;
    PolyType& operator=(std::initializer_list<std::string> rhs);
    PolyType& operator=(uint64_t val);

    PolyNative DecryptionCRTInterpolate(PlaintextModulus ptm) const override;
    PolyNative ToNativePoly() const final {
        usint vlen{m_params->GetRingDimension()};
        auto c{m_params->GetCyclotomicOrder()};
        NativeInteger m{std::numeric_limits<BasicInteger>::max()};
        auto params{std::make_shared<ILParamsImpl<NativeInteger>>(c, m, 1)};
        typename PolyImpl<VecType>::PolyNative tmp(params, m_format, true);
        this->copy_from_shadow();
        for (usint i = 0; i < vlen; ++i)
            tmp[i] = NativeInteger((*m_values)[i]);
        return tmp;
    }

    void SetValues(const VecType& values, Format format) override;
    void SetValues(VecType&& values, Format format) override;
    void SetValuesShadow(const VecType& values, Format format);
    void SetValuesShadow(VecType&& values, Format format);

    void SetValuesToZero() override {
        usint r{m_params->GetRingDimension()};
        m_values = std::make_unique<VecType>(r, m_params->GetModulus());
        this->indicate_modified_orig();
    }

    void SetValuesToMax() override {
        usint r{m_params->GetRingDimension()};
        auto max{m_params->GetModulus() - Integer(1)};
        m_values = std::make_unique<VecType>(r, m_params->GetModulus(), max);
        this->indicate_modified_orig();
    }

    inline Format GetFormat() const final {
        return m_format;
    }

    void OverrideFormat(const Format f) final {
        m_format = f;
    }

    inline const std::shared_ptr<Params>& GetParams() const {
        return m_params;
    }

    inline const VecType& GetValues() const final {
        this->copy_from_shadow();
        if (m_values == nullptr)
            OPENFHE_THROW(not_available_error, "No values in PolyImpl");
        return *m_values;
    }

    inline bool IsEmpty() const final {
        this->copy_from_shadow();
        return m_values == nullptr;
    }

    inline Integer& at(usint i) final {
        this->copy_from_shadow();
        if (m_values == nullptr)
            OPENFHE_THROW(not_available_error, "No values in PolyImpl");
        return m_values->at(i);
    }

    inline const Integer& at(usint i) const final {
        this->copy_from_shadow();
        if (m_values == nullptr)
            OPENFHE_THROW(not_available_error, "No values in PolyImpl");
        return m_values->at(i);
    }

    inline Integer& operator[](usint i) final {
        this->copy_from_shadow();
        return (*m_values)[i];
    }

    inline const Integer& operator[](usint i) const final {
        this->copy_from_shadow();
        return (*m_values)[i];
    }

    /* From below, openfhe's unit operation is shown.
    For the management purpose of the operation, we use the working queue,
    so we define the information of the operation as a task and put it into the queue.
    Other definitions of unit operation are in poly-impl.h, transformnat.h and transformnat-impl.h */
    PolyImpl Plus(const PolyImpl& rhs) const override {
        if (m_params->GetRingDimension() != rhs.m_params->GetRingDimension())
            OPENFHE_THROW(math_error, "RingDimension missmatch");
        if (m_params->GetModulus() != rhs.m_params->GetModulus())
            OPENFHE_THROW(math_error, "Modulus missmatch");
        if (m_format != rhs.m_format)
            OPENFHE_THROW(not_implemented_error, "Format missmatch");
        auto tmp(*this);

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_Plus);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->poly3 = (void*)&rhs;

        work_queue.addWork(item);

        delete item;

        return tmp;
    }
    PolyImpl PlusNoCheck(const PolyImpl& rhs) const {
        auto tmp(*this);

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_Plus);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->poly3 = (void*)&rhs;

        work_queue.addWork(item);

        delete item;

        return tmp;
    }
    PolyImpl& operator+=(const PolyImpl& element) override;

    PolyImpl Plus(const Integer& element) const override;
    PolyImpl& operator+=(const Integer& element) override {
        return *this = this->Plus(element);  // don't change this
    }

    PolyImpl Minus(const PolyImpl& element) const override;
    PolyImpl& operator-=(const PolyImpl& element) override;

    PolyImpl Minus(const Integer& element) const override;
    PolyImpl& operator-=(const Integer& element) override {
        OPENFHE_THROW(not_implemented_error, "hcho: not tested here");
        this->copy_from_shadow();
        m_values->ModSubEq(element);
        this->indicate_modified_orig();
        return *this;
    }

    PolyImpl Times(const PolyImpl& rhs) const override {
        if (m_params->GetRingDimension() != rhs.m_params->GetRingDimension())
            OPENFHE_THROW(math_error, "RingDimension missmatch");
        if (m_params->GetModulus() != rhs.m_params->GetModulus())
            OPENFHE_THROW(math_error, "Modulus missmatch");
        if (m_format != Format::EVALUATION || rhs.m_format != Format::EVALUATION)
            OPENFHE_THROW(not_implemented_error, "operator* for PolyImpl supported only in Format::EVALUATION");
        auto tmp(*this);

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_Times);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->poly3 = (void*)&rhs;

        work_queue.addWork(item);

        delete item;

        return tmp;
    }
    PolyImpl TimesNoCheck(const PolyImpl& rhs) const {
        auto tmp(*this);

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_TimesNoCheck);

        item->poly = (void*)this;
        item->poly2 = (void*)&tmp;
        item->poly3 = (void*)&rhs;

        work_queue.addWork(item);

        delete item;

        return tmp;
    }
    PolyImpl& operator*=(const PolyImpl& rhs) override {
        if (m_params->GetRingDimension() != rhs.m_params->GetRingDimension())
            OPENFHE_THROW(math_error, "RingDimension missmatch");
        if (m_params->GetModulus() != rhs.m_params->GetModulus())
            OPENFHE_THROW(math_error, "Modulus missmatch");
        if (m_format != Format::EVALUATION || rhs.m_format != Format::EVALUATION)
            OPENFHE_THROW(not_implemented_error, "operator* for PolyImpl supported only in Format::EVALUATION");

        if (!m_values)
            m_values = std::make_unique<VecType>(m_params->GetRingDimension(), m_params->GetModulus());

        CustomTaskItem* item = new CustomTaskItem(TASK_TYPE_TimesInPlace);

        item->poly = (void*)this;
        item->poly2 = (void*)&rhs;

        work_queue.addWork(item);

        delete item;

        return *this;
    }

    PolyImpl Times(const Integer& element) const override;
    PolyImpl& operator*=(const Integer& element) override; 

    PolyImpl Times(NativeInteger::SignedNativeInt element) const override;
#if NATIVEINT != 64
    inline PolyImpl Times(int64_t element) const {
        return this->Times(static_cast<NativeInteger::SignedNativeInt>(element));
    }
#endif

    PolyImpl MultiplyAndRound(const Integer& p, const Integer& q) const override;
    PolyImpl DivideAndRound(const Integer& q) const override;

    PolyImpl Negate() const override;
    inline PolyImpl operator-() const override {
        return PolyImpl(m_params, m_format, true) -= *this;
    }

    inline bool operator==(const PolyImpl& rhs) const override {
        return ((m_format == rhs.GetFormat()) && (m_params->GetRootOfUnity() == rhs.GetRootOfUnity()) &&
                (this->GetValues() == rhs.GetValues()));
    }

    void AddILElementOne() override;
    PolyImpl AutomorphismTransform(uint32_t k) const override;
    PolyImpl AutomorphismTransform(uint32_t k, const std::vector<uint32_t>& vec) const override;
    PolyImpl MultiplicativeInverse() const override;
    PolyImpl ModByTwo() const override;
    PolyImpl Mod(const Integer& modulus) const override;

    void SwitchModulus(const Integer& modulus, const Integer& rootOfUnity, const Integer& modulusArb,
                       const Integer& rootOfUnityArb) override;
    void SwitchFormat() override;
    void MakeSparse(uint32_t wFactor) override;
    bool InverseExists() const override;
    double Norm() const override;
    std::vector<PolyImpl> BaseDecompose(usint baseBits, bool evalModeAnswer) const override;
    std::vector<PolyImpl> PowersOfBase(usint baseBits) const override;

    template <class Archive>
    void save(Archive& ar, std::uint32_t const version) const {
        ar(::cereal::make_nvp("v", m_values));
        ar(::cereal::make_nvp("f", m_format));
        ar(::cereal::make_nvp("p", m_params));
    }

    template <class Archive>
    void load(Archive& ar, std::uint32_t const version) {
        if (version > SerializedVersion()) {
            OPENFHE_THROW(deserialize_error, "serialized object version " + std::to_string(version) +
                                                 " is from a later version of the library");
        }
        ar(::cereal::make_nvp("v", m_values));
        ar(::cereal::make_nvp("f", m_format));
        ar(::cereal::make_nvp("p", m_params));
    }

    static const std::string GetElementName() {
        return "PolyImpl";
    }

    std::string SerializedObjectName() const override {
        return "Poly";
    }

    static uint32_t SerializedVersion() {
        return 1;
    }

protected:
public:
    Format m_format{Format::EVALUATION};
    std::shared_ptr<Params> m_params{nullptr};
    mutable std::unique_ptr<VecType> m_values{nullptr};
    mutable ShadowType<VecType> m_values_shadow;
    void ArbitrarySwitchFormat();
};

// TODO: fix issue with pke build system so this can be moved back to implementation file
template <>
inline PolyImpl<BigVector>::PolyImpl(const std::shared_ptr<ILDCRTParams<BigInteger>>& params, Format format,
                                     bool initializeElementToZero)
    : m_format(format), m_params(nullptr), m_values(nullptr) {
    const auto c = params->GetCyclotomicOrder();
    const auto m = params->GetModulus();
    m_params     = std::make_shared<ILParams>(c, m, 1);
    if (initializeElementToZero)
        this->SetValuesToZero();
}

}  // namespace lbcrypto

#endif
