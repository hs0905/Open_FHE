//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2022, NJIT, Duality Technologies Inc. and other contributors
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

Example for CKKS bootstrapping with sparse packing

*/

#define PROFILE

#include "openfhe.h"
#include <thread>
#include <cstdlib>

using namespace lbcrypto;
using namespace std;

usint Initialize(CryptoContext<DCRTPoly>& cryptoContext, KeyPair<lbcrypto::DCRTPoly>& keyPair, uint32_t numSlots);
void BootstrapExample(CryptoContext<DCRTPoly> cryptoContext, KeyPair<lbcrypto::DCRTPoly> keyPair, uint32_t numSlots, usint depth);

void init_stat();
void init_stat_no_workqueue();
void print_stat();
void print_stat_no_workqueue();
void print_memory_stat();
void set_num_prallel_jobs(uint32_t num_parallel);
void print_elapsed_busywating_();
void init_elapsed_busywaiting_();

extern uint32_t FPGA_N;
extern uint32_t OCB_MB;
extern uint32_t HBM_GB;
extern uint32_t BOOT_SCHEME;
extern uint32_t cnt_copy_from_shadow_ocb_real;
extern uint32_t cnt_copy_from_shadow_hbm_real;
extern uint32_t cnt_copy_to_shadow_real;
extern uint32_t cnt_copy_from_other_shadow1;
extern uint32_t cnt_copy_from_other_shadow2;
extern uint32_t cnt_copy_from_other_shadow3;
extern uint32_t cnt_copy_from_other_shadow4;
extern size_t AUXMODSIZE;

extern uint32_t cnt_ntt;
extern uint32_t cnt_intt;
extern uint32_t cnt_auto;
extern uint32_t cnt_add;
extern uint32_t cnt_sub;
extern uint32_t cnt_mult;
extern uint32_t cnt_bconv_up;
extern uint32_t cnt_bconv_down;

extern bool compute_flag;
extern void clear_tracking_object();

void decrypt_and_print(const Ciphertext<DCRTPoly> &cipher, CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys, size_t front) {
    Plaintext plain;
		cc->Decrypt(keys.secretKey, cipher, &plain);
    
		auto rtn_vec = plain->GetRealPackedValue();
    cout << "( "; 
    for (size_t i = 0; i < front; i++) cout << rtn_vec[i] << ", ";
    cout << "... ";

    cout << ") Estimated precision: " << plain->GetLogPrecision() << " bits" << endl;
}

void print_moduli_chain(const DCRTPoly& poly){
    int num_primes = poly.GetNumOfElements();
    double total_bit_len = 0.0;
    for (int i = 0; i < num_primes; i++) {
        auto qi = poly.GetParams()->GetParams()[i]->GetModulus();
        std::cout << "q_" << i << ": " 
                    << qi
                    << ",  log q_" << i <<": " << log(qi.ConvertToDouble()) / log(2)
                    << std::endl;
        total_bit_len += log(qi.ConvertToDouble()) / log(2);
    }   
    std::cout << "Total bit length: " << total_bit_len << std::endl;
}

