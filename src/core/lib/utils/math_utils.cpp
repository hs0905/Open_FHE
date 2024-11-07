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
#include <thread>
#include <iomanip>
#include <fstream>

#include "utils/custom_task.h"
#include "utils/memory_tracking.h"

uint32_t cnt_copy_from_shadow;
uint32_t cnt_copy_from_shadow_ocb_real;
uint32_t cnt_copy_from_shadow_hbm_real;
uint32_t cnt_copy_from_root_shadow_hbm_real;
uint32_t cnt_copy_from_inv_root_shadow_hbm_real;
uint32_t cnt_copy_to_shadow;
uint32_t cnt_copy_to_root_shadow;
uint32_t cnt_copy_to_shadow_real;
uint32_t cnt_copy_from_other_shadow;
uint32_t cnt_copy_from_other_shadow1;
uint32_t cnt_copy_from_other_shadow2;
uint32_t cnt_copy_from_other_shadow3;
uint32_t cnt_copy_from_other_shadow4;
uint32_t cnt_create_shadow;
uint32_t cnt_discard_shadow;
uint32_t cnt_create_root_shadow;
uint32_t cnt_compute_implemented;
uint32_t cnt_compute_not_implemented;

uint32_t cnt_copy_to_shadow_real_evk;

uint32_t cnt_ntt;
uint32_t cnt_intt;
uint32_t cnt_auto;
uint32_t cnt_add;
uint32_t cnt_sub;
uint32_t cnt_mult;
uint32_t cnt_bconv_up;
uint32_t cnt_bconv_down;

bool compute_flag = false;

WorkQueue work_queue;
std::thread consumerThread;

extern std::unordered_set<uint64_t> evk_set;
extern std::unordered_map<uint64_t,uint64_t> evk_map;
extern std::chrono::duration<double, std::milli> elapsed_overhead;

std::chrono::duration<double, std::milli> elapsed_getwork(0.0);
std::chrono::duration<double, std::milli> elapsed_work(0.0);

bool check_evk_set(uint64_t evk_addr);

uint64_t total_sizeQlP = 0;

namespace lbcrypto{
extern  void consumer(WorkQueue& queue);
}

