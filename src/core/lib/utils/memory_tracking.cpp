#include "utils/memory_tracking.h"

uint32_t cnt_cur_ocb_entries = 0;
uint32_t cnt_cur_hbm_entries = 0;
std::mutex ocb_entries_m;
/*  This is memory tracking structure.
    For performance, information about the shadow buffer is stored in deque and unordered maps.
    Origin memory address, shadow memory address,ongoing flag are
    in deque 'shadow_tracking_array' 'shadow_hbm_tracking_array'.
    The map was used for washing for performance,
    and it is possible to find where a particular buffer is in the memory tracking structure at once.
*/
std::deque<std::tuple<uint64_t,uint64_t,bool*>> shadow_tracking_array;
std::deque<std::tuple<uint64_t,uint64_t>> shadow_hbm_tracking_array;
std::unordered_map<uint64_t,std::deque<std::tuple<uint64_t,uint64_t,bool*>>::iterator> shadow_tracking_map;
std::unordered_map<uint64_t,std::deque<std::tuple<uint64_t,uint64_t>>::iterator> shadow_hbm_tracking_map;
bool false_flag = false;
std::unordered_set<uint64_t> evk_set;

bool check_evk_set(uint64_t evk_addr){
    if(evk_set.find(evk_addr) == evk_set.end()){
        return false;
    }
    else{
        std::cout << "evk b vector" << std::endl;
        return true;
    }
}

void insert_evk_set(uint64_t evk_addr){
    evk_set.insert(evk_addr);
}

/* print hardware configuration stat */
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

/* increase current on-chip buffer entry number */
void inc_cur_ocb_entries(){
    cnt_cur_ocb_entries++;
}
/* decrease current on-chip buffer entry number */
void dec_cur_ocb_entries(){
    cnt_cur_ocb_entries--;
}
/* increase current HBM entry number */
void inc_cur_hbm_entries(){
    cnt_cur_hbm_entries++;
}
/* decrease current HBM entry number */
void dec_cur_hbm_entries(){
    cnt_cur_hbm_entries--;
}

/* Enter information in the memory tracking structure when the shadow is newly created */
void insert_shadow_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr, bool &ongoing_flag){
    shadow_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr,&ongoing_flag));
    shadow_tracking_map[m_values_shadow_addr] = shadow_tracking_array.begin();

    inc_cur_ocb_entries();
}
void insert_shadow_hbm_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr){
    shadow_hbm_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr));
    shadow_hbm_tracking_map[m_values_shadow_addr] = shadow_hbm_tracking_array.begin();
    inc_cur_hbm_entries();
}

/* for 'discard_shadow' fucntion, when on-chip-buffer is full,
    choose victim (FIFO Rule) */
std::tuple<uint64_t,uint64_t,bool*> evict_shadow_tracking_array(){
    auto tmp = shadow_tracking_array.back();

    while(*std::get<2>(tmp)==true || std::get<1>(tmp) == 0){
        shadow_tracking_array.pop_back();
        if(*std::get<2>(tmp)==true){
            shadow_tracking_array.push_front(tmp);
            shadow_tracking_map[std::get<1>(tmp)] = shadow_tracking_array.begin();
        }
        tmp = shadow_tracking_array.back();
    }
    auto it = shadow_tracking_map.find(std::get<1>(tmp));
    if(it == shadow_tracking_map.end()){ // for debugging
        printf("wrong evict_shadow_tracking_array\n");
        while(1){
            usleep(1000);
        }
    }
    shadow_tracking_map.erase(it);
    shadow_tracking_array.pop_back();
 
    dec_cur_ocb_entries();
    return tmp;
}
/* for 'discard_shadow' fucntion, HBM is full,
    choose victim (FIFO Rule) */
std::tuple<uint64_t,uint64_t> evict_shadow_hbm_tracking_array(){
    auto tmp = shadow_hbm_tracking_array.back();
    while(std::get<1>(tmp) == 0){
        shadow_hbm_tracking_array.pop_back();
        tmp = shadow_hbm_tracking_array.back();
    }
    auto it = shadow_hbm_tracking_map.find(std::get<1>(tmp));
    if(it == shadow_hbm_tracking_map.end()){ // for debugging
        printf("wrong evict_shadow_hbm_tracking_array\n");
        while(1){
            usleep(1000);
        }
    }
    shadow_hbm_tracking_map.erase(it);
    shadow_hbm_tracking_array.pop_back();
    dec_cur_hbm_entries();
    return tmp;
}

/* not use */
std::tuple<uint64_t,uint64_t,bool*> select_shadow_tracking_array(uint64_t m_values_shadow_addr){
    auto it = shadow_tracking_map.find(m_values_shadow_addr);
    if(it == shadow_tracking_map.end()) std::cout << "wrong select_shadow_tracking_array" << std::endl;
    auto tmp = *(it->second);
    *(it->second) = std::make_tuple(0,0,&false_flag);
    shadow_tracking_map.erase(it);
    dec_cur_ocb_entries();
    return tmp;
}

/* for copy_to_shadow function, when shadow buffer in HBM
    we need to find shadow buffer information in memory tracking structure */
std::tuple<uint64_t,uint64_t> select_shadow_hbm_tracking_array(uint64_t m_values_shadow_addr){
    auto it = shadow_hbm_tracking_map.find(m_values_shadow_addr);
    if(it == shadow_hbm_tracking_map.end()) std::cout << "wrong select_shadow_hbm_tracking_array" << std::endl;
    auto tmp = *(it->second);
    *(it->second) = std::make_tuple(0,0);
    shadow_hbm_tracking_map.erase(it);
    dec_cur_hbm_entries();
    return tmp;
}

/* Openfhe automatically releases memory because it uses smart pointers.
    Therefore, when the memory for the polynomial is released,
    Remove the memory information from the memory tracking data structure */
void clean_shadow_tracking_array(uint64_t m_values_shadow_addr){
    ocb_entries_m.lock();
    auto it = shadow_tracking_map.find(m_values_shadow_addr);
    if(it != shadow_tracking_map.end()){
        *(it->second) = std::make_tuple(0,0,&false_flag);
        shadow_tracking_map.erase(it);
        dec_cur_ocb_entries();
        ocb_entries_m.unlock();
        return;
    }
    auto it_ = shadow_hbm_tracking_map.find(m_values_shadow_addr);
    if(it_ != shadow_hbm_tracking_map.end()){
        *(it_->second) = std::make_tuple(0,0);
        shadow_hbm_tracking_map.erase(it_);
        dec_cur_hbm_entries();
        ocb_entries_m.unlock();
        return;
    }

    ocb_entries_m.unlock();
    return;
}

/* for checking that the on-chip buffer is full */
bool check_full_ocb_entries(){
    if(cnt_cur_ocb_entries >= OCB_ENTRIES_NUM){
        return true;
    }
    else{
        return false;
    }
}

/* for checking that the HBM is full */
bool check_full_hbm_entries(){
    if(cnt_cur_hbm_entries >= HBM_ENTRIES_NUM){
        return true;
    }
    else{
        return false;
    }
}