int main(int argc, char* argv[]){
    // FPGA_N = atoi(argv[1]);
    OCB_MB = atoi(argv[1]); // On-Chip Buffer Size
    HBM_GB = 16; 
    // BOOT_SCHEME = 1; // Normal: 1, on-the-fly only evk b: 2, on-the-fly all: 3
    uint32_t BOOT_NUM = atoi(argv[2]); // Number of bootstrapping : 1~4
    uint32_t BATSEQ = atoi(argv[3]); // Batching: 1, Sequential: 2

    // std::cout << "OCB_MB: " << OCB_MB << " BOOT_SCHEME: " << BOOT_SCHEME << " BOOT_NUM: " << BOOT_NUM << " BATSEQ: " << BATSEQ << std::endl;

    init_stat();
    print_memory_stat();
    uint32_t numSlots = 1<<14; // Fully-Packed (numSlots = (logN)/2) / also logN/4+1 < slot -> fully packed

    // SEAL and bootstrapping setting (Resnet-20 Bootstrapping Setting)
	// long boundary_K = 25;
	// long boot_deg = 59;
    // long scale_factor = 2;
    // long inverse_deg = 1; 
	long logN = 16;
	long loge = 10; 
	// long logn = 15;		// full slots
	// long logn_1 = 14;	// sparse slots
	// long logn_2 = 13;
	// long logn_3 = 12;
	int logp = 59; //59; // 53; //46; 46 results in math error exception.
	int logq = 60;
	// int log_special_prime = 51;
    // int log_integer_part = logq - logp - loge + 5;
	int remaining_level = 16; // Calculation required

    std::vector<uint32_t> levelBudget = {4, 4}; // {4, 4};
	std::vector<uint32_t> bsgsDim = {0, 0}; // {0, 0}
	SecretKeyDist secretKeyDist = /*SPARSE_TERNARY*/UNIFORM_TERNARY;
	int boot_level = FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist) + 2;
    // std::cout << "boot_level: " << boot_level << std::endl;

    // int total_level = 16 + boot_level;
    int total_level = remaining_level + boot_level;
	std::cout << "boot_level: " << boot_level << std::endl;
	std::cout << "remaining_level: " << remaining_level << std::endl;
	std::cout << "total_level: " << total_level << std::endl;

    CCParams<CryptoContextCKKSRNS> parameters;
    
	parameters.SetSecretKeyDist(secretKeyDist);
    // for(size_t i=0;;i++){
    //     if(FPGA_N == 1<<i){
    //         logN = i; 
    //         break;
    //     }
    // }

    // std::cout << "logN: " << logN << std::endl;

	parameters.SetRingDim(1<<logN);
	parameters.SetMultiplicativeDepth(total_level);
    // std::cout << "Enter default modulus bits(q0 ~ qL): "<< std::endl;
    // std::cin >> logp;
	parameters.SetScalingModSize(logp);
    // std::cout << "Enter special modulus bits(p0 ~ pk-1): "<< std::endl;
    // std::cin >> logq;
    parameters.SetFirstModSize(logq);
    AUXMODSIZE = logq;
	parameters.SetSecurityLevel(/*HEStd_128_quantum*/HEStd_NotSet);

    uint32_t dnum = atoi(argv[4]);
	parameters.SetScalingTechnique(FLEXIBLEAUTO);
	parameters.SetNumLargeDigits(dnum);
    parameters.SetKeySwitchTechnique(HYBRID);

	CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
  
	std::cout << parameters << std::endl;
	
	// Enable the features that you wish to use
	cc->Enable(PKE);
	cc->Enable(KEYSWITCH);
	cc->Enable(LEVELEDSHE);
	cc->Enable(ADVANCEDSHE);
	cc->Enable(FHE);
	std::cout << "CKKS scheme is using ring dimension " << cc->GetRingDimension() << std::endl << std::endl;

    auto keys = cc->KeyGen();
    const std::vector<DCRTPoly>& ckks_pk = keys.publicKey->GetPublicElements();
    print_moduli_chain(ckks_pk[0]);

    cc->EvalMultKeyGen(keys.secretKey);

    const auto cryptoParamsCKKS =
        std::dynamic_pointer_cast<CryptoParametersCKKSRNS>(
            cc->GetCryptoParameters());
        std::cout << "\nScaling factors of levels: " << std::endl;
    for (uint32_t i = 0; i < parameters.GetMultiplicativeDepth(); i++) {
      std::cout << "Level " << i << ": "
                << cryptoParamsCKKS->GetScalingFactorReal(i) << std::endl;
    }

    // // additional rotation kinds for CNN
	// std::vector<int> rotation_kinds = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33
	// 	,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55
	// 	,56
	// 	,57,58,59,60,61
	// 	,62,63,64,66,84,124,128,132,256,512,959,960,990,991,1008
	// 	,1023,1024,1036,1064,1092,1952,1982,1983,2016,2044,2047,2048,2072,2078,2100,3007,3024,3040,3052,3070,3071,3072,3080,3108,4031
	// 	,4032,4062,4063,4095,4096,5023,5024,5054,5055,5087,5118,5119,5120,6047,6078,6079,6111,6112,6142,6143,6144,7071,7102,7103,7135
	// 	,7166,7167,7168,8095,8126,8127,8159,8190,8191,8192,9149,9183,9184,9213,9215,9216,10173,10207,10208,10237,10239,10240,11197,11231
	// 	,11232,11261,11263,11264,12221,12255,12256,12285,12287,12288,13214,13216,13246,13278,13279,13280,13310,13311,13312,14238,14240
	// 	,14270,14302,14303,14304,14334,14335,15262,15264,15294,15326,15327,15328,15358,15359,15360,16286,16288,16318,16350,16351,16352
	// 	,16382,16383,16384,17311,17375,18335,18399,18432,19359,19423,20383,20447,20480,21405,21406,21437,21469,21470,21471,21501,21504
	// 	,22429,22430,22461,22493,22494,22495,22525,22528,23453,23454,23485,23517,23518,23519,23549,24477,24478,24509,24541,24542,24543
	// 	,24573,24576,25501,25565,25568,25600,26525,26589,26592,26624,27549,27613,27616,27648,28573,28637,28640,28672,29600,29632,29664
	// 	,29696,30624,30656,30688,30720,31648,31680,31712,31743,31744,31774,32636,32640,32644,32672,32702,32704,32706,32735
	// 	,32736,32737,32759,32760,32761,32762,32763,32764,32765,32766,32767		
	// };

    // cout << "Adding Bootstrapping Keys..." << endl;
	// vector<int> gal_steps_vector;
	// gal_steps_vector.push_back(0);
	// for(int i=0; i<logN-1; i++) gal_steps_vector.push_back((1 << i));
	// for(auto rot: rotation_kinds)
	// {
	// 	if(find(gal_steps_vector.begin(), gal_steps_vector.end(), rot) == gal_steps_vector.end()) gal_steps_vector.push_back(rot);
	// } 

	// cout << "EvalRotateKeyGen+" << endl;
	// for(auto rot: gal_steps_vector)
	// {
	// 	vector<int> r;
	// 	r.push_back(rot);
	// 	cc->EvalRotateKeyGen(keys.secretKey, r);
	// }
	// cout << "EvalRotateKeyGen-" << endl;

	cout << "EvalBootstrapSetup+" << endl;
	cc->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots);
	// // cc->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots>>1);
	// // cc->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots>>2);
	cout << "EvalBootstrapSetup-" << endl;

	cout << "EvalBootstrapKeyGen+" << endl;
	cc->EvalBootstrapKeyGen(keys.secretKey, numSlots);
	// // cc->EvalBootstrapKeyGen(keys.secretKey, numSlots>>1);
	// // cc->EvalBootstrapKeyGen(keys.secretKey, numSlots>>2);
	cout << "EvalBootstrapKeyGen-" << endl;

    init_stat_no_workqueue();
    init_elapsed_busywaiting_();
    std::vector<std::thread> threads;

    if(BATSEQ == 2){ // Sequential

        for(size_t i=0; i<BOOT_NUM; i++)
        {
            BootstrapExample(cc, keys, numSlots, total_level);
        }
    }
    else{ // Batching
        set_num_prallel_jobs(BOOT_NUM);

        for (int i = 0; i < BOOT_NUM; ++i) {
        threads.emplace_back(BootstrapExample, cc, keys, numSlots, total_level);
        }

        for (auto& t : threads) {
            t.join();
        }
        // switch(BOOT_NUM){
        //     case 1:
        //         BootstrapExample(cc, keys, numSlots, total_level);
        //         break;
        //     case 2:{
        //         std::thread t1(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t2(BootstrapExample, cc, keys, numSlots, total_level); 
        //         t1.join();
        //         t2.join();
        //         break;
        //     }
        //     case 3:{
        //         std::thread t1(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t2(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t3(BootstrapExample, cc, keys, numSlots, total_level); 
        //         t1.join();
        //         t2.join();
        //         t3.join();
        //         break;
        //     }
        //     case 4:{
        //         std::thread t1(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t2(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t3(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t4(BootstrapExample, cc, keys, numSlots, total_level); 
        //         t1.join();
        //         t2.join();
        //         t3.join();
        //         t4.join();
        //         break;
        //     }
        //     case 5:{
        //         std::thread t1(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t2(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t3(BootstrapExample, cc, keys, numSlots, total_level); 
        //         std::thread t4(BootstrapExample, cc, keys, numSlots, total_level);
        //         std::thread t5(BootstrapExample, cc, keys, numSlots, total_level);
        //         t1.join();
        //         t2.join();
        //         t3.join();
        //         t4.join();
        //         t5.join();
        //         break;
        //     }
        //     default:
        //         break;
        // }
    }
    print_stat();
    print_elapsed_busywating_();
    print_memory_stat();

    // std::cout << "logN: " << logN << " logp: " << logp << " logq: " << logq << " dnum: " << dnum << std::endl;
    std::ofstream file("boot_result2.csv", std::ios::app);
    if (file.is_open()) {
        file << OCB_MB << ", " << BOOT_NUM << ", " <<
            /*BATSEQ << ", " <<*/ dnum
            << ", " << cnt_copy_from_shadow_ocb_real
            << ", " << cnt_copy_from_shadow_hbm_real
            << ", " << cnt_copy_to_shadow_real
            << ", " << cnt_copy_from_other_shadow1
            << ", " << cnt_copy_from_other_shadow2
            << ", " << cnt_copy_from_other_shadow3
            << ", " << cnt_copy_from_other_shadow4
            << ", " << cnt_ntt << ", " << cnt_intt << ", " << cnt_auto
            << ", " << cnt_add << ", " << cnt_sub << ", " << cnt_mult 
            << ", " << cnt_bconv_up << ", " << cnt_bconv_down << std::endl;
        file.close();
    }

}