void init_stat() {
    std::cout << "init_stat" << std::endl;
    cnt_copy_from_shadow = 0;
    cnt_copy_from_shadow_ocb_real = 0;
    cnt_copy_from_shadow_hbm_real = 0;
    cnt_copy_from_root_shadow_hbm_real = 0;
    cnt_copy_to_shadow = 0;
    cnt_copy_to_root_shadow = 0;
    cnt_copy_to_shadow_real = 0;
    cnt_copy_from_other_shadow = 0;
    cnt_copy_from_other_shadow1 = 0;
    cnt_copy_from_other_shadow2 = 0;
    cnt_copy_from_other_shadow3 = 0;
    cnt_copy_from_other_shadow4 = 0;
    cnt_create_shadow = 0;
    cnt_discard_shadow = 0;
    cnt_create_root_shadow = 0;
    cnt_compute_implemented = 0;
    cnt_compute_not_implemented = 0;
    cnt_ntt = 0;
    cnt_intt = 0;
    cnt_auto = 0;
    cnt_add = 0;
    cnt_sub = 0;
    cnt_mult = 0;
    cnt_bconv_up = 0;
    cnt_bconv_down = 0;

    cnt_copy_to_shadow_real_evk = 0;
    elapsed_overhead = std::chrono::duration<double, std::milli>(0.0);
    elapsed_getwork = std::chrono::duration<double, std::milli>(0.0);
    elapsed_work = std::chrono::duration<double, std::milli>(0.0);

    consumerThread = std::thread(lbcrypto::consumer, std::ref(work_queue));
}
void init_stat_no_workqueue() {
    std::cout << "init_stat" << std::endl;
    cnt_copy_from_shadow = 0;
    cnt_copy_from_shadow_ocb_real = 0;
    cnt_copy_from_shadow_hbm_real = 0;
    cnt_copy_from_root_shadow_hbm_real = 0;
    cnt_copy_to_shadow = 0;
    cnt_copy_to_root_shadow = 0;
    cnt_copy_to_shadow_real = 0;
    cnt_copy_from_other_shadow = 0;
    cnt_copy_from_other_shadow1 = 0;
    cnt_copy_from_other_shadow2 = 0;
    cnt_copy_from_other_shadow3 = 0;
    cnt_copy_from_other_shadow4 = 0;
    cnt_create_shadow = 0;
    cnt_discard_shadow = 0;
    cnt_create_root_shadow = 0;
    cnt_compute_implemented = 0;
    cnt_compute_not_implemented = 0;
    cnt_ntt = 0;
    cnt_intt = 0;
    cnt_auto = 0;
    cnt_add = 0;
    cnt_sub = 0;
    cnt_mult = 0;
    cnt_bconv_up = 0;
    cnt_bconv_down = 0;

    total_sizeQlP = 0;

    cnt_copy_to_shadow_real_evk = 0;
    elapsed_overhead = std::chrono::duration<double, std::milli>(0.0);
    elapsed_getwork = std::chrono::duration<double, std::milli>(0.0);
    elapsed_work = std::chrono::duration<double, std::milli>(0.0);

    std::cout << "initialized evk_set size: " << evk_set.size() << " / " << evk_set.size()/2 << "MB" << std::endl;
    // for (auto& pair : evk_map) {
    //     pair.second = 0;
    // }
}
void print_stat() {
    std::cout << "print_stat" << std::endl;
    std::cout << "cnt_copy_from_shadow: " << cnt_copy_from_shadow<< std::endl;
    std::cout << "ORIGIN <-- OCB          : " << cnt_copy_from_shadow_ocb_real<< std::endl;
    std::cout << "ORIGIN     <--     HBM  : " << cnt_copy_from_shadow_hbm_real<< std::endl;
    std::cout << "cnt_copy_to_shadow: " << cnt_copy_to_shadow<< std::endl;
    std::cout << "ORIGIN --> OCB          : " << cnt_copy_to_shadow_real<<" (evk: " << cnt_copy_to_shadow_real_evk << ")" << std::endl;
    std::cout << "cnt_copy_from_other_shadow: " << cnt_copy_from_other_shadow<< std::endl;
    std::cout << "           OCB          : " << cnt_copy_from_other_shadow1<< std::endl;
    std::cout << "           OCB <-- HBM  : " << cnt_copy_from_other_shadow2<< std::endl;
    std::cout << "           OCB --> HBM  : " << cnt_copy_from_other_shadow3<< std::endl;
    std::cout << "  total    OCB --- HBM  : " << cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3<< std::endl;
    std::cout << "                   HBM  : " << cnt_copy_from_other_shadow4<< std::endl;
    std::cout << "cnt_create_shadow: " << cnt_create_shadow<< std::endl;
    std::cout << "root copy: " << cnt_copy_from_root_shadow_hbm_real << std::endl;
    std::cout << "inv root copy: " << cnt_copy_from_inv_root_shadow_hbm_real << std::endl;
    std::cout << "cnt_compute_implemented: " << cnt_compute_implemented<< std::endl;
    std::cout << std::endl;
    std::cout << "cnt_ntt: " << cnt_ntt << std::endl;
    std::cout << "cnt_intt: " << cnt_intt << std::endl;
    std::cout << "cnt_auto: " << cnt_auto << std::endl;
    std::cout << "cnt_add: " << cnt_add << std::endl;
    std::cout << "cnt_sub: " << cnt_sub << std::endl;
    std::cout << "cnt_mult: " << cnt_mult << std::endl;
    std::cout << "cnt_bconv_up: " << cnt_bconv_up << std::endl;
    std::cout << "cnt_bconv_down: " << cnt_bconv_down << std::endl;
    std::cout << std::endl;
    std::cout << "elapsed_overhead: " << elapsed_overhead.count() << "ms" << std::endl;

    // // double total_time = 0;
    // // double ntt_cycle = 3454; // e=512
    // // double intt_cycle = 3343;
    // // double auto_cycle = 2578;
    // // double add_cycle = 149;
    // // double mult_cycle = 233;
    // // double sub_cycle = 146;
    // // double bconv_up_cycle = 275; // add + add + mul - N/E
    // // double bconv_down_cycle = 359; // add + mul + mul - N/E

    // double total_time = 0;
    // double compute_time = 0;
    // double transfer_time = 0;
    // double N = 65536;
    // double loge = 16;
    // double ntt_cycle = 42008; // e=16
    // double intt_cycle = 37797;
    // double auto_cycle = 41484;
    // double add_cycle = 4117;
    // double mult_cycle = 4201;
    // double sub_cycle = 4114;
    // double bconv_up_cycle = add_cycle + add_cycle + mult_cycle - 2*(N/loge); // add + add + mul - 2(N/E)
    // double bconv_down_cycle = add_cycle + mult_cycle + mult_cycle - 2*(N/loge); // add + mul + mul - 2(N/E)

    // double pcie_bw = 15754;
    // double hbm_bw = 460000;
    // double sram_bw = 57609; // e=16
    // double poly_size = 0.5;
    // double ghz = 0.2;

    // compute_time = (cnt_ntt*ntt_cycle + cnt_intt*intt_cycle + cnt_auto*auto_cycle + cnt_add*add_cycle + cnt_mult*mult_cycle +
    //             cnt_sub*sub_cycle + cnt_bconv_up*bconv_up_cycle + cnt_bconv_down*bconv_down_cycle)/ghz;
    // transfer_time = ((cnt_copy_from_shadow_ocb_real + cnt_copy_from_shadow_hbm_real + cnt_copy_to_shadow_real)/pcie_bw +
    //             (cnt_copy_from_other_shadow1)/sram_bw +
    //             (cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3+cnt_copy_from_other_shadow4)/hbm_bw)*poly_size*1000000000;
    // total_time = compute_time + transfer_time;

    // std::cout << "compute_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << compute_time << "ns"<< std::endl;
    // std::cout << "transfer_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << transfer_time << "ns"<< std::endl;
    // std::cout << "total_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << total_time << "ns"<< std::endl;
    // std::cout << std::endl;

    // loge = 512;
    // ntt_cycle = 3454; // e=512
    // intt_cycle = 3343;
    // auto_cycle = 2578;
    // add_cycle = 149;
    // mult_cycle = 233;
    // sub_cycle = 146;
    // bconv_up_cycle = add_cycle + add_cycle + mult_cycle - 2*(N/loge); // add + add + mul - 2(N/E)
    // bconv_down_cycle = add_cycle + mult_cycle + mult_cycle - 2*(N/loge); // add + mul + mul - 2(N/E)

    // sram_bw = 1843488; // e=512

    // compute_time = (cnt_ntt*ntt_cycle + cnt_intt*intt_cycle + cnt_auto*auto_cycle + cnt_add*add_cycle + cnt_mult*mult_cycle +
    //             cnt_sub*sub_cycle + cnt_bconv_up*bconv_up_cycle + cnt_bconv_down*bconv_down_cycle)/ghz;
    // transfer_time = ((cnt_copy_from_shadow_ocb_real + cnt_copy_from_shadow_hbm_real + cnt_copy_to_shadow_real)/pcie_bw +
    //             (cnt_copy_from_other_shadow1)/sram_bw +
    //             (cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3+cnt_copy_from_other_shadow4)/hbm_bw)*poly_size*1000000000;
    // total_time = compute_time + transfer_time;

    // std::cout << "compute_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << compute_time << "ns"<< std::endl;
    // std::cout << "transfer_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << transfer_time << "ns"<< std::endl;
    // std::cout << "total_time(loge=" << loge << "): " << std::fixed << std::setprecision(0) << total_time << "ns"<< std::endl;
    // std::cout << std::endl;

    // std::ofstream file("evk_map.txt", std::ios::trunc);
    // if (file.is_open()) {
    //     for (const auto& pair : evk_map) {
    //         file << pair.first << " : " << pair.second << std::endl;
    //     }     
    // }        

    work_queue.finish();

    if (consumerThread.joinable()) {
        consumerThread.join();
    }

}

