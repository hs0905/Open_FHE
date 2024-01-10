#include "utils/memory_tracking.h"

uint32_t cnt_cur_ocb_entries = 0;
uint32_t cnt_cur_hbm_entries = 0;
std::mutex ocb_entries_m;
std::deque<std::tuple<uint64_t,uint64_t,bool*>> shadow_tracking_array;
std::deque<std::tuple<uint64_t,uint64_t>> shadow_hbm_tracking_array;
std::unordered_map<uint64_t,std::deque<std::tuple<uint64_t,uint64_t,bool*>>::iterator> shadow_tracking_map;
std::unordered_map<uint64_t,std::deque<std::tuple<uint64_t,uint64_t>>::iterator> shadow_hbm_tracking_map;
// std::deque<std::vector<std::tuple<uint64_t,uint64_t,bool*>>::iterator> ocb_freelist;
// std::deque<std::vector<std::tuple<uint64_t,uint64_t>>::iterator> hbm_freelist;
// std::deque<std::vector<std::tuple<uint64_t,uint64_t,bool*>>::iterator> ocb_fifolist;
// std::deque<std::vector<std::tuple<uint64_t,uint64_t>>::iterator> hbm_fifolist;
bool false_flag = false;


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

void initialize_memory_tracking(){
    // for(size_t i=0; i<OCB_ENTRIES_NUM; i++){
    //     shadow_tracking_array.push_front(std::make_tuple(0,0,&false_flag));
    //     cnt_cur_ocb_entries = OCB_ENTRIES_NUM;
    // }
    
    // for(size_t i=0; i<HBM_ENTRIES_NUM; i++){
    //     shadow_hbm_tracking_array.push_front(std::make_tuple(0,0));
    //     cnt_cur_hbm_entries = cnt_cur_hbm_entries;
    // }
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

void print_shadow_tracking_array(){
    std::cout << "----------array------------" << std::endl;
    for(const auto& entry : shadow_tracking_array){
        std::cout << std::get<1>(entry) << " ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------" << std::endl;
}

void print_shadow_tracking_map(){
    std::cout << "----------map------------" << std::endl;
    for(const auto& entry : shadow_tracking_map){
        std::cout << entry.first << " ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------" << std::endl;

}

void insert_shadow_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr, bool &ongoing_flag){
    shadow_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr,&ongoing_flag));
    // shadow_tracking_map.insert(std::make_pair(m_values_shadow_addr,shadow_tracking_array.begin()));
    shadow_tracking_map[m_values_shadow_addr] = shadow_tracking_array.begin();

    // std::cout << "insert" << std::endl;
    // std::cout << m_values_shadow_addr << std::endl;
    // print_shadow_tracking_array();
    // print_shadow_tracking_map();

    inc_cur_ocb_entries();
}

void insert_shadow_hbm_tracking_array(uint64_t m_values_addr, uint64_t m_values_shadow_addr){
    shadow_hbm_tracking_array.push_front(std::make_tuple(m_values_addr,m_values_shadow_addr));
    // shadow_hbm_tracking_map.insert(std::make_pair(m_values_shadow_addr,shadow_hbm_tracking_array.begin()));
    shadow_hbm_tracking_map[m_values_shadow_addr] = shadow_hbm_tracking_array.begin();
    inc_cur_hbm_entries();
}

std::tuple<uint64_t,uint64_t,bool*> evict_shadow_tracking_array(){
    auto tmp = shadow_tracking_array.back();
    if(std::get<1>(tmp) == 0){
        shadow_tracking_array.pop_back();
        dec_cur_ocb_entries();
        return tmp;
    }
    while(*std::get<2>(tmp)==true){
        shadow_tracking_array.pop_back();
        shadow_tracking_array.push_front(tmp);
        shadow_tracking_map[std::get<1>(tmp)] = shadow_tracking_array.begin();
        tmp = shadow_tracking_array.back();
    }
    auto it = shadow_tracking_map.find(std::get<1>(tmp));
    if(it == shadow_tracking_map.end()){
        // std::cout << shadow_tracking_array.size() << " " << shadow_tracking_map.size() << std::endl;
        // std::cout << std::get<1>(tmp) << std::endl;
        // print_shadow_tracking_array();
        // print_shadow_tracking_map();
        printf("wrong evict_shadow_tracking_array\n");
        while(1){
            usleep(1000);
        }
    }
    shadow_tracking_map.erase(it);
    shadow_tracking_array.pop_back();

    // std::cout << "evict" << std::endl;
    // std::cout << std::get<1>(tmp) << std::endl;
    // print_shadow_tracking_array();
    // print_shadow_tracking_map();
    
    dec_cur_ocb_entries();
    return tmp;
    // std::tuple<uint64_t,uint64_t,bool*> tmp;
    // for(auto iter=shadow_tracking_array.rbegin(); iter != shadow_tracking_array.rend(); iter++){
    //     if(*std::get<2>(*iter)==false){
    //         tmp = *iter;
    //         shadow_tracking_array.erase(iter.base()-1);
    //         dec_cur_ocb_entries();
    //         return tmp;
    //     }
    // }

    // printf("wrong evict_shadow_tracking_array\n");
    // return tmp;
}

