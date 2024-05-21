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

using namespace lbcrypto;
using namespace std;

usint Initialize(CryptoContext<DCRTPoly>& cryptoContext, KeyPair<lbcrypto::DCRTPoly>& keyPair, uint32_t numSlots);
void BootstrapExample(CryptoContext<DCRTPoly> cryptoContext, KeyPair<lbcrypto::DCRTPoly> keyPair, uint32_t numSlots, usint depth);

void init_stat();
void init_stat_no_workqueue();
void print_stat();
void print_memory_stat();
void set_num_prallel_jobs(uint32_t num_parallel);

int main(int argc, char* argv[]){
    init_stat();
    print_memory_stat();
    uint32_t numSlots = 1<<14; // Fully-Packed (numSlots = (logN)/2)

    // SEAL and bootstrapping setting (Resnet-20 Bootstrapping Setting)
	long boundary_K = 25;
	long boot_deg = 59;
    long scale_factor = 2;
    long inverse_deg = 1; 
	long logN = 16;
	long loge = 10; 
	long logn = 15;		// full slots
	long logn_1 = 14;	// sparse slots
	long logn_2 = 13;
	long logn_3 = 12;
	int logp = 59;//53;//46; 46 results in math error exception.
	int logq = 51;
	int log_special_prime = 51;
    int log_integer_part = logq - logp - loge + 5;
	int remaining_level = 16; // Calculation required

    std::vector<uint32_t> levelBudget = {4, 4};
	std::vector<uint32_t> bsgsDim = {0, 0};
	SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
	int boot_level = FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist) + 2;

    int total_level = remaining_level + boot_level;
	std::cout << "boot_level: " << boot_level << std::endl;
	std::cout << "remaining_level: " << remaining_level << std::endl;
	std::cout << "total_level: " << total_level << std::endl;

    CCParams<CryptoContextCKKSRNS> parameters;

	parameters.SetSecretKeyDist(secretKeyDist);
	parameters.SetRingDim(1<<logN);
	parameters.SetMultiplicativeDepth(total_level);
	parameters.SetScalingModSize(logp);
	parameters.SetSecurityLevel(HEStd_NotSet);

	uint32_t dnum = 1;
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
    cc->EvalMultKeyGen(keys.secretKey);

    //	additional rotation kinds for CNN
	std::vector<int> rotation_kinds = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33
		,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55
		,56
		,57,58,59,60,61
		,62,63,64,66,84,124,128,132,256,512,959,960,990,991,1008
		,1023,1024,1036,1064,1092,1952,1982,1983,2016,2044,2047,2048,2072,2078,2100,3007,3024,3040,3052,3070,3071,3072,3080,3108,4031
		,4032,4062,4063,4095,4096,5023,5024,5054,5055,5087,5118,5119,5120,6047,6078,6079,6111,6112,6142,6143,6144,7071,7102,7103,7135
		,7166,7167,7168,8095,8126,8127,8159,8190,8191,8192,9149,9183,9184,9213,9215,9216,10173,10207,10208,10237,10239,10240,11197,11231
		,11232,11261,11263,11264,12221,12255,12256,12285,12287,12288,13214,13216,13246,13278,13279,13280,13310,13311,13312,14238,14240
		,14270,14302,14303,14304,14334,14335,15262,15264,15294,15326,15327,15328,15358,15359,15360,16286,16288,16318,16350,16351,16352
		,16382,16383,16384,17311,17375,18335,18399,18432,19359,19423,20383,20447,20480,21405,21406,21437,21469,21470,21471,21501,21504
		,22429,22430,22461,22493,22494,22495,22525,22528,23453,23454,23485,23517,23518,23519,23549,24477,24478,24509,24541,24542,24543
		,24573,24576,25501,25565,25568,25600,26525,26589,26592,26624,27549,27613,27616,27648,28573,28637,28640,28672,29600,29632,29664
		,29696,30624,30656,30688,30720,31648,31680,31712,31743,31744,31774,32636,32640,32644,32672,32702,32704,32706,32735
		,32736,32737,32759,32760,32761,32762,32763,32764,32765,32766,32767		
	};

    cout << "Adding Bootstrapping Keys..." << endl;
	vector<int> gal_steps_vector;
	gal_steps_vector.push_back(0);
	for(int i=0; i<logN-1; i++) gal_steps_vector.push_back((1 << i));
	for(auto rot: rotation_kinds)
	{
		if(find(gal_steps_vector.begin(), gal_steps_vector.end(), rot) == gal_steps_vector.end()) gal_steps_vector.push_back(rot);
	} 

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
	// cc->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots>>1);
	// cc->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots>>2);
	cout << "EvalBootstrapSetup-" << endl;

	cout << "EvalBootstrapKeyGen+" << endl;
	cc->EvalBootstrapKeyGen(keys.secretKey, numSlots);
	// cc->EvalBootstrapKeyGen(keys.secretKey, numSlots>>1);
	// cc->EvalBootstrapKeyGen(keys.secretKey, numSlots>>2);
	cout << "EvalBootstrapKeyGen-" << endl;

    init_stat_no_workqueue();

    BootstrapExample(cc, keys, numSlots, total_level);
    // BootstrapExample(cc, keys, numSlots>>1, total_level);
    // BootstrapExample(cc, keys, numSlots>>2, total_level);
    
    // std::thread t1(BootstrapExample, keys, numSlots, total_level); 
    // std::thread t2(BootstrapExample, keys, numSlots, total_level); 
    // std::thread t3(BootstrapExample, keys, numSlots, total_level); 
    // std::thread t4(BootstrapExample, keys, numSlots, total_level); 


    // t1.join();
    // t2.join();
    // t3.join();
    // t4.join();

    print_stat();
    print_memory_stat();
}