void print_stat_no_workqueue() {
    // std::cout << "print_stat" << std::endl;
    std::cout << "Total: " << cnt_copy_from_shadow_ocb_real+cnt_copy_from_shadow_hbm_real+cnt_copy_to_shadow_real+cnt_copy_from_other_shadow1+cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3+cnt_copy_from_other_shadow4 << std::endl;
    std::cout << "PCIe : " << cnt_copy_from_shadow_ocb_real+cnt_copy_from_shadow_hbm_real+cnt_copy_to_shadow_real << std::endl;
    std::cout << "SRAM : " << cnt_copy_from_other_shadow1 << std::endl;
    std::cout << "HBM  : " << cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3+cnt_copy_from_other_shadow4 << std::endl;
    std::cout << "cnt_copy_from_shadow: " << cnt_copy_from_shadow<< std::endl;
    std::cout << "ORIGIN <-- OCB          : " << cnt_copy_from_shadow_ocb_real<< std::endl;
    std::cout << "ORIGIN     <--     HBM  : " << cnt_copy_from_shadow_hbm_real<< std::endl;
    std::cout << "cnt_copy_to_shadow: " << cnt_copy_to_shadow<< std::endl;
    std::cout << "ORIGIN --> OCB          : " << cnt_copy_to_shadow_real<< std::endl;
    std::cout << "cnt_copy_from_other_shadow: " << cnt_copy_from_other_shadow<< std::endl;
    std::cout << "           OCB          : " << cnt_copy_from_other_shadow1<< std::endl;
    // std::cout << "           OCB <-- HBM  : " << cnt_copy_from_other_shadow2<< std::endl;
    // std::cout << "           OCB --> HBM  : " << cnt_copy_from_other_shadow3<< std::endl;
    std::cout << "  total    OCB --- HBM  : " << cnt_copy_from_other_shadow2+cnt_copy_from_other_shadow3<< std::endl;
    std::cout << "                   HBM  : " << cnt_copy_from_other_shadow4<< std::endl;
    std::cout << std::endl;
    // std::cout << "cnt_create_shadow: " << cnt_create_shadow<< std::endl;
    // std::cout << "root copy: " << cnt_copy_from_root_shadow_hbm_real << std::endl;
    // std::cout << "inv root copy: " << cnt_copy_from_inv_root_shadow_hbm_real << std::endl;
    // std::cout << "cnt_compute_implemented: " << cnt_compute_implemented<< std::endl;
    // std::cout << std::endl;
    // std::cout << "cnt_ntt: " << cnt_ntt << std::endl;
    // std::cout << "cnt_intt: " << cnt_intt << std::endl;
    // std::cout << "cnt_auto: " << cnt_auto << std::endl;
    // std::cout << "cnt_add: " << cnt_add << std::endl;
    // std::cout << "cnt_sub: " << cnt_sub << std::endl;
    // std::cout << "cnt_mult: " << cnt_mult << std::endl;
    // std::cout << "cnt_bconv_up: " << cnt_bconv_up << std::endl;
    // std::cout << "cnt_bconv_down: " << cnt_bconv_down << std::endl;
    // std::cout << std::endl;

    // std::cout << "elapsed_overhead: " << elapsed_overhead.count() << "ms" << std::endl;
    // std::cout << "elapsed_getwork: " << elapsed_getwork.count() << "ms" << std::endl;
    // std::cout << "elapsed_work: " << elapsed_work.count() << "ms" << std::endl;
    // std::cout << std::endl;

    // elapsed_overhead = std::chrono::duration<double, std::milli>(0.0);
    // elapsed_getwork = std::chrono::duration<double, std::milli>(0.0);
    // elapsed_work = std::chrono::duration<double, std::milli>(0.0);
}

