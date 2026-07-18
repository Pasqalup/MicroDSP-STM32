/*
    * CS43131.c
    *
    *  Created on: Jul 7, 2026
    * CS43131 i2c driver
*/


#include "i2c.h"
#include <stdint.h>
#include "CS43131.h"

#define VOLSCALE_DOUBLE 0



void CS43131_Init(void)
{
    // 1. Power up ASP, XTAL, and HP
    uint8_t pdn_ctrl;
    CS43131_ReadRegister(PDN_CTRL, &pdn_ctrl);
    CS43131_WriteRegister(PDN_CTRL,  pdn_ctrl & ~(PDN_ASP | PDN_XTAL | PDN_HP));
    //  MCLK_INT=0 (24.576MHz), MCLK_SRC_SEL=00 (XTAL)
    CS43131_WriteRegister(SYSCLK_CTRL, 0x00);
    //// 4. Set ASP sample rate to 48kHz
    CS43131_WriteRegister(ASP_SPRATE, ASP_SPRATE_48KHZ);
    // 5. Set ASP sample size to 16-bit
    CS43131_WriteRegister(ASP_SPSIZE, ASP_SPSIZE_16BIT);
    // enable hpf, disable deemphasis
    CS43131_WriteRegister(PCM_FILT_OPTION, HP_FILT_ON);
    // configure pcm path - match  volume, mute both, 
    CS43131_WriteRegister(PCM_SIG_CTRL_1, PCM_VOL_BEQA | PCM_MUTE_AB | PCM_AMUTE | PCM_SZC_SOFT_RMP); 
}

void CS43131_setVolumePercent(uint32_t percent)
{
    if (percent >= 100U) {
        CS43131_WriteRegister(PCM_VOL_A, 0x00); // Max volume
        //VOL_BEQA automatically matches volume on both channels
    } else if (percent == 0U) {
        CS43131_WriteRegister(PCM_VOL_A, 0xFF); // Min volume
    } else {
        #if VOLSCALE_DOUBLE
        CS43131_WriteRegister(PCM_VOL_A, 200U - (percent * 2U)); // Scale volume
        #else
        CS43131_WriteRegister(PCM_VOL_A, 100U - percent); // Scale volume
        #endif
    }

}
void CS43131_Mute(uint8_t mute)
{
    uint8_t pcm_sig_ctrl_1;
    CS43131_ReadRegister(PCM_SIG_CTRL_1, &pcm_sig_ctrl_1);
    if (mute) {
        pcm_sig_ctrl_1 |= (PCM_MUTE_A | PCM_MUTE_B | PCM_AMUTE);
    } else {
        pcm_sig_ctrl_1 &= ~(PCM_MUTE_A | PCM_MUTE_B | PCM_AMUTE);
    }
    CS43131_WriteRegister(PCM_SIG_CTRL_1, pcm_sig_ctrl_1);
}
/*
typedef struct {
    int32_t b0, b1, b2;
    int32_t a1, a2;
    int32_t s1, s2;
} BiquadFilter;
*/

__attribute__((optimize("O3,unroll-loops")))
void CS43131_setFilterCoefficients(uint8_t select, const BiquadCoefficients *bq) {
    uint32_t base_addr;
    switch (select) {
        case 1: base_addr = SOS1_COEFF_B0_LSB; break;
        case 2: base_addr = SOS2_COEFF_B0_LSB; break;
        case 3: base_addr = SOS3_COEFF_B0_LSB; break;
        default: return;
    }
    
    uint8_t coeff_bytes[15]; // 5 coefficients × 3 bytes each
    int32_t coeffs[5] = {bq->b0, bq->b1, bq->b2, bq->a1, bq->a2};
    
    for (int i = 0; i < 5; i++) {
        // Convert Q2.30 → Q1.16: shift right by 14
        int32_t coeff_q16 = coeffs[i] >> 14;
        
        // Clamp to -1.0 to +1.0 (Q1.16 range)
        if (coeff_q16 > 0xFFFF) coeff_q16 = 0xFFFF;    // +1.0
        if (coeff_q16 < -0x10000) coeff_q16 = -0x10000; // -1.0
        
        // Mask to 17 bits (Q1.16)
        uint32_t coeff_17 = coeff_q16 & 0x1FFFF; // 17 bits
        
        coeff_bytes[i * 3]     = coeff_17 & 0xFF;          // LSB (bits 7:0)
        coeff_bytes[i * 3 + 1] = (coeff_17 >> 8) & 0xFF;   // MSB (bits 15:8)
        coeff_bytes[i * 3 + 2] = (coeff_q16 < 0) ? 0x01 : 0x00; // Sign (bit 16)
    }
    
    CS43131_WriteMultiple(base_addr, coeff_bytes, 15);
}


void CS43131_WriteRegister(uint32_t regaddr, uint8_t value) {
    uint8_t data[5];
    data[0] = (regaddr >> 16) & 0xFF; // MSB of register address
    data[1] = (regaddr >> 8) & 0xFF;
    data[2] = regaddr & 0xFF; // LSB of register address
    data[3] = 0x00; // INCR = 0; WIDTH=8bit
    data[4] = value;
    HAL_I2C_Master_Transmit(&hi2c1, CS43131_I2C_ADDRESS << 1, data, 5, HAL_MAX_DELAY);
}
void CS43131_WriteMultiple(uint32_t regaddr, uint8_t* values, uint8_t length) {
    uint8_t data[4 + length];
    data[0] = (regaddr >> 16) & 0xFF; // MSB of register address
    data[1] = (regaddr >> 8) & 0xFF;
    data[2] = regaddr & 0xFF; // LSB of register address
    data[3] = 0x01; // INCR = 1; WIDTH=8bit
    for (uint8_t i = 0; i < length; i++) {
        data[4 + i] = values[i];
    }
    HAL_I2C_Master_Transmit(&hi2c1, CS43131_I2C_ADDRESS << 1, data, 4 + length, HAL_MAX_DELAY);
}
void CS43131_ReadRegister(uint32_t regaddr, uint8_t* value) {
    uint8_t addr[4];
    addr[0] = (regaddr >> 16) & 0xFF; // MSB of register address
    addr[1] = (regaddr >> 8) & 0xFF;
    addr[2] = regaddr & 0xFF; // LSB of register address
    addr[3] = 0x00; // INCR = 0; WIDTH=8bit
    HAL_I2C_Master_Transmit(&hi2c1, CS43131_I2C_ADDRESS << 1, addr, 4, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, CS43131_I2C_ADDRESS << 1, value, 1, HAL_MAX_DELAY);
}