std::tuple<uint64_t,uint64_t> evict_shadow_hbm_tracking_array(){
    auto tmp = shadow_hbm_tracking_array.back();
    if(std::get<1>(tmp) == 0){
        shadow_tracking_array.pop_back();
        dec_cur_ocb_entries();
        return tmp;
    }
    auto it = shadow_hbm_tracking_map.find(std::get<1>(tmp));
    if(it == shadow_hbm_tracking_map.end()){
        printf("wrong evict_shadow_hbm_tracking_array\n");
        while(1){
            usleep(1000);
        }
    }
    shadow_hbm_tracking_map.erase(it);
    shadow_hbm_tracking_array.pop_back();
    dec_cur_hbm_entries();
    return tmp;
    // std::tuple<uint64_t,uint64_t> tmp = shadow_hbm_tracking_array.back();
    // shadow_hbm_tracking_array.pop_back();
    // dec_cur_hbm_entries();
    // return tmp;
}

std::tuple<uint64_t,uint64_t,bool*> select_shadow_tracking_array(uint64_t m_values_shadow_addr){
    auto it = shadow_tracking_map.find(m_values_shadow_addr);
    if(it == shadow_tracking_map.end()) std::cout << "wrong select_shadow_tracking_array" << std::endl;
    auto tmp = *(it->second);
    // shadow_tracking_array.erase(it->second);
    *(it->second) = std::make_tuple(0,0,nullptr);
    shadow_tracking_map.erase(it);
    dec_cur_ocb_entries();
    return tmp;
    // std::tuple<uint64_t,uint64_t,bool*> tmp;
    // for(auto iter=shadow_tracking_array.rbegin(); iter != shadow_tracking_array.rend(); iter++){
    //     if(std::get<1>(*iter)==m_values_shadow_addr){
    //         tmp = *iter;
    //         shadow_tracking_array.erase(iter.base()-1);
    //         dec_cur_ocb_entries();
    //         return tmp;
    //     }
    // }

    // printf("wrong evict_shadow_hbm_tracking_array\n");
    // return tmp;
}

std::tuple<uint64_t,uint64_t> select_shadow_hbm_tracking_array(uint64_t m_values_shadow_addr){
    auto it = shadow_hbm_tracking_map.find(m_values_shadow_addr);
    if(it == shadow_hbm_tracking_map.end()) std::cout << "wrong select_shadow_hbm_tracking_array" << std::endl;
    auto tmp = *(it->second);
    // shadow_hbm_tracking_array.erase(it->second);
    *(it->second) = std::make_tuple(0,0);
    shadow_hbm_tracking_map.erase(it);
    dec_cur_hbm_entries();
    return tmp;
    // std::tuple<uint64_t,uint64_t> tmp;
    // for(auto iter=shadow_hbm_tracking_array.rbegin(); iter != shadow_hbm_tracking_array.rend(); iter++){
    //     if(std::get<1>(*iter)==m_values_shadow_addr){
    //         tmp = *iter;
    //         shadow_hbm_tracking_array.erase(iter.base()-1);
    //         dec_cur_hbm_entries();
    //         return tmp;
    //     }
    // }
    
    // printf("wrong select_shadow_hbm_tracking_array\n");
    // while(1){
    //     usleep(1000);
    // }
    // return tmp;
}

void clean_shadow_tracking_array(uint64_t m_values_shadow_addr){
    ocb_entries_m.lock();
    auto it = shadow_tracking_map.find(m_values_shadow_addr);
    if(it != shadow_tracking_map.end()){
        // shadow_tracking_array.erase(it->second);
        *(it->second) = std::make_tuple(0,0,nullptr);
        shadow_tracking_map.erase(it);
        dec_cur_ocb_entries();
        ocb_entries_m.unlock();
        return;
    }
    auto it_ = shadow_hbm_tracking_map.find(m_values_shadow_addr);
    if(it_ != shadow_hbm_tracking_map.end()){
        // shadow_hbm_tracking_array.erase(it_->second);
        *(it_->second) = std::make_tuple(0,0);
        shadow_hbm_tracking_map.erase(it_);
        dec_cur_hbm_entries();
        ocb_entries_m.unlock();
        return;
    }

    ocb_entries_m.unlock();
    return;
    
    // ocb_entries_m.lock();
    // auto shadow_remove_if_result = std::remove_if(
    //     shadow_tracking_array.begin(),
    //     shadow_tracking_array.end(),
    //     [m_values_shadow_addr](const std::tuple<uint64_t, uint64_t, bool*>& t) {
    //         return std::get<1>(t) == m_values_shadow_addr;
    //     }
    // );

    // if (shadow_remove_if_result != shadow_tracking_array.end()){
    //     shadow_tracking_array.erase(shadow_remove_if_result, shadow_tracking_array.end());
    //     dec_cur_ocb_entries();
    //     ocb_entries_m.unlock();
    //     return;
    // }
    // auto shadow_hbm_remove_if_result = std::remove_if(
    //     shadow_hbm_tracking_array.begin(),
    //     shadow_hbm_tracking_array.end(),
    //     [m_values_shadow_addr](const std::tuple<uint64_t, uint64_t>& t) {
    //         return std::get<1>(t) == m_values_shadow_addr;
    //     }
    // );

    // if (shadow_hbm_remove_if_result == shadow_hbm_tracking_array.end()){
    //     ocb_entries_m.unlock();
    //     return;
    // }
    // else{
    //     shadow_hbm_tracking_array.erase(shadow_hbm_remove_if_result, shadow_hbm_tracking_array.end());
    //     dec_cur_hbm_entries();
    //     ocb_entries_m.unlock();
    //     return;
    // }
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