void set_num_prallel_jobs(uint32_t num_parallel) {
    work_queue.setNumParallelJobs(num_parallel);
}

void print_elapsed_busywating_(){
    work_queue.print_elapsed_busywaiting();
}

void init_elapsed_busywaiting_(){
    work_queue.init_elapsed_busywaiting();
}

void inc_copy_from_shadow() {
    cnt_copy_from_shadow++;
}
void inc_copy_from_shadow_ocb_real()    {
    cnt_copy_from_shadow_ocb_real++;
}
void inc_copy_from_shadow_hbm_real()    {
    cnt_copy_from_shadow_hbm_real++;
}
void inc_copy_from_root_shadow_hbm_real()    {
    cnt_copy_from_root_shadow_hbm_real++;
}
void inc_copy_from_inv_root_shadow_hbm_real()    {
    cnt_copy_from_inv_root_shadow_hbm_real++;
}
void inc_copy_to_shadow()   {
    cnt_copy_to_shadow++;
}
void inc_copy_to_root_shadow()  {
    cnt_copy_to_root_shadow++;
}
void inc_copy_to_shadow_real(uint64_t addr)  {
    cnt_copy_to_shadow_real++;
    if(check_evk_set(addr)){
        cnt_copy_to_shadow_real_evk++;
    }
}
void inc_copy_from_other_shadow1()   {
    cnt_copy_from_other_shadow1++;
}
void inc_copy_from_other_shadow2()   {
    cnt_copy_from_other_shadow2++;
}
void inc_copy_from_other_shadow3()   {
    cnt_copy_from_other_shadow3++;
}
void inc_copy_from_other_shadow4()   {
    cnt_copy_from_other_shadow4++;
}
void inc_copy_from_other_shadow()   {
    cnt_copy_from_other_shadow++;
}
void inc_create_shadow()    {
    cnt_create_shadow++;
}
void inc_discard_shadow()    {
    cnt_discard_shadow++;
}
void inc_create_root_shadow()    {
    cnt_create_root_shadow++;
}
void inc_compute_not_implemented()    {
    cnt_compute_not_implemented++;
}
void inc_compute_implemented()    {
    cnt_compute_implemented++;
}

void inc_ntt(){
    cnt_ntt++;
}

void inc_intt(){
    cnt_intt++;
}

void inc_auto(){
    cnt_auto++;
}

void inc_add(){
    cnt_add++;
}

void inc_sub(){
    cnt_sub++;
}

void inc_mult(){
    cnt_mult++;
}

void inc_bconv_up(){
    cnt_bconv_up++;
}

void inc_bconv_down(){
    cnt_bconv_down++;
}


////////////////////////////////////////////////////////////
// make 192bit barrett parameter (SEAL-Like)
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