void BootstrapExample(CryptoContext<DCRTPoly> cryptoContext, KeyPair<lbcrypto::DCRTPoly> keyPair, uint32_t numSlots, usint depth) {
    compute_flag = true;
    // Step 4: Encoding and encryption of inputs
    // Generate random input
    std::vector<double> x;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    for (size_t i = 0; i < numSlots; i++) {
        x.push_back(dis(gen));
    }

    // Encoding as plaintexts
    // We specify the number of slots as numSlots to achieve a performance improvement.
    // We use the other default values of depth 1, levels 0, and no params.
    // Alternatively, you can also set batch size as a parameter in the CryptoContext as follows:
    // parameters.SetBatchSize(numSlots);
    // Here, we assume all ciphertexts in the cryptoContext will have numSlots slots.
    // We start with a depleted ciphertext that has used up all of its levels.
    Plaintext ptxt = cryptoContext->MakeCKKSPackedPlaintext(x, 1, 3, nullptr, numSlots);
    // ptxt->SetLength(numSlots>10?10:numSlots);
    auto rtn_vec = ptxt->GetRealPackedValue();
    cout << "( "; 
    for (size_t i = 0; i < 10; i++) cout << rtn_vec[i] << ", ";
    cout << "... ";

    cout << ") Estimated precision: " << ptxt->GetLogPrecision() << " bits" << endl;
    // std::cout << "Input \n\t" << ptxt << std::endl;

    // Encrypt the encoded vectors
    Ciphertext<DCRTPoly> ciph = cryptoContext->Encrypt(keyPair.publicKey, ptxt);

    std::cout << "Initial number of levels remaining: " << /*depth - */ciph->GetLevel() << std::endl;

    // Step 5: Perform the bootstrapping operation. The goal is to increase the number of levels remaining
    // for HE computation.
    auto ciphertextAfter1 = cryptoContext->EvalBootstrap(ciph);

    std::cout << "Number of levels remaining after bootstrapping1: " << /*depth - */ciphertextAfter1->GetLevel() << std::endl << std::endl;

    // Step 7: Decryption and output
    Plaintext result;
    decrypt_and_print(ciphertextAfter1, cryptoContext, keyPair, 10);
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextAfter, &result);
    // result->SetLength(numSlots>10?10:numSlots);
    // std::cout << "Output after bootstrapping1 \n\t" << result << std::endl;




    // // Encrypt the encoded vectors
    // // ciph = cryptoContext->Encrypt(keyPair.publicKey, result);

    // std::cout << "Initial number of levels remaining: " << /*depth - */ciphertextAfter1->GetLevel() << std::endl;

    // // Step 5: Perform the bootstrapping operation. The goal is to increase the number of levels remaining
    // // for HE computation.
    // auto ciphertextAfter2 = cryptoContext->EvalBootstrap(ciphertextAfter1);

    // std::cout << "Number of levels remaining after bootstrapping2: " << /*depth - */ciphertextAfter2->GetLevel() << std::endl
    //           << std::endl;

    // // Step 7: Decryption and output
    // decrypt_and_print(ciphertextAfter2, cryptoContext, keyPair, 10);
    // // cryptoContext->Decrypt(keyPair.secretKey, ciphertextAfter, &result);
    // // result->SetLength(numSlots>10?10:numSlots);
    // // std::cout << "Output after bootstrapping2 \n\t" << result << std::endl;




    // // Encrypt the encoded vectors
    // // ciph = cryptoContext->Encrypt(keyPair.publicKey, result);

    // std::cout << "Initial number of levels remaining: " << /*depth - */ciphertextAfter2->GetLevel() << std::endl;

    // // Step 5: Perform the bootstrapping operation. The goal is to increase the number of levels remaining
    // // for HE computation.
    // auto ciphertextAfter3 = cryptoContext->EvalBootstrap(ciphertextAfter2);

    // std::cout << "Number of levels remaining after bootstrapping3: " << /*depth - */ciphertextAfter3->GetLevel() << std::endl
    //           << std::endl;

    // // Step 7: Decryption and output
    // decrypt_and_print(ciphertextAfter3, cryptoContext, keyPair, 10);
    // // cryptoContext->Decrypt(keyPair.secretKey, ciphertextAfter, &result);
    // // result->SetLength(numSlots>10?10:numSlots);
    // // std::cout << "Output after bootstrapping3 \n\t" << result << std::endl;




    // // Encrypt the encoded vectors
    // // ciph = cryptoContext->Encrypt(keyPair.publicKey, result);

    // std::cout << "Initial number of levels remaining: " << /*depth - */ciphertextAfter3->GetLevel() << std::endl;

    // // Step 5: Perform the bootstrapping operation. The goal is to increase the number of levels remaining
    // // for HE computation.
    // auto ciphertextAfter4 = cryptoContext->EvalBootstrap(ciphertextAfter3);

    // std::cout << "Number of levels remaining after bootstrapping4: " << /*depth - */ciphertextAfter4->GetLevel() << std::endl
    //           << std::endl;

    // // Step 7: Decryption and output
    // decrypt_and_print(ciphertextAfter4, cryptoContext, keyPair, 10);
    // // cryptoContext->Decrypt(keyPair.secretKey, ciphertextAfter, &result);
    // // result->SetLength(numSlots>10?10:numSlots);
    // // std::cout << "Output after bootstrapping4 \n\t" << result << std::endl;
}
