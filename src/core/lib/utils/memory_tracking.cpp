#include "utils/memory_tracking.h"

uint32_t cnt_cur_ocb_entries = 0;
uint32_t cnt_cur_hbm_entries = 0;
std::mutex ocb_entries_m;
std::deque<std::tuple<uint64_t,uint64_t,bool*>> shadow_tracking_array;
std::deque<std::tuple<uint64_t,uint64_t>> shadow_hbm_tracking_array;

void print_memory_stat(){
    std::cout << "print_memroy_stat" << std::endl;
    std::cout << "FPGA_N: " << FPGA_N<< std::endl;
    std::cout << "OCB_MB: " << OCB_MB<< std::endl;
    std::cout << "HBM_GB: " << HBM_GB<< std::endl;
    std::cout << "POLY_SIZE: " << POLY_SIZE<< std::endl;
    std::cout << "MB_TO_BYTES: " << MB_TO_BYTES<< std::endl;
    std::cout << "GB_TO_MB: " << GB_TO_MB<< std::endl;
    std::cout << "MB_TO_ENTRIES_NUM: " << MB_TO_ENTRIES_NUM<< std::endl;
    std::cout << "OCB_ENTRIES_NUM: " << OCB_ENTRIES_NUM<< std::endl;
    std::cout << "HBM_ENTRIES_NUM: " << HBM_ENTRIES_NUM<< std::endl;
    std::cout << "cnt_cur_ocb_entries: " << cnt_cur_ocb_entries<< std::endl;
    std::cout << "cnt_cur_hbm_entries: " << cnt_cur_hbm_entries<< std::endl;
}

void inc_cur_ocb_entries(){
    cnt_cur_ocb_entries++;
}

void dec_cur_ocb_entries(){
    cnt_cur_ocb_entries--;
}

void inc_cur_hbm_entries(){
    cnt_cur_hbm_entries++;
}

void dec_cur_hbm_entries(){
    cnt_cur_hbm_entries--;
}

void insert_shadow_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr, bool &ongoing_flag){
    shadow_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr,&ongoing_flag));
    inc_cur_ocb_entries();
}

void insert_shadow_hbm_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr){
    shadow_hbm_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr));
    inc_cur_hbm_entries();
}

std::tuple<uint64_t,uint64_t,bool*> evict_shadow_tracking_array(){
    std::tuple<uint64_t,uint64_t,bool*> tmp;
    for(auto iter=shadow_tracking_array.rbegin(); iter != shadow_tracking_array.rend(); iter++){
        if(*std::get<2>(*iter)==false){
            tmp = *iter;
            shadow_tracking_array.erase(iter.base()-1);
            dec_cur_ocb_entries();
            return tmp;
        }
    }

    printf("wrong evict_shadow_tracking_array\n");
    return tmp;
}

std::tuple<uint64_t,uint64_t> evict_shadow_hbm_tracking_array(){
    std::tuple<uint64_t,uint64_t> tmp = shadow_hbm_tracking_array.back();
    shadow_hbm_tracking_array.pop_back();
    dec_cur_hbm_entries();
    return tmp;
}

std::tuple<uint64_t,uint64_t,bool*> select_shadow_tracking_array(uint64_t m_values_shadow_addr){
    std::tuple<uint64_t,uint64_t,bool*> tmp;
    for(auto iter=shadow_tracking_array.rbegin(); iter != shadow_tracking_array.rend(); iter++){
        if(std::get<1>(*iter)==m_values_shadow_addr){
            tmp = *iter;
            shadow_tracking_array.erase(iter.base()-1);
            dec_cur_ocb_entries();
            return tmp;
        }
    }

    printf("wrong evict_shadow_hbm_tracking_array\n");
    return tmp;
}

std::tuple<uint64_t,uint64_t> select_shadow_hbm_tracking_array(uint64_t m_values_shadow_addr){
    std::tuple<uint64_t,uint64_t> tmp;
    for(auto iter=shadow_hbm_tracking_array.rbegin(); iter != shadow_hbm_tracking_array.rend(); iter++){
        if(std::get<1>(*iter)==m_values_shadow_addr){
            tmp = *iter;
            shadow_hbm_tracking_array.erase(iter.base()-1);
            dec_cur_hbm_entries();
            return tmp;
        }
    }
    
    printf("wrong select_shadow_hbm_tracking_array\n");
    while(1){
        usleep(1000);
    }
    return tmp;
}

void clean_shadow_tracking_array(uint64_t m_values_shadow_addr){
    ocb_entries_m.lock();
    auto shadow_remove_if_result = std::remove_if(
        shadow_tracking_array.begin(),
        shadow_tracking_array.end(),
        [m_values_shadow_addr](const std::tuple<uint64_t, uint64_t, bool*>& t) {
            return std::get<1>(t) == m_values_shadow_addr;
        }
    );

    if (shadow_remove_if_result != shadow_tracking_array.end()){
        shadow_tracking_array.erase(shadow_remove_if_result, shadow_tracking_array.end());
        dec_cur_ocb_entries();
        ocb_entries_m.unlock();
        return;
    }
    auto shadow_hbm_remove_if_result = std::remove_if(
        shadow_hbm_tracking_array.begin(),
        shadow_hbm_tracking_array.end(),
        [m_values_shadow_addr](const std::tuple<uint64_t, uint64_t>& t) {
            return std::get<1>(t) == m_values_shadow_addr;
        }
    );

    if (shadow_hbm_remove_if_result == shadow_hbm_tracking_array.end()){
        ocb_entries_m.unlock();
        return;
    }
    else{
        shadow_hbm_tracking_array.erase(shadow_hbm_remove_if_result, shadow_hbm_tracking_array.end());
        dec_cur_hbm_entries();
        ocb_entries_m.unlock();
        return;
    }
}

bool check_full_ocb_entries(){
    if(cnt_cur_ocb_entries >= OCB_ENTRIES_NUM){
        return true;
    }
    else{
        return false;
    }
}

bool check_full_hbm_entries(){
    if(cnt_cur_hbm_entries >= HBM_ENTRIES_NUM){
        return true;
    }
    else{
        return false;
    }
}