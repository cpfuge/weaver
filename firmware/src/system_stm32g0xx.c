#include "stm32g0xx.h"

#if !defined (HSE_VALUE)
#define HSE_VALUE (8000000UL)     /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined (HSI_VALUE)
#define HSI_VALUE (16000000UL)    /*!< Value of the Internal oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined (LSI_VALUE)
#define LSI_VALUE  (32000UL)      /*!< Value of LSI in Hz */
#endif /* LSI_VALUE */

#if !defined (LSE_VALUE)
#define LSE_VALUE (32768UL)       /*!< Value of LSE in Hz */
#endif /* LSE_VALUE */

/* Uncomment the following line if you need to relocate your vector Table in Internal SRAM. */
/* #define VECT_TAB_SRAM */

/**
 * @brief Vector Table base offset field.
 *
 * This value must be a multiple of 0x100.
 */
#define VECT_TAB_OFFSET 0x0U 

/**
 * @brief System core clock
 *
 * The SystemCoreClock variable is updated in three ways:
 *     1) by calling CMSIS function SystemCoreClockUpdate()
 *     2) by calling HAL API function HAL_RCC_GetHCLKFreq()
 *     3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
 *
 * If you use this function to configure the system clock; then there
 * is no need to call the 2 first functions listed above, since SystemCoreClock
 * variable is updated automatically.
 */
uint32_t SystemCoreClock = 16000000UL;

const uint32_t AHBPrescTable[16UL] = { 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 1UL, 2UL, 3UL, 4UL, 6UL, 7UL, 8UL, 9UL };
const uint32_t APBPrescTable[8UL] =  { 0UL, 0UL, 0UL, 0UL, 1UL, 2UL, 3UL, 4UL };

/**
 * @brief Setup the microcontroller system.
 */
void SystemInit(void)
{
#ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET;  /* Vector table relocation in internal SRAM */
#else
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector table relocation in internal FLASH */
#endif
}

/**
 * @brief Update SystemCoreClock variable according to Clock Register Values.
 *        The SystemCoreClock variable contains the core clock (HCLK), it can
 *        be used by the user application to setup the SysTick timer or configure
 *        other parameters.
 *
 * Each time the core clock (HCLK) changes, this function must be called
 * to update SystemCoreClock variable value. Otherwise, any configuration
 * based on this variable will be incorrect.
 *
 * The system frequency computed by this function is not the real
 * frequency in the chip. It is calculated based on the predefined
 * constant and the selected clock source:
 *
 *     - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**) / HSI division factor
 *     - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
 *     - If SYSCLK source is LSI, SystemCoreClock will contain the LSI_VALUE
 *     - If SYSCLK source is LSE, SystemCoreClock will contain the LSE_VALUE
 *     - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
 *       or HSI_VALUE(*) multiplied/divided by the PLL factors.
 *
 *         (**) HSI_VALUE is a constant defined in stm32g0xx_hal_conf.h file (default value
 *              16 MHz) but the real value may vary depending on the variations
 *              in voltage and temperature.
 *         (***) HSE_VALUE is a constant defined in stm32g0xx_hal_conf.h file (default value
 *              8 MHz), user has to ensure that HSE_VALUE is same as the real
 *              frequency of the crystal used. Otherwise, this function may
 *              have wrong result.
 *
 *     - The result of this function could be not correct when using fractional
 *       value for HSE crystal.
 */
void SystemCoreClockUpdate(void)
{
    uint32_t tmp;
    uint32_t pllvco;
    uint32_t pllr;
    uint32_t pllsource;
    uint32_t pllm;
    uint32_t hsidiv;

    /* Get SYSCLK source */
    switch (RCC->CFGR & RCC_CFGR_SWS)
    {
    case RCC_CFGR_SWS_0:    /* HSE used as system clock */
        SystemCoreClock = HSE_VALUE;
        break;

    case (RCC_CFGR_SWS_1 | RCC_CFGR_SWS_0):    /* LSI used as system clock */
        SystemCoreClock = LSI_VALUE;
        break;

    case RCC_CFGR_SWS_2:    /* LSE used as system clock */
        SystemCoreClock = LSE_VALUE;
        break;

    case RCC_CFGR_SWS_1:    /* PLL used as system clock */
        /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
        SYSCLK = PLL_VCO / PLLR
        */
        pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
        pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1UL;

        if (pllsource == 0x03UL)    /* HSE used as PLL clock source */
        {
            pllvco = (HSE_VALUE / pllm);
        }
        else    /* HSI used as PLL clock source */
        {
            pllvco = (HSI_VALUE / pllm);
        }
        pllvco = pllvco * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
        pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1UL);

        SystemCoreClock = pllvco/pllr;
        break;
    
    case 0x00000000U:    /* HSI used as system clock */
    default:             /* HSI used as system clock */
        hsidiv = (1UL << ((READ_BIT(RCC->CR, RCC_CR_HSIDIV))>> RCC_CR_HSIDIV_Pos));
        SystemCoreClock = (HSI_VALUE / hsidiv);
        break;
    }

    /* Compute HCLK clock frequency */
    /* Get HCLK prescaler */
    tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos)];
    /* HCLK clock frequency */
    SystemCoreClock >>= tmp;
}