// int main(int argc, char* argv[]) {
//     // We run the example with 8 slots and ring dimension 4096 to illustrate how to run bootstrapping with a sparse plaintext.
//     // Using a sparse plaintext and specifying the smaller number of slots gives a performance improvement (typically up to 3x).
    
//     uint32_t numSlots = 8; // Fully-Packed (numSlots = (logN)/2)
//     init_stat();
//     print_memory_stat();

//     CryptoContext<DCRTPoly> cryptoContext;
//     KeyPair<lbcrypto::DCRTPoly> keyPair;

//     usint depth = Initialize(cryptoContext,keyPair,numSlots);

//     set_num_prallel_jobs(1);

//     init_stat_no_workqueue();

//     BootstrapExample(cryptoContext, keyPair, numSlots, depth);
//     // BootstrapExample(cryptoContext, keyPair, numSlots, depth);
//     // BootstrapExample(cryptoContext, keyPair, numSlots, depth);
//     // BootstrapExample(cryptoContext, keyPair, numSlots, depth);
    
//     // std::thread t1(BootstrapExample, cryptoContext,keyPair,numSlots,depth); 
//     // std::thread t2(BootstrapExample, cryptoContext,keyPair,numSlots,depth); 
//     // std::thread t3(BootstrapExample, cryptoContext,keyPair,numSlots,depth); 
//     // std::thread t4(BootstrapExample, cryptoContext,keyPair,numSlots,depth); 


//     // t1.join();
//     // t2.join();
//     // t3.join();
//     // t4.join();

//     print_stat();
//     print_memory_stat();
// }

