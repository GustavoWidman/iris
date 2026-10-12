#pragma once
#include <crypto/CHIPCryptoPAL.h>
#include <platform/CommissionableDataProvider.h>
#include <lib/support/CodeUtils.h>
#include "private_config.h"
class PairingProvider : public chip::DeviceLayer::CommissionableDataProvider {
public:
 CHIP_ERROR GetSetupDiscriminator(uint16_t &v) override {v=BRIDGE_DISCRIMINATOR;return CHIP_NO_ERROR;}
 CHIP_ERROR SetSetupDiscriminator(uint16_t) override {return CHIP_ERROR_NOT_IMPLEMENTED;}
 CHIP_ERROR GetSpake2pIterationCount(uint32_t &v) override {v=10000;return CHIP_NO_ERROR;}
 CHIP_ERROR GetSpake2pSalt(chip::MutableByteSpan &s) override {const uint8_t salt[]=BRIDGE_PAIR_SALT;VerifyOrReturnError(s.size()>=sizeof(salt),CHIP_ERROR_BUFFER_TOO_SMALL);memcpy(s.data(),salt,sizeof(salt));s.reduce_size(sizeof(salt));return CHIP_NO_ERROR;}
 CHIP_ERROR GetSpake2pVerifier(chip::MutableByteSpan &v,size_t &n) override {uint8_t s[16]=BRIDGE_PAIR_SALT;chip::Crypto::Spake2pVerifier verifier;ReturnErrorOnFailure(verifier.Generate(10000,chip::ByteSpan(s),BRIDGE_SETUP_PIN));ReturnErrorOnFailure(verifier.Serialize(v));n=v.size();return CHIP_NO_ERROR;}
 CHIP_ERROR GetSetupPasscode(uint32_t &v) override {v=BRIDGE_SETUP_PIN;return CHIP_NO_ERROR;}
 CHIP_ERROR SetSetupPasscode(uint32_t) override {return CHIP_ERROR_NOT_IMPLEMENTED;}
};