usint Initialize(CryptoContext<DCRTPoly>& cryptoContext, KeyPair<lbcrypto::DCRTPoly>& keyPair, uint32_t numSlots) {
    // Step 1: Set CryptoContext
    CCParams<CryptoContextCKKSRNS> parameters;

    // A. Specify main parameters
    /*  A1) Secret key distribution
    * The secret key distribution for CKKS should either be SPARSE_TERNARY or UNIFORM_TERNARY.
    * The SPARSE_TERNARY distribution was used in the original CKKS paper,
    * but in this example, we use UNIFORM_TERNARY because this is included in the homomorphic
    * encryption standard.
    */
    SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
    parameters.SetSecretKeyDist(secretKeyDist);

    /*  A2) Desired security level based on FHE standards.
    * In this example, we use the "NotSet" option, so the example can run more quickly with
    * a smaller ring dimension. Note that this should be used only in
    * non-production environments, or by experts who understand the security
    * implications of their choices. In production-like environments, we recommend using
    * HEStd_128_classic, HEStd_192_classic, or HEStd_256_classic for 128-bit, 192-bit,
    * or 256-bit security, respectively. If you choose one of these as your security level,
    * you do not need to set the ring dimension.
    */
    parameters.SetSecurityLevel(HEStd_NotSet);
    parameters.SetRingDim(1 << 16);

    /*  A3) Key switching parameters.
    * By default, we use HYBRID key switching with a digit size of 3.
    * Choosing a larger digit size can reduce complexity, but the size of keys will increase.
    * Note that you can leave these lines of code out completely, since these are the default values.
    */
    parameters.SetNumLargeDigits(3);
    parameters.SetKeySwitchTechnique(HYBRID);

    /*  A4) Scaling parameters.
    * By default, we set the modulus sizes and rescaling technique to the following values
    * to obtain a good precision and performance tradeoff. We recommend keeping the parameters
    * below unless you are an FHE expert.
    */
#if NATIVEINT == 128 && !defined(__EMSCRIPTEN__)
    // Currently, only FIXEDMANUAL and FIXEDAUTO modes are supported for 128-bit CKKS bootstrapping.
    ScalingTechnique rescaleTech = FIXEDAUTO;
    usint dcrtBits               = 78;
    usint firstMod               = 89;
#else
    // All modes are supported for 64-bit CKKS bootstrapping.
    ScalingTechnique rescaleTech = FLEXIBLEAUTO;
    usint dcrtBits               = 59;
    usint firstMod               = 60;
#endif

    parameters.SetScalingModSize(dcrtBits);
    parameters.SetScalingTechnique(rescaleTech);
    parameters.SetFirstModSize(firstMod);

    /*  A4) Bootstrapping parameters.
    * We set a budget for the number of levels we can consume in bootstrapping for encoding and decoding, respectively.
    * Using larger numbers of levels reduces the complexity and number of rotation keys,
    * but increases the depth required for bootstrapping.
	* We must choose values smaller than ceil(log2(slots)). A level budget of {4, 4} is good for higher ring
    * dimensions (65536 and higher).
    */
    std::vector<uint32_t> levelBudget = {3, 3};

    /* We give the user the option of configuring values for an optimization algorithm in bootstrapping.
    * Here, we specify the giant step for the baby-step-giant-step algorithm in linear transforms
    * for encoding and decoding, respectively. Either choose this to be a power of 2
    * or an exact divisor of the number of slots. Setting it to have the default value of {0, 0} allows OpenFHE to choose
    * the values automatically.
    */
    std::vector<uint32_t> bsgsDim = {0, 0};

    /*  A5) Multiplicative depth.
    * The goal of bootstrapping is to increase the number of available levels we have, or in other words,
    * to dynamically increase the multiplicative depth. However, the bootstrapping procedure itself
    * needs to consume a few levels to run. We compute the number of bootstrapping levels required
    * using GetBootstrapDepth, and add it to levelsAvailableAfterBootstrap to set our initial multiplicative
    * depth.
    */
    uint32_t levelsAvailableAfterBootstrap = 10;
    usint depth = levelsAvailableAfterBootstrap + FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
    parameters.SetMultiplicativeDepth(depth);

    // Generate crypto context.
    cryptoContext = GenCryptoContext(parameters);

    // Enable features that you wish to use. Note, we must enable FHE to use bootstrapping.
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);
    cryptoContext->Enable(ADVANCEDSHE);
    cryptoContext->Enable(FHE);

    usint ringDim = cryptoContext->GetRingDimension();
    std::cout << "CKKS scheme is using ring dimension " << ringDim << std::endl << std::endl;

    // Step 2: Precomputations for bootstrapping
    std::cout << "Precomputations for bootstrapping..." << std::endl;
    cryptoContext->EvalBootstrapSetup(levelBudget, bsgsDim, numSlots);

    // Step 3: Key Generation
    std::cout << "Key generation..." << std::endl;
    keyPair = cryptoContext->KeyGen();
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);
    // Generate bootstrapping keys.
    std::cout << "Generating bootstrapping keys..." << std::endl;
    cryptoContext->EvalBootstrapKeyGen(keyPair.secretKey, numSlots);
    
    return depth;
}

void BootstrapExample(CryptoContext<DCRTPoly> cryptoContext, KeyPair<lbcrypto::DCRTPoly> keyPair, uint32_t numSlots, usint depth) {
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
    Plaintext ptxt = cryptoContext->MakeCKKSPackedPlaintext(x, 1, depth - 1, nullptr, numSlots);
    ptxt->SetLength(numSlots);
    std::cout << "Input \n\t" << ptxt << std::endl;

    // Encrypt the encoded vectors
    Ciphertext<DCRTPoly> ciph = cryptoContext->Encrypt(keyPair.publicKey, ptxt);

    std::cout << "Initial number of levels remaining: " << depth - ciph->GetLevel() << std::endl;

    // Step 5: Perform the bootstrapping operation. The goal is to increase the number of levels remaining
    // for HE computation.
    auto ciphertextAfter = cryptoContext->EvalBootstrap(ciph);

    std::cout << "Number of levels remaining after bootstrapping: " << depth - ciphertextAfter->GetLevel() << std::endl
              << std::endl;

    // Step 7: Decryption and output
    Plaintext result;
    cryptoContext->Decrypt(keyPair.secretKey, ciphertextAfter, &result);
    result->SetLength(numSlots);
    std::cout << "Output after bootstrapping \n\t" << result << std::endl;
}
