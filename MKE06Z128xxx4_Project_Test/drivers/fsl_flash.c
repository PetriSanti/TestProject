/*
 * The Clear BSD License
 * Copyright 2017 NXP
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_flash.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flash"
#endif


/*!
 * @name Misc utility defines
 * @{
 */
/*! @brief Alignment utility. */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x) & (uint32_t)(-((int32_t)(a))))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(x, a) (-((int32_t)((uint32_t)(-((int32_t)(x))) & (uint32_t)(-((int32_t)(a))))))
#endif

/*! @brief Join bytes to word utility. */
#define B1P2(b) (((uint16_t)(b)&0xFFU) << 8)
#define B1P1(b) ((uint16_t)(b)&0xFFU)
#define B2P1(b) ((uint16_t)(b)&0xFFFFU)
#define BYTES_JOIN_TO_WORD_1_1(x, y) (B1P1(x) | B1P2(y))
#define BYTES_JOIN_TO_WORD_1_2(x, y) (B1P2(x) | B2P1(y))
/*@}*/

/*!
 * @name Secondary flash configuration
 * @{
 */
/*! @brief Indicates whether the secondary flash has its own protection register in flash module. */
#define FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER (0)
/*@}*/

/*!
 * @name Flash cache ands speculation control defines
 * @{
 */
#if defined(MCM_PLACR_CFCC_MASK) || defined(MCM_CPCR2_CCBC_MASK)
#define FLASH_CACHE_IS_CONTROLLED_BY_MCM (1)
#else
#define FLASH_CACHE_IS_CONTROLLED_BY_MCM (0)
#endif
#if defined(FMC_PFB0CR_CINV_WAY_MASK) || defined(FMC_PFB01CR_CINV_WAY_MASK)
#define FLASH_CACHE_IS_CONTROLLED_BY_FMC (1)
#else
#define FLASH_CACHE_IS_CONTROLLED_BY_FMC (0)
#endif
#if defined(MCM_PLACR_DFCS_MASK)
#define FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_MCM (1)
#else
#define FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_MCM (0)
#endif
#if defined(FMC_PFB0CR_S_INV_MASK) || defined(FMC_PFB0CR_S_B_INV_MASK) || defined(FMC_PFB01CR_S_INV_MASK) || \
    defined(FMC_PFB01CR_S_B_INV_MASK)
#define FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC (1)
#else
#define FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC (0)
#endif
/*@}*/

/*!
 * @name Reserved EEPROM size (For a variety of purposes) defines
 * @{
 */
#define NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED 0x100U
/*@}*/

/*!
 * @name Flash Program Once Field defines
 * @{
 */
#if defined(FSL_FEATURE_FLASH_IS_FTMRE) || defined(FSL_FEATURE_FLASH_IS_FTMRH)
/* FTMRE and FTMRH only support 8-bytes unit size */
#define FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT 0
#define FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT 1
#endif
/*@}*/

/*!
 * @name Flash security status defines
 * @{
 */
#define FLASH_SECURITY_STATE_KEYEN 0x80U
#define FLASH_SECURITY_STATE_UNSECURED 0x02U
#define FLASH_NOT_SECURE 0x01U
#define FLASH_SECURE_BACKDOOR_ENABLED 0x02U
#define FLASH_SECURE_BACKDOOR_DISABLED 0x04U
/*@}*/

/*!
 * @name Flash controller command numbers
 * @{
 */
#define FTMRx_ERASE_VERIFY_ALL_BLOCK 0x01U          /*!< ERSVERALL*/
#define FTMRx_ERASE_VERIFY_BLOCK 0x02U              /*!< ERSVERBLK*/
#define FTMRx_ERASE_VERIFY_SECTION 0x03U            /*!< ERSVERSECT*/
#define FTMRx_READ_ONCE 0x04U                       /*!< RDONCE or RDINDEX*/
#define FTMRx_PROGRAM 0x06U                         /*!< PGM*/
#define FTMRx_PROGRAM_ONCE 0x07U                    /*!< PGMONCE or PGMINDEX*/
#define FTMRx_ERASE_ALL_BLOCK 0x08U                 /*!< ERSALL*/
#define FTMRx_ERASE_BLOCK 0x09U                     /*!< ERSBLK*/
#define FTMRx_ERASE_SECTOR 0x0AU                    /*!< ERSSCR*/
#define FTMRx_ERASE_ALL_BLOCK_UNSECURE 0x0BU        /*!< ERSALLU*/
#define FTMRx_SECURITY_BY_PASS 0x0CU                /*!< VFYKEY*/
#define FTMRx_SET_USER_MARGIN_LEVEL 0x0DU           /*!< SETUSERLVL*/
#define FTMRx_SET_FACTORY_MARGIN_LEVEL 0x0EU        /*!< SETFTYLVL*/
#define FTMRx_CONFIGURE_NVM 0x0FU                   /*!< CONNVM*/
#define FTMRx_ERASE_VERIFY_EEPROM_SECTION 0x10U     /*!< ERSVES*/
#define FTMRx_PROGRAM_EEPROM 0x11U                  /*!< PGME*/
#define FTMRx_ERASE_EEPROM_SECTOR 0x12U             /*!< ERSESCR*/
/*@}*/

/*!
 * @name Common flash register info defines
 * @{
 */
#if defined(FTMRE)
#define FTMRx FTMRE
#define FTMRx_BASE FTMRE_BASE
#define FTMRx_FCCOBIX_CCOBIX(x) FTMRE_FCCOBIX_CCOBIX(x)
#define FTMRx_FCCOBLO_CCOB(x) FTMRE_FCCOBLO_CCOB(x)
#define FTMRx_FCCOBHI_CCOB(x) FTMRE_FCCOBHI_CCOB(x)
#define FTMRx_FCLKDIV_FDIVLCK_MASK FTMRE_FCLKDIV_FDIVLCK_MASK
#define FTMRx_FCLKDIV_FDIVLD_MASK FTMRE_FCLKDIV_FDIVLD_MASK
#define FTMRx_FCLKDIV_FDIV_MASK FTMRE_FCLKDIV_FDIV_MASK
#define FTMRx_FSTAT_CCIF_MASK FTMRE_FSTAT_CCIF_MASK
#define FTMRx_FCLKDIV_FDIV(x) FTMRE_FCLKDIV_FDIV(x)
#define FTMRx_FSTAT_ACCERR_MASK FTMRE_FSTAT_ACCERR_MASK
#define FTMRx_FSTAT_FPVIOL_MASK FTMRE_FSTAT_FPVIOL_MASK
#define FTMRx_FSTAT_MGBUSY_MASK FTMRE_FSTAT_MGBUSY_MASK
#define FTMRx_FSTAT_MGSTAT_MASK FTMRE_FSTAT_MGSTAT_MASK
#define FTMRx_FSTAT_MGSTAT0_MASK FTMRE_FSTAT_MGSTAT(1)
#define FTMRx_FSTAT_MGSTAT1_MASK FTMRE_FSTAT_MGSTAT(2)
#define FTMRx_FSEC_SEC_MASK FTMRE_FSEC_SEC_MASK
#define FTMRx_FSEC_KEYEN_MASK FTMRE_FSEC_KEYEN_MASK
#if defined(FTMRE_FPROT_FPHDIS_MASK)
#define FTMRx_FPROT_FPHDIS_MASK FTMRE_FPROT_FPHDIS_MASK
#endif
#if defined(FTMRE_FERSTAT_SFDIF_MASK)
#define FTMRx_FERSTAT_SFDIF_MASK FTMRE_FERSTAT_SFDIF_MASK
#define FTMRx_FERSTAT_DFDIF_MASK FTMRE_FERSTAT_DFDIF_MASK
#endif
#elif defined(FTMRH)
#define FTMRx FTMRH
#define FTMRx_BASE FTMRH_BASE
#define FTMRx_FCCOBIX_CCOBIX(x) FTMRH_FCCOBIX_CCOBIX(x)
#define FTMRx_FCCOBLO_CCOB(x) FTMRH_FCCOBLO_CCOB(x)
#define FTMRx_FCCOBHI_CCOB(x) FTMRH_FCCOBHI_CCOB(x)
#define FTMRx_FCLKDIV_FDIVLCK_MASK FTMRH_FCLKDIV_FDIVLCK_MASK
#define FTMRx_FCLKDIV_FDIVLD_MASK FTMRH_FCLKDIV_FDIVLD_MASK
#define FTMRx_FCLKDIV_FDIV_MASK FTMRH_FCLKDIV_FDIV_MASK
#define FTMRx_FCLKDIV_FDIV(x) FTMRH_FCLKDIV_FDIV(x)
#define FTMRx_FSTAT_CCIF_MASK FTMRH_FSTAT_CCIF_MASK
#define FTMRx_FSTAT_ACCERR_MASK FTMRH_FSTAT_ACCERR_MASK
#define FTMRx_FSTAT_FPVIOL_MASK FTMRH_FSTAT_FPVIOL_MASK
#define FTMRx_FSTAT_MGBUSY_MASK FTMRH_FSTAT_MGBUSY_MASK
#define FTMRx_FSTAT_MGSTAT_MASK FTMRH_FSTAT_MGSTAT_MASK
#define FTMRx_FSTAT_MGSTAT0_MASK FTMRH_FSTAT_MGSTAT(1U)
#define FTMRx_FSTAT_MGSTAT1_MASK FTMRH_FSTAT_MGSTAT(2U)
#define FTMRx_FSEC_SEC_MASK FTMRH_FSEC_SEC_MASK
#define FTMRx_FSEC_KEYEN_MASK FTMRH_FSEC_KEYEN_MASK
#if defined(FTMRH_FPROT_FPHDIS_MASK)
#define FTMRx_FPROT_FPHDIS_MASK FTMRH_FPROT_FPHDIS_MASK
#endif
#if defined(FTMRH_FERSTAT_SFDIF_MASK)
#define FTMRx_FERSTAT_SFDIF_MASK FTMRH_FERSTAT_SFDIF_MASK
#define FTMRx_FERSTAT_DFDIF_MASK FTMRH_FERSTAT_DFDIF_MASK
#endif
#else
#error "Unknown flash controller"
#endif
/*@}*/

/*!
 * @brief Enumeration for flash config area.
 */
enum _flash_config_area_range
{
    kFLASH_ConfigAreaStart = 0x400U,
    kFLASH_ConfigAreaEnd = 0x40FU
};

/*!
 * @name Flash register access type defines
 * @{
 */
#define FTMRx_REG8_ACCESS_TYPE volatile uint8_t *
#define FTMRx_REG32_ACCESS_TYPE volatile uint32_t *
/*@}*/

/*!
 * @brief MCM cache register access info defines.
 */
#if defined(MCM_PLACR_CFCC_MASK)
#define MCM_CACHE_CLEAR_MASK MCM_PLACR_CFCC_MASK
#define MCM_CACHE_CLEAR_SHIFT MCM_PLACR_CFCC_SHIFT
#if defined(MCM)
#define MCM0_CACHE_REG MCM->PLACR
#elif defined(MCM0)
#define MCM0_CACHE_REG MCM0->PLACR
#endif
#if defined(MCM1)
#define MCM1_CACHE_REG MCM1->PLACR
#endif
#elif defined(MCM_CPCR2_CCBC_MASK)
#define MCM_CACHE_CLEAR_MASK MCM_CPCR2_CCBC_MASK
#define MCM_CACHE_CLEAR_SHIFT MCM_CPCR2_CCBC_SHIFT
#if defined(MCM)
#define MCM0_CACHE_REG MCM->CPCR2
#elif defined(MCM0)
#define MCM0_CACHE_REG MCM0->CPCR2
#endif
#if defined(MCM1)
#define MCM1_CACHE_REG MCM1->CPCR2
#endif
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief Copy flash_run_command() to RAM*/
static void copy_flash_run_command(uint32_t *flashRunCommand);
/*! @brief Copy flash_cache_clear_command() to RAM*/
static void copy_flash_common_bit_operation(uint32_t *flashCommonBitOperation);
/*! @brief Check whether flash execute-in-ram functions are ready*/
static status_t flash_check_execute_in_ram_function_info(flash_config_t *config);
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Internal function Flash command sequence. Called by driver APIs only*/
static status_t flash_command_sequence(flash_config_t *config);

/*! @brief Internal function Flash set command. Called by driver APIs only*/
void flash_set_command(uint8_t index, uint8_t fValue, uint8_t sValue);

/*! @brief Perform the cache clear to the flash*/
void flash_cache_clear(flash_config_t *config);

/*! @brief Process the cache to the flash*/
static void flash_cache_clear_process(flash_config_t *config, flash_cache_clear_process_t process);

/*! @brief Validates the range and alignment of the given address range.*/
static status_t flash_check_range(flash_config_t *config,
                                  uint32_t startAddress,
                                  uint32_t lengthInBytes,
                                  uint32_t alignmentBaseline);
/*! @brief Gets the right address, sector and block size of current flash type which is indicated by address.*/
static status_t flash_get_matched_operation_info(flash_config_t *config,
                                                 uint32_t address,
                                                 flash_operation_config_t *info);
/*! @brief Validates the given user key for flash erase APIs.*/
static status_t flash_check_user_key(uint32_t key);

/*! @brief Gets the flash protection information.*/
static status_t flash_get_protection_info(flash_config_t *config, flash_protection_config_t *info);

#if FLASH_CACHE_IS_CONTROLLED_BY_MCM
/*! @brief Performs the cache clear to the flash by MCM.*/
void mcm_flash_cache_clear(flash_config_t *config);
#endif /* FLASH_CACHE_IS_CONTROLLED_BY_MCM */

#if FLASH_CACHE_IS_CONTROLLED_BY_FMC
/*! @brief Performs the cache clear to the flash by FMC.*/
void fmc_flash_cache_clear(void);
#endif /* FLASH_CACHE_IS_CONTROLLED_BY_FMC */

#if FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC
/*! @brief Performs the prefetch speculation buffer clear to the flash by FMC.*/
void fmc_flash_prefetch_speculation_clear(void);
#endif /* FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC */

/*! @brief Set the flash and eeprom to the specified user margin level.*/
status_t flash_setusermarginlevel(flash_config_t *config, uint32_t start, uint8_t iseeprom, flash_user_margin_value_t margin);

/*! @brief Set the flash and eeprom to the specified factory margin level.*/
status_t flash_setfactorymarginlevel(flash_config_t *config, uint32_t start, uint8_t iseeprom, flash_factory_margin_value_t margin);

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief A function pointer used to point to relocated flash_run_command() */
static void (*callFlashRunCommand)(FTMRx_REG8_ACCESS_TYPE FTMRx_fstat);
/*! @brief A function pointer used to point to relocated flash_common_bit_operation() */
static void (*callFlashCommonBitOperation)(FTMRx_REG32_ACCESS_TYPE base,
                                           uint32_t bitMask,
                                           uint32_t bitShift,
                                           uint32_t bitValue);

/*!
 * @brief Position independent code of flash_run_command()
 *
 * Note1: The prototype of C function is shown as below:
 * @code
 *   void flash_run_command(FTMRx_REG8_ACCESS_TYPE FTMRx_fstat)
 *   {
 *       // clear CCIF bit
 *       *FTMRx_fstat = FTMRx_FSTAT_CCIF_MASK;
 *
 *       // Check CCIF bit of the flash status register, wait till it is set.
 *       // IP team indicates that this loop will always complete.
 *       while (!((*FTMRx_fstat) & FTMRx_FSTAT_CCIF_MASK))
 *       {
 *       }
 *   }
 * @endcode
 * Note2: The binary code is generated by IAR 7.70.1
 */
const static uint16_t s_flashRunCommandFunctionCode[] = {
    0x2180, /* MOVS  R1, #128 ; 0x80 */
    0x7001, /* STRB  R1, [R0] */
    /* @4: */
    0x7802, /* LDRB  R2, [R0] */
    0x420a, /* TST   R2, R1 */
    0xd0fc, /* BEQ.N @4 */
    0x4770  /* BX    LR */
};

/*!
 * @brief Position independent code of flash_common_bit_operation()
 *
 * Note1: The prototype of C function is shown as below:
 * @code
 *   void flash_common_bit_operation(FTMRx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift, uint32_t
 * bitValue)
 *   {
 *       if (bitMask)
 *       {
 *           uint32_t value = (((uint32_t)(((uint32_t)(bitValue)) << bitShift)) & bitMask);
 *           *base = (*base & (~bitMask)) | value;
 *       }
 *
 *       __ISB();
 *       __DSB();
 *   }
 * @endcode
 * Note2: The binary code is generated by IAR 7.70.1
 */
const static uint16_t s_flashCommonBitOperationFunctionCode[] = {
    0xb510, /* PUSH  {R4, LR} */
    0x2900, /* CMP   R1, #0 */
    0xd005, /* BEQ.N @12 */
    0x6804, /* LDR   R4, [R0] */
    0x438c, /* BICS  R4, R4, R1 */
    0x4093, /* LSLS  R3, R3, R2 */
    0x4019, /* ANDS  R1, R1, R3 */
    0x4321, /* ORRS  R1, R1, R4 */
    0x6001, /* STR   R1, [R0] */
    /*  @12: */
    0xf3bf, 0x8f6f, /* ISB */
    0xf3bf, 0x8f4f, /* DSB */
    0xbd10          /* POP   {R4, PC} */
};
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

#if (FLASH_DRIVER_IS_FLASH_RESIDENT && !FLASH_DRIVER_IS_EXPORTED)
/*! @brief A static buffer used to hold flash_run_command() */
static uint32_t s_flashRunCommand[kFLASH_ExecuteInRamFunctionMaxSizeInWords];
/*! @brief A static buffer used to hold flash_common_bit_operation() */
static uint32_t s_flashCommonBitOperation[kFLASH_ExecuteInRamFunctionMaxSizeInWords];
/*! @brief Flash execute-in-ram function information */
static flash_execute_in_ram_function_config_t s_flashExecuteInRamFunctionInfo;
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t FLASH_Init(flash_config_t *config)
{
    uint8_t clkDiver; 

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }
    clkDiver = config->PFlashClockFreq / 1000000U - 1;

    /* Initialize the flash clock to be within spec 1MHz. */
    if ((!(FTMRx->FCLKDIV & FTMRx_FCLKDIV_FDIVLCK_MASK)) && (FTMRx->FSTAT & FTMRx_FSTAT_CCIF_MASK))
    {
        /* FCLKDIV register is not locked.*/
        FTMRx->FCLKDIV = (FTMRx->FCLKDIV & (~FTMRx_FCLKDIV_FDIV_MASK)) | FTMRx_FCLKDIV_FDIV(clkDiver);
        if ((FTMRx->FCLKDIV & FTMRx_FCLKDIV_FDIV_MASK) != FTMRx_FCLKDIV_FDIV(clkDiver))
        {
            return kStatus_FLASH_ClockDivider;
        }
    }
    else
    {
        /* FCLKDIV register is locked. */
        if((FTMRx->FCLKDIV & FTMRx_FCLKDIV_FDIV_MASK) != FTMRx_FCLKDIV_FDIV(clkDiver))
        {
            return kStatus_FLASH_ClockDivider;
        }
    }
#if FLASH_SSD_IS_SECONDARY_FLASH_ENABLED
    if (config->FlashMemoryIndex == (uint8_t)kFLASH_MemoryIndexSecondaryFlash)
    {
        config->PFlashBlockBase = SECONDARY_FLASH_FEATURE_PFLASH_START_ADDRESS;
        config->PFlashTotalSize = SECONDARY_FLASH_FEATURE_PFLASH_BLOCK_COUNT * SECONDARY_FLASH_FEATURE_PFLASH_BLOCK_SIZE;
        config->PFlashBlockCount = SECONDARY_FLASH_FEATURE_PFLASH_BLOCK_COUNT;
        config->PFlashSectorSize = SECONDARY_FLASH_FEATURE_PFLASH_BLOCK_SECTOR_SIZE;
    }
    else
#endif
    {
        /* fill out a few of the structure members */
        config->PFlashBlockBase = FSL_FEATURE_FLASH_PFLASH_START_ADDRESS;
        config->PFlashTotalSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE;
        config->PFlashBlockCount = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT;
        config->PFlashSectorSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE;
    }
    config->PFlashCallback = NULL;

/* copy required flash commands to RAM */
#if (FLASH_DRIVER_IS_FLASH_RESIDENT && !FLASH_DRIVER_IS_EXPORTED)
    if (kStatus_FLASH_Success != flash_check_execute_in_ram_function_info(config))
    {
        s_flashExecuteInRamFunctionInfo.activeFunctionCount = 0;
        s_flashExecuteInRamFunctionInfo.flashRunCommand = s_flashRunCommand;
        s_flashExecuteInRamFunctionInfo.flashCommonBitOperation = s_flashCommonBitOperation;
        config->flashExecuteInRamFunctionInfo = &s_flashExecuteInRamFunctionInfo.activeFunctionCount;
        FLASH_PrepareExecuteInRamFunctions(config);
    }
#endif
#if FLASH_SSD_IS_EEPROM_ENABLED
    config->EEpromBlockBase = FSL_FEATURE_FLASH_EEPROM_START_ADDRESS;
    config->EEpromBlockCount = FSL_FEATURE_FLASH_EEPROM_BLOCK_COUNT;
    config->EEpromTotalSize = FSL_FEATURE_FLASH_EEPROM_BLOCK_SIZE * FSL_FEATURE_FLASH_EEPROM_BLOCK_COUNT;
    config->EEpromSectorSize = FSL_FEATURE_FLASH_EEPROM_BLOCK_SECTOR_SIZE;
#endif
    config->PFlashMarginLevel = kFLASH_MarginValueNormal;

    return kStatus_FLASH_Success;
}

status_t FLASH_SetCallback(flash_config_t *config, flash_callback_t callback)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    config->PFlashCallback = callback;

    return kStatus_FLASH_Success;
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
status_t FLASH_PrepareExecuteInRamFunctions(flash_config_t *config)
{
    flash_execute_in_ram_function_config_t *flashExecuteInRamFunctionInfo;

    if ((config == NULL) || (config->flashExecuteInRamFunctionInfo == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flashExecuteInRamFunctionInfo = (flash_execute_in_ram_function_config_t *)config->flashExecuteInRamFunctionInfo;

    copy_flash_run_command(flashExecuteInRamFunctionInfo->flashRunCommand);
    copy_flash_common_bit_operation(flashExecuteInRamFunctionInfo->flashCommonBitOperation);
    flashExecuteInRamFunctionInfo->activeFunctionCount = kFLASH_ExecuteInRamFunctionTotalNum;

    return kStatus_FLASH_Success;
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

status_t FLASH_EraseAll(flash_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* preparing passing parameter to erase all flash & EEPROM blocks */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    /* Write index to specify the command code to be loaded */
    flash_set_command(0U, 0U, FTMRx_ERASE_ALL_BLOCK);

    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

    return returnCode;
}

status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key)
{
    uint32_t sectorSize;
    flash_operation_config_t flashOperationInfo;
    uint32_t endAddress;      /* storing end address */
    uint32_t numberOfSectors; /* number of sectors calculated by endAddress */
    status_t returnCode;

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.sectorCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }
    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    start = flashOperationInfo.convertedAddress;
    sectorSize = flashOperationInfo.activeSectorSize;

    /* calculating Flash end address */
    endAddress = start + lengthInBytes - 1;

    /* re-calculate the endAddress and align it to the start of the next sector
     * which will be used in the comparison below */
    if (endAddress % sectorSize)
    {
        numberOfSectors = (endAddress / sectorSize) + 1;
        endAddress = (numberOfSectors * sectorSize) - 1;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    /* the start address will increment to the next sector address
     * until it reaches the endAdddress */
    while (start <= endAddress)
    {
        /* clear error flags */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;
        /* preparing passing parameter to erase a flash block */
        /* Write index to specify the command code to be loaded */
        flash_set_command(0U, start >> 16, FTMRx_ERASE_SECTOR);

        /* Write index to specify the lower byte memory address bits[15:0] to be loaded */
        flash_set_command(1U, start, start >> 8);

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        /* calling flash callback function if it is available */
        if (config->PFlashCallback)
        {
            config->PFlashCallback();
        }

        /* checking the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }
        /* Increment to the next sector */
        start += sectorSize;
    }

    flash_cache_clear(config);

    return (returnCode);
}

#if defined(FSL_FEATURE_FLASH_HAS_UNSECURE_FLASH_CMD) && FSL_FEATURE_FLASH_HAS_UNSECURE_FLASH_CMD
status_t FLASH_EraseAllUnsecure(flash_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Prepare passing parameter to erase all flash blocks (unsecure). */
    /* clear error flags */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    /* Write index to specify the command code to be loaded */
    flash_set_command(0U, 0U, FTMRx_ERASE_ALL_BLOCK_UNSECURE);

    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_UNSECURE_FLASH_CMD */

status_t FLASH_Program(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    flash_operation_config_t flashOperationInfo;

    if (src == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.blockWriteUnitSize);
    if (returnCode)
    {
        return returnCode;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    while (lengthInBytes > 0)
    {
        /* pass paramters to FTMRx */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

        /* Write index to specify the command code to be loaded */
        flash_set_command(0U, start >> 16, FTMRx_PROGRAM);
        flash_set_command(1U, start, start >> 8);
        /*Small endian by default.*/
        /* Write index to specify the word0 (MSB word) to be programmed */
        flash_set_command(2U, *src, *src >> 8);

        /* Write index to specify the word1 (LSB word) to be programmed */
        flash_set_command(3U, *src >> 16, *src >> 24); 
        /* preparing passing parameter to program the flash block */
        
        if (4 == flashOperationInfo.blockWriteUnitSize)
        {
            /* calling flash command sequence function to execute the command */
            returnCode = flash_command_sequence(config);
        }
        else if (8 == flashOperationInfo.blockWriteUnitSize)
        {
            src++;
            /* Write index to specify the word (MSB word) to be programmed */
            flash_set_command(4U, *src, *src >> 8);
            /* Write index to specify the word (LSB word) to be programmed */
            flash_set_command(5U, *src >> 16, *src >> 24);

            /* calling flash command sequence function to execute the command */
            returnCode = flash_command_sequence(config);
            
        }
        else
        {
        }
        src++;
        /* calling flash callback function if it is available */
        if (config->PFlashCallback)
        {
            config->PFlashCallback();
        }

        /* checking for the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }
        else
        {
            /* update start address for next iteration */
            start += flashOperationInfo.blockWriteUnitSize;

            /* update lengthInBytes for next iteration */
            lengthInBytes -= flashOperationInfo.blockWriteUnitSize;
        }
    }

    flash_cache_clear(config);

    return (returnCode);
}

status_t FLASH_ProgramOnce(flash_config_t *config, uint32_t index, uint32_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (src == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* pass paramters to FTMRx */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;
    
    /* Write index to specify the command code to be loaded. */
    flash_set_command(0U, 0U, FTMRx_PROGRAM_ONCE);
    flash_set_command(1U, index, index >> 8);

    /* Write index to specify the word0 (MSB word) to be programmed. */
    flash_set_command(2U, *src, *src >> 8);

    /* Write index to specify the word1 (LSB word) to be programmed. */
    flash_set_command(3U, *src >> 16, *src >> 24);

#if FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT

    /* Write index to specify the word2 (MSB word) to be programmed. */
    flash_set_command(4U, *(src + 1), *(src + 1) >> 8);

    /* Write index to specify the word3 (LSB word) to be programmed. */
    flash_set_command(5U, *(src + 1) >> 16, *(src + 1) >> 24);
#endif
    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

    return returnCode;
}

#if FLASH_SSD_IS_EEPROM_ENABLED
status_t FLASH_EepromWrite(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    uint32_t i;
    if ((config == NULL)|| (src == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }
    if ((lengthInBytes > 4) || (0 == lengthInBytes))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Check the supplied address range. */
    /* Validates the range of the given address */
    if ((start < config->EEpromBlockBase) ||
        ((start + lengthInBytes) > (config->EEpromBlockBase + config->EEpromTotalSize)))
    {
        return kStatus_FLASH_AddressError;
    }

    returnCode = kStatus_FLASH_Success;

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);

    while (lengthInBytes > 3)
    {
        /* pass paramters to FTMRx */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

        /* Write index to specify the command code to be loaded */
        flash_set_command(0U, start >> 16, FTMRx_PROGRAM_EEPROM);
        flash_set_command(1U, start, start >> 8);

        /* Write index to specify the byte (MSB word) to be programmed */
        for (i = 0; i < lengthInBytes; i++)
        {
            flash_set_command(0x02U + i, *src++, 0U);
        }

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        /* calling flash callback function if it is available */
        if (config->PFlashCallback)
        {
            config->PFlashCallback();
        }

        /* checking for the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }
        else
        {
            /* update start address for next iteration */
            start += 4U;
            src += 4U;
            /* update lengthInBytes for next iteration */
            lengthInBytes -= 4U;
        }
    }
    if ((lengthInBytes > 0) && (kStatus_FLASH_Success == returnCode))
    {
        /* pass paramters to FTMRx */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

        /* Write index to specify the command code to be loaded */
        flash_set_command(0U, start >> 16, FTMRx_PROGRAM_EEPROM);
        flash_set_command(1U, start, start >> 8);

        /* Write index to specify the byte (MSB word) to be programmed */
        for (i = 0; i < lengthInBytes; i++)
        {
            flash_set_command(0x02U + i, *src++, 0U);
        }
        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);
    }
    flash_cache_clear(config);

    return (returnCode);
}
#endif /* FLASH_SSD_IS_EEPROM_ENABLED */

status_t FLASH_ReadOnce(flash_config_t *config, uint32_t index, uint32_t *dst, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (dst == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }
    
    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);
    /* clear error flags */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    /* Write index to specify the command code to be loaded */
    flash_set_command(0U, 0U, FTMRx_READ_ONCE);
    flash_set_command(1U, index, index >> 8);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    if (kStatus_FLASH_Success == returnCode)
    {
        /* Read lengthInBytes words. */
        FTMRx->FCCOBIX = FTMRx_FCCOBIX_CCOBIX(2U);
        *dst = (uint32_t)BYTES_JOIN_TO_WORD_1_1(FTMRx->FCCOBLO, FTMRx->FCCOBHI);
        FTMRx->FCCOBIX = FTMRx_FCCOBIX_CCOBIX(3U);
        *dst |= ((uint32_t)BYTES_JOIN_TO_WORD_1_1(FTMRx->FCCOBLO, FTMRx->FCCOBHI)) << 16;
#if FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT
        FTMRx->FCCOBIX = FTMRx_FCCOBIX_CCOBIX(4U);
        *(dst + 1) = (uint32_t)BYTES_JOIN_TO_WORD_1_1(FTMRx->FCCOBLO, FTMRx->FCCOBHI);
        FTMRx->FCCOBIX = FTMRx_FCCOBIX_CCOBIX(5U);
        *(dst + 1) |= ((uint32_t)BYTES_JOIN_TO_WORD_1_1(FTMRx->FCCOBLO, FTMRx->FCCOBHI)) << 16;
#endif
    }
    flash_cache_clear(config);

    return returnCode;
}

status_t FLASH_GetSecurityState(flash_config_t *config, flash_security_state_t *state)
{
    /* store data read from flash register */
    uint8_t registerValue;

    if ((config == NULL) || (state == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Get flash security register value */
    registerValue = FTMRx->FSEC;

    /* check the status of the flash security bits in the security register */
    if (FLASH_SECURITY_STATE_UNSECURED == (registerValue & FTMRx_FSEC_SEC_MASK))
    {
        /* Flash in unsecured state */
        *state = kFLASH_SecurityStateNotSecure;
    }
    else
    {
        /* Flash in secured state
         * check for backdoor key security enable bit */
        if (FLASH_SECURITY_STATE_KEYEN == (registerValue & FTMRx_FSEC_KEYEN_MASK))
        {
            /* Backdoor key security enabled */
            *state = kFLASH_SecurityStateBackdoorEnabled;
        }
        else
        {
            /* Backdoor key security disabled */
            *state = kFLASH_SecurityStateBackdoorDisabled;
        }
    }

    return (kStatus_FLASH_Success);
}

status_t FLASH_SecurityBypass(flash_config_t *config, const uint8_t *backdoorKey)
{
    uint8_t registerValue; /* registerValue */
    status_t returnCode;   /* return code variable */
    uint8_t tmpKey;
    uint8_t i;

    if ((config == NULL) || (backdoorKey == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* set the default return code as kStatus_Success */
    returnCode = kStatus_FLASH_Success;

    /* Get flash security register value */
    registerValue = FTMRx->FSEC;

    /* Check to see if flash is in secure state (any state other than 0x2)
     * If not, then skip this since flash is not secure */
    if (0x02 != (registerValue & 0x03))
    {
        
        /* clear error flags */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;
        /* preparing passing parameter to erase a flash block */
        /* Write index to specify the command code to be loaded */
        flash_set_command(0U, 0U, FTMRx_SECURITY_BY_PASS);
    
        for(i = 0; i < 4; i++)
        {
            tmpKey = *backdoorKey++;
            flash_set_command(i + 1U, tmpKey, *backdoorKey++);
        }

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);
    }

    return (returnCode);
}

status_t FLASH_VerifyEraseAll(flash_config_t *config, flash_margin_value_t margin)
{
    status_t returnCode;
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }
    /* set the default return code as kStatus_Success */
    returnCode = kStatus_FLASH_Success;

    if (margin != config->PFlashMarginLevel)
    {
        switch(margin)
        {
            case kFLASH_MarginValueUser:
                returnCode = flash_setusermarginlevel(config, config->PFlashBlockBase, 0U, kFLASH_UserMarginValue1);
                break;
            case kFLASH_MarginValueFactory:
                returnCode = flash_setfactorymarginlevel(config, config->PFlashBlockBase, 0U, kFLASH_FactoryMarginValue1);
                break;
            case kFLASH_MarginValueNormal:
                returnCode = flash_setusermarginlevel(config, config->PFlashBlockBase, 0U, kFLASH_ReadMarginValueNormal);
                break;
            default:
                returnCode = kStatus_FLASH_InvalidArgument;
                break;
        }
        config->PFlashMarginLevel = margin;
    }
    if (returnCode)
    {
        return returnCode;
    }
#if FLASH_SSD_IS_EEPROM_ENABLED
    if (margin != config->PFlashMarginLevel)
    {
        switch(margin)
        {
            case kFLASH_MarginValueUser:
                returnCode = flash_setusermarginlevel(config, config->EEpromBlockBase, 1U, kFLASH_UserMarginValue1);
                break;
            case kFLASH_MarginValueFactory:
                returnCode = flash_setfactorymarginlevel(config, config->EEpromBlockBase, 1U, kFLASH_FactoryMarginValue1);
                break;
            case kFLASH_MarginValueNormal:
                returnCode = flash_setusermarginlevel(config, config->EEpromBlockBase, 1U, kFLASH_ReadMarginValueNormal);
                break;
            default:
                returnCode = kStatus_FLASH_InvalidArgument;
                break;
        }
        config->PFlashMarginLevel = margin;
    }
    if (returnCode)
    {
        return returnCode;
    }
#endif

    /* clear error flags */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;
     
    /* Erase verify all flash & EEPROM blocks */
    flash_set_command(0U, 0U, FTMRx_ERASE_VERIFY_ALL_BLOCK);
    
    returnCode = flash_command_sequence(config);

    return returnCode;
}

status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, flash_margin_value_t margin)
{
    uint32_t blockSize;
    flash_operation_config_t flashOperationInfo;
    uint32_t nextBlockStartAddress;
    uint32_t remainingBytes;
    status_t returnCode;

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.sectionCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);
    start = flashOperationInfo.convertedAddress;
    blockSize = flashOperationInfo.activeBlockSize;

    nextBlockStartAddress = ALIGN_UP(start, blockSize);
    if (nextBlockStartAddress == start)
    {
        nextBlockStartAddress += blockSize;
    }

    remainingBytes = lengthInBytes;

    while (remainingBytes)
    {
        uint32_t numberOfPhrases;
        uint32_t verifyLength = nextBlockStartAddress - start;
        if (verifyLength > remainingBytes)
        {
            verifyLength = remainingBytes;
        }
        numberOfPhrases = verifyLength >> flashOperationInfo.sectionCmdAddressAligment;
        if (margin != config->PFlashMarginLevel)
        {
            switch(margin)
            {
                case kFLASH_MarginValueUser:
                    returnCode = flash_setusermarginlevel(config, start, 0U, kFLASH_UserMarginValue1);
                    break;
                case kFLASH_MarginValueFactory:
                    returnCode = flash_setfactorymarginlevel(config, start, 0U, kFLASH_FactoryMarginValue1);
                    break;
                case kFLASH_MarginValueNormal:
                    returnCode = flash_setusermarginlevel(config, start, 0U, kFLASH_ReadMarginValueNormal);
                    break;
                default:
                    returnCode = kStatus_FLASH_InvalidArgument;
                    break;
            }
            config->PFlashMarginLevel = margin;
        }
        if (returnCode)
        {
            return returnCode;
        }
        flash_cache_clear_process(config, kFLASH_CacheClearProcessPre); 
        /* clear error flags */
        FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

        /* Fill in verify FLASH section command parameters. */
        flash_set_command(0U, start >> 16, FTMRx_ERASE_VERIFY_SECTION);
        flash_set_command(1U, start, start >> 8);
        /* Write index to specify the # of longwords to be verified */
        flash_set_command(2U, numberOfPhrases, numberOfPhrases >> 8);

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);
        if (returnCode)
        {
            return returnCode;
        }

        remainingBytes -= verifyLength;
        start += verifyLength;
        nextBlockStartAddress += blockSize;
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_IsProtected(flash_config_t *config,
                           uint32_t start,
                           uint32_t lengthInBytes,
                           flash_protection_state_t *protection_state)
{   
    flash_protection_config_t flashProtectionInfo; /* flash protection information */
    uint32_t endAddress; /* end address for protection check */
    status_t returnCode;
   
    if ((protection_state == NULL) || (lengthInBytes == 0U))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE);
    if (returnCode)
    {
        return returnCode;
    }

    /* Get necessary flash protection information. */
    returnCode = flash_get_protection_info(config, &flashProtectionInfo);
    if (returnCode)
    {
        return returnCode;
    }

    /* calculating Flash end address */
    endAddress = start + lengthInBytes;

    if (((start >= flashProtectionInfo.lowRegionStart) && (endAddress <= flashProtectionInfo.lowRegionEnd)) ||
        ((start >= flashProtectionInfo.highRegionStart) && (endAddress <= flashProtectionInfo.highRegionEnd)))
    {
        *protection_state = kFLASH_ProtectionStateProtected;
    }
    else if (((start >= flashProtectionInfo.lowRegionEnd) && (endAddress <= flashProtectionInfo.highRegionStart)) ||
             (start >= flashProtectionInfo.highRegionEnd) || (endAddress <= flashProtectionInfo.lowRegionStart))
    {
        *protection_state = kFLASH_ProtectionStateUnprotected;
    }
    else
    {
        *protection_state = kFLASH_ProtectionStateMixed;
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value)
{
    if ((config == NULL) || (value == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    switch (whichProperty)
    {
        case kFLASH_PropertyPflashSectorSize:
            *value = config->PFlashSectorSize;
            break;

        case kFLASH_PropertyPflashTotalSize:
            *value = config->PFlashTotalSize;
            break;

        case kFLASH_PropertyPflashBlockSize:
            *value = config->PFlashTotalSize / (uint32_t)config->PFlashBlockCount;
            break;

        case kFLASH_PropertyPflashBlockCount:
            *value = (uint32_t)config->PFlashBlockCount;
            break;

        case kFLASH_PropertyPflashBlockBaseAddr:
            *value = config->PFlashBlockBase;
            break;

        case kFLASH_PropertyPflashFacSupport:
#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL)
            *value = FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL;
#else
            *value = 0;
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
            break;

        case kFLASH_PropertyFlashClockFrequency:
             *value = config->PFlashClockFreq;
            break;

#if FLASH_SSD_IS_EEPROM_ENABLED
        case kFLASH_PropertyEepromBlockBaseAddr:
            *value = config->EEpromBlockBase;
            break;

        case kFLASH_PropertyEepromTotalSize:
            *value = config->EEpromTotalSize;
            break;

        case kFLASH_PropertyEepromSectorSize:
            *value = (uint32_t)config->EEpromSectorSize;
            break;

        case kFLASH_PropertyEepromBlockSize:
            *value = config->EEpromTotalSize / (uint32_t)config->EEpromBlockCount;
            break;

        case kFLASH_PropertyEepromBlockCount:
            *value = (uint32_t)config->EEpromBlockCount;
            break;
#endif /* FLASH_SSD_IS_EEPROM_ENABLED */

        default: /* catch inputs that are not recognized */
            return kStatus_FLASH_UnknownProperty;
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_SetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t value)
{
    status_t status = kStatus_FLASH_Success;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    switch (whichProperty)
    {
#if FLASH_SSD_IS_SECONDARY_FLASH_ENABLED
        case kFLASH_PropertyFlashMemoryIndex:
            if ((value != (uint32_t)kFLASH_MemoryIndexPrimaryFlash) &&
                (value != (uint32_t)kFLASH_MemoryIndexSecondaryFlash))
            {
                return kStatus_FLASH_InvalidPropertyValue;
            }
            config->FlashMemoryIndex = (uint8_t)value;
            break;
#endif /* FLASH_SSD_IS_SECONDARY_FLASH_ENABLED */

        case kFLASH_PropertyFlashCacheControllerIndex:
            if ((value != (uint32_t)kFLASH_CacheControllerIndexForCore0) &&
                (value != (uint32_t)kFLASH_CacheControllerIndexForCore1))
            {
                return kStatus_FLASH_InvalidPropertyValue;
            }
            config->FlashCacheControllerIndex = (uint8_t)value;
            break;
        case kFLASH_PropertyFlashClockFrequency:
             config->PFlashClockFreq = value;
            break;
        case kFLASH_PropertyPflashSectorSize:
        case kFLASH_PropertyPflashTotalSize:
        case kFLASH_PropertyPflashBlockSize:
        case kFLASH_PropertyPflashBlockCount:
        case kFLASH_PropertyPflashBlockBaseAddr:
        case kFLASH_PropertyPflashFacSupport:
#if FLASH_SSD_IS_EEPROM_ENABLED        
        case kFLASH_PropertyEepromBlockBaseAddr:
        case kFLASH_PropertyEepromTotalSize:
        case kFLASH_PropertyEepromSectorSize:
        case kFLASH_PropertyEepromBlockSize:
        case kFLASH_PropertyEepromBlockCount:
#endif /* FLASH_SSD_IS_EEPROM_ENABLED */
            status = kStatus_FLASH_ReadOnlyProperty;
            break;
        default: /* catch inputs that are not recognized */
            status = kStatus_FLASH_UnknownProperty;
            break;
    }

    return status;
}

status_t FLASH_PflashSetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }
    /* The reserved bit6 value of FPROT is reset value, which cannot change.
     * If flash memory is only 8KB, the bit3 to bit5 of FPROT are also reserved.*/
    FTMRx->FPROT = protectStatus->fprotvalue;
    if (protectStatus->fprotvalue != FTMRx->FPROT)
    {
        return kStatus_FLASH_CommandFailure;
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_PflashGetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    protectStatus->fprotvalue = FTMRx->FPROT;

    return kStatus_FLASH_Success;
}


#if FLASH_SSD_IS_EEPROM_ENABLED
status_t FLASH_EepromSetProtection(flash_config_t *config, uint8_t protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->EEpromTotalSize == 0) || (config->EEpromTotalSize == NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    FTMRx->EEPROT = protectStatus;

    if (FTMRx->EEPROT != protectStatus)
    {
        return kStatus_FLASH_CommandFailure;
    }

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_EEPROM_ENABLED */

#if FLASH_SSD_IS_EEPROM_ENABLED
status_t FLASH_EepromGetProtection(flash_config_t *config, uint8_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->EEpromTotalSize == 0) || (config->EEpromTotalSize == NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    *protectStatus = FTMRx->EEPROT;

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_EEPROM_ENABLED */

status_t FLASH_PflashSetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus)
{
#if FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_MCM
    {
        FTMRx_REG32_ACCESS_TYPE regBase;
#if defined(MCM)
        regBase = (FTMRx_REG32_ACCESS_TYPE)&MCM->PLACR;
#elif defined(MCM0)
        regBase = (FTMRx_REG32_ACCESS_TYPE)&MCM0->PLACR;
#endif
        if (speculationStatus->instructionOption == kFLASH_prefetchSpeculationOptionDisable)
        {
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
                return kStatus_FLASH_InvalidSpeculationOption;
            }
            else
            {
                *regBase |= MCM_PLACR_DFCS_MASK;
            }
        }
        else
        {
            *regBase &= ~MCM_PLACR_DFCS_MASK;
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
                *regBase |= MCM_PLACR_EFDS_MASK;
            }
            else
            {
                *regBase &= ~MCM_PLACR_EFDS_MASK;
            }
        }
    }
#elif FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC
    {
        FTMRx_REG32_ACCESS_TYPE regBase;
        uint32_t b0dpeMask, b0ipeMask;
#if defined(FMC_PFB01CR_B0DPE_MASK)
        regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
        b0dpeMask = FMC_PFB01CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB01CR_B0IPE_MASK;
#elif defined(FMC_PFB0CR_B0DPE_MASK)
        regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
        b0dpeMask = FMC_PFB0CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB0CR_B0IPE_MASK;
#endif
        if (speculationStatus->instructionOption == kFLASH_prefetchSpeculationOptionEnable)
        {
            *regBase |= b0ipeMask;
        }
        else
        {
            *regBase &= ~b0ipeMask;
        }
        if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
        {
            *regBase |= b0dpeMask;
        }
        else
        {
            *regBase &= ~b0dpeMask;
        }

/* Invalidate Prefetch Speculation Buffer */
#if defined(FMC_PFB01CR_S_INV_MASK)
        FMC->PFB01CR |= FMC_PFB01CR_S_INV_MASK;
#elif defined(FMC_PFB01CR_S_B_INV_MASK)
        FMC->PFB01CR |= FMC_PFB01CR_S_B_INV_MASK;
#elif defined(FMC_PFB0CR_S_INV_MASK)
        FMC->PFB0CR |= FMC_PFB0CR_S_INV_MASK;
#elif defined(FMC_PFB0CR_S_B_INV_MASK)
        FMC->PFB0CR |= FMC_PFB0CR_S_B_INV_MASK;
#endif
    }
#endif /* FSL_FEATURE_FTMRx_MCM_FLASH_CACHE_CONTROLS */

    return kStatus_FLASH_Success;
}

status_t FLASH_PflashGetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus)
{
    memset(speculationStatus, 0, sizeof(flash_prefetch_speculation_status_t));

    /* Assuming that all speculation options are enabled. */
    speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionEnable;
    speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionEnable;

#if FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_MCM
    {
        uint32_t value;
#if defined(MCM)
        value = MCM->PLACR;
#elif defined(MCM0)
        value = MCM0->PLACR;
#endif
        if (value & MCM_PLACR_DFCS_MASK)
        {
            /* Speculation buffer is off. */
            speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionDisable;
            speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
        }
        else
        {
            /* Speculation buffer is on for instruction. */
            if (!(value & MCM_PLACR_EFDS_MASK))
            {
                /* Speculation buffer is off for data. */
                speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
            }
        }
    }
#elif FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC
    {
        uint32_t value;
        uint32_t b0dpeMask, b0ipeMask;
#if defined(FMC_PFB01CR_B0DPE_MASK)
        value = FMC->PFB01CR;
        b0dpeMask = FMC_PFB01CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB01CR_B0IPE_MASK;
#elif defined(FMC_PFB0CR_B0DPE_MASK)
        value = FMC->PFB0CR;
        b0dpeMask = FMC_PFB0CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB0CR_B0IPE_MASK;
#endif
        if (!(value & b0dpeMask))
        {
            /* Do not prefetch in response to data references. */
            speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
        }
        if (!(value & b0ipeMask))
        {
            /* Do not prefetch in response to instruction fetches. */
            speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionDisable;
        }
    }
#endif

    return kStatus_FLASH_Success;
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Copy PIC of flash_run_command() to RAM
 */
static void copy_flash_run_command(uint32_t *flashRunCommand)
{
    assert(sizeof(s_flashRunCommandFunctionCode) <= (kFLASH_ExecuteInRamFunctionMaxSizeInWords * 4));

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    memcpy((void *)flashRunCommand, (void *)s_flashRunCommandFunctionCode, sizeof(s_flashRunCommandFunctionCode));
    callFlashRunCommand = (void (*)(FTMRx_REG8_ACCESS_TYPE FTMRx_fstat))((uint32_t)flashRunCommand + 1);
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*!
 * @brief Flash Set Command
 *
 * This function is used to write the command sequence to the flash reg.
 */
void flash_set_command(uint8_t index, uint8_t fValue, uint8_t sValue)
{
    FTMRx->FCCOBIX = FTMRx_FCCOBIX_CCOBIX(index);
    FTMRx->FCCOBLO = FTMRx_FCCOBLO_CCOB(fValue);
    FTMRx->FCCOBHI = FTMRx_FCCOBHI_CCOB(sValue);
}

/*!
 * @brief Flash Command Sequence
 *
 * This function is used to perform the command write sequence to the flash.
 *
 * @param driver Pointer to storage for the driver runtime state.
 * @return An error code or kStatus_FLASH_Success
 */
static status_t flash_command_sequence(flash_config_t *config)
{
    uint8_t registerValue;

#if FLASH_DRIVER_IS_FLASH_RESIDENT
    /* clear ACCERR & FPVIOL flag in flash status register */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    status_t returnCode = flash_check_execute_in_ram_function_info(config);
    if (kStatus_FLASH_Success != returnCode)
    {
        return returnCode;
    }

    /* We pass the FTMRx_fstat address as a parameter to flash_run_comamnd() instead of using
     * pre-processed MICRO sentences or operating global variable in flash_run_comamnd()
     * to make sure that flash_run_command() will be compiled into position-independent code (PIC). */
    callFlashRunCommand((FTMRx_REG8_ACCESS_TYPE)(&FTMRx->FSTAT));
#else
#if FLASH_ENABLE_STALLING_FLASH_CONTROLLER
     MCM->PLACR |= MCM_PLACR_ESFC_MASK;          /* enable stalling flash controller when flash is busy */
#endif
   /* clear ACCERR & FPVIOL flag in flash status register */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;
    /* clear CCIF bit */
    FTMRx->FSTAT = FTMRx_FSTAT_CCIF_MASK;
     /* Check CCIF bit of the flash status register, wait till it is set.
     * IP team indicates that this loop will always complete. */
    while (!(FTMRx->FSTAT & FTMRx_FSTAT_CCIF_MASK));
    {

    }
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */
    /* Check error bits */
    /* Get flash status register value */
    registerValue = FTMRx->FSTAT;
    /* checking access error */
    if(registerValue & FTMRx_FSTAT_ACCERR_MASK)
    {
        return kStatus_FLASH_AccessError;
    }
    /* checking protection error */
    if(registerValue & FTMRx_FSTAT_FPVIOL_MASK)
    {
        return kStatus_FLASH_ProtectionViolation;
    }
     /* checking MGSTAT non-correctable error */
    if(registerValue & FTMRx_FSTAT_MGSTAT_MASK)
    {
        return kStatus_FLASH_CommandFailure;
    }

#if FLASH_SSD_IS_EEPROM_ENABLED
    registerValue = FTMRx->FERSTAT;
    
    if(registerValue & (FTMRx_FERSTAT_SFDIF_MASK))
    {
        return kStatus_FLASH_EepromDoubleBitFault;
    }
    if(registerValue & (FTMRx_FERSTAT_DFDIF_MASK))
    {
         return kStatus_FLASH_EepromSingleBitFault;
    }
#endif
    return kStatus_FLASH_Success;
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Copy PIC of flash_common_bit_operation() to RAM
 *
 */
static void copy_flash_common_bit_operation(uint32_t *flashCommonBitOperation)
{
    assert(sizeof(s_flashCommonBitOperationFunctionCode) <= (kFLASH_ExecuteInRamFunctionMaxSizeInWords * 4));

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    memcpy((void *)flashCommonBitOperation, (void *)s_flashCommonBitOperationFunctionCode,
           sizeof(s_flashCommonBitOperationFunctionCode));
    callFlashCommonBitOperation = (void (*)(FTMRx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift,
                                            uint32_t bitValue))((uint32_t)flashCommonBitOperation + 1);
    /* Workround for some devices which doesn't need this function */
    callFlashCommonBitOperation((FTMRx_REG32_ACCESS_TYPE)0, 0, 0, 0);
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

#if FLASH_CACHE_IS_CONTROLLED_BY_MCM
/*! @brief Performs the cache clear to the flash by MCM.*/
void mcm_flash_cache_clear(flash_config_t *config)
{
    FTMRx_REG32_ACCESS_TYPE regBase = (FTMRx_REG32_ACCESS_TYPE)&MCM0_CACHE_REG;

#if defined(MCM0) && defined(MCM1)
    if (config->FlashCacheControllerIndex == (uint8_t)kFLASH_CacheControllerIndexForCore1)
    {
        regBase = (FTMRx_REG32_ACCESS_TYPE)&MCM1_CACHE_REG;
    }
#endif

#if FLASH_DRIVER_IS_FLASH_RESIDENT
    callFlashCommonBitOperation(regBase, MCM_CACHE_CLEAR_MASK, MCM_CACHE_CLEAR_SHIFT, 1U);
#else  /* !FLASH_DRIVER_IS_FLASH_RESIDENT */
    *regBase |= MCM_CACHE_CLEAR_MASK;

    /* Memory barriers for good measure.
     * All Cache, Branch predictor and TLB maintenance operations before this instruction complete */
    __ISB();
    __DSB();
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */
}
#endif /* FLASH_CACHE_IS_CONTROLLED_BY_MCM */

#if FLASH_CACHE_IS_CONTROLLED_BY_FMC
/*! @brief Performs the cache clear to the flash by FMC.*/
void fmc_flash_cache_clear(void)
{
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    FTMRx_REG32_ACCESS_TYPE regBase = (FTMRx_REG32_ACCESS_TYPE)0;
#if defined(FMC_PFB01CR_CINV_WAY_MASK)
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
    callFlashCommonBitOperation(regBase, FMC_PFB01CR_CINV_WAY_MASK, FMC_PFB01CR_CINV_WAY_SHIFT, 0xFU);
#else
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
    callFlashCommonBitOperation(regBase, FMC_PFB0CR_CINV_WAY_MASK, FMC_PFB0CR_CINV_WAY_SHIFT, 0xFU);
#endif
#else /* !FLASH_DRIVER_IS_FLASH_RESIDENT */
#if defined(FMC_PFB01CR_CINV_WAY_MASK)
    FMC->PFB01CR = (FMC->PFB01CR & ~FMC_PFB01CR_CINV_WAY_MASK) | FMC_PFB01CR_CINV_WAY(~0);
#else
    FMC->PFB0CR = (FMC->PFB0CR & ~FMC_PFB0CR_CINV_WAY_MASK) | FMC_PFB0CR_CINV_WAY(~0);
#endif
    /* Memory barriers for good measure.
     * All Cache, Branch predictor and TLB maintenance operations before this instruction complete */
    __ISB();
    __DSB();
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */
}
#endif /* FLASH_CACHE_IS_CONTROLLED_BY_FMC */

#if FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC
/*! @brief Performs the prefetch speculation buffer clear to the flash by FMC.*/
void fmc_flash_prefetch_speculation_clear(void)
{
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    FTMRx_REG32_ACCESS_TYPE regBase = (FTMRx_REG32_ACCESS_TYPE)0;
#if defined(FMC_PFB01CR_S_INV_MASK)
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
    callFlashCommonBitOperation(regBase, FMC_PFB01CR_S_INV_MASK, FMC_PFB01CR_S_INV_SHIFT, 1U);
#elif defined(FMC_PFB01CR_S_B_INV_MASK)
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
    callFlashCommonBitOperation(regBase, FMC_PFB01CR_S_B_INV_MASK, FMC_PFB01CR_S_B_INV_SHIFT, 1U);
#elif defined(FMC_PFB0CR_S_INV_MASK)
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
    callFlashCommonBitOperation(regBase, FMC_PFB0CR_S_INV_MASK, FMC_PFB0CR_S_INV_SHIFT, 1U);
#elif defined(FMC_PFB0CR_S_B_INV_MASK)
    regBase = (FTMRx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
    callFlashCommonBitOperation(regBase, FMC_PFB0CR_S_B_INV_MASK, FMC_PFB0CR_S_B_INV_SHIFT, 1U);
#endif
#else /* !FLASH_DRIVER_IS_FLASH_RESIDENT */
#if defined(FMC_PFB01CR_S_INV_MASK)
    FMC->PFB01CR |= FMC_PFB01CR_S_INV_MASK;
#elif defined(FMC_PFB01CR_S_B_INV_MASK)
    FMC->PFB01CR |= FMC_PFB01CR_S_B_INV_MASK;
#elif defined(FMC_PFB0CR_S_INV_MASK)
    FMC->PFB0CR |= FMC_PFB0CR_S_INV_MASK;
#elif defined(FMC_PFB0CR_S_B_INV_MASK)
    FMC->PFB0CR |= FMC_PFB0CR_S_B_INV_MASK;
#endif
    /* Memory barriers for good measure.
     * All Cache, Branch predictor and TLB maintenance operations before this instruction complete */
    __ISB();
    __DSB();
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */
}
#endif /* FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC */

/*!
 * @brief Flash Cache Clear
 *
 * This function is used to perform the cache and prefetch speculation clear to the flash.
 */
void flash_cache_clear(flash_config_t *config)
{
    flash_cache_clear_process(config, kFLASH_CacheClearProcessPost);
}

/*!
 * @brief Flash Cache Clear Process
 *
 * This function is used to perform the cache and prefetch speculation clear process to the flash.
 */
static void flash_cache_clear_process(flash_config_t *config, flash_cache_clear_process_t process)
{
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    status_t returnCode = flash_check_execute_in_ram_function_info(config);
    if (kStatus_FLASH_Success != returnCode)
    {
        return;
    }
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

    /* We pass the FTMRx register address as a parameter to flash_common_bit_operation() instead of using
     * pre-processed MACROs or a global variable in flash_common_bit_operation()
     * to make sure that flash_common_bit_operation() will be compiled into position-independent code (PIC). */
    if (process == kFLASH_CacheClearProcessPost)
    {
#if FLASH_CACHE_IS_CONTROLLED_BY_MCM
        mcm_flash_cache_clear(config);
#endif
#if FLASH_CACHE_IS_CONTROLLED_BY_FMC
        fmc_flash_cache_clear();
#endif

#if FLASH_PREFETCH_SPECULATION_IS_CONTROLLED_BY_FMC
        fmc_flash_prefetch_speculation_clear();
#endif
    }
    if (process == kFLASH_CacheClearProcessPre)
    {

    }
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief Check whether flash execute-in-ram functions are ready  */
static status_t flash_check_execute_in_ram_function_info(flash_config_t *config)
{
    flash_execute_in_ram_function_config_t *flashExecuteInRamFunctionInfo;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flashExecuteInRamFunctionInfo = (flash_execute_in_ram_function_config_t *)config->flashExecuteInRamFunctionInfo;

    if ((config->flashExecuteInRamFunctionInfo) &&
        (kFLASH_ExecuteInRamFunctionTotalNum == flashExecuteInRamFunctionInfo->activeFunctionCount))
    {
        return kStatus_FLASH_Success;
    }

    return kStatus_FLASH_ExecuteInRamFunctionNotReady;
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Validates the range and alignment of the given address range.*/
static status_t flash_check_range(flash_config_t *config,
                                  uint32_t startAddress,
                                  uint32_t lengthInBytes,
                                  uint32_t alignmentBaseline)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Verify the start and length are alignmentBaseline aligned, Global address [1:0] must be 00.*/
    if ((startAddress & (alignmentBaseline - 1)) || (lengthInBytes & (alignmentBaseline - 1)))
    {
        return kStatus_FLASH_AlignmentError;
    }

    /* check for valid range of the target addresses */
    if ((startAddress >= config->PFlashBlockBase) &&
        ((startAddress + lengthInBytes) <= (config->PFlashBlockBase + config->PFlashTotalSize)))
    {
        return kStatus_FLASH_Success;
    }

    return kStatus_FLASH_AddressError;
}

/*! @brief Gets the right address, sector and block size of current flash type which is indicated by address.*/
static status_t flash_get_matched_operation_info(flash_config_t *config,
                                                 uint32_t address,
                                                 flash_operation_config_t *info)
{
    if ((config == NULL) || (info == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Clean up info Structure*/
    memset(info, 0, sizeof(flash_operation_config_t));

    info->convertedAddress = address - config->PFlashBlockBase;
    info->activeSectorSize = config->PFlashSectorSize;
    info->activeBlockSize = config->PFlashTotalSize / config->PFlashBlockCount;
#if FLASH_SSD_IS_SECONDARY_FLASH_ENABLED
    if (config->FlashMemoryIndex == (uint8_t)kFLASH_MemoryIndexSecondaryFlash)
    {
#if FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER || FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER
        /* When required by the command, address bit 23 selects between main flash memory
         * (=0) and secondary flash memory (=1).*/
        info->convertedAddress += 0x800000U;
#endif
        info->blockWriteUnitSize = SECONDARY_FLASH_FEATURE_PFLASH_BLOCK_WRITE_UNIT_SIZE;
    }
    else
#endif /* FLASH_SSD_IS_SECONDARY_FLASH_ENABLED */
    {
        info->blockWriteUnitSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE;
    }

    info->sectorCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
    info->sectionCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
    info->programCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_PROGRAM_CMD_ADDRESS_ALIGMENT;
    
    return kStatus_FLASH_Success;
}

/*! @brief Validates the given user key for flash erase APIs.*/
static status_t flash_check_user_key(uint32_t key)
{
    /* Validate the user key */
    if (key != kFLASH_ApiEraseKey)
    {
        return kStatus_FLASH_EraseKeyError;
    }

    return kStatus_FLASH_Success;
}

/*! @brief Gets the flash protection information.*/
static status_t flash_get_protection_info(flash_config_t *config, flash_protection_config_t *info)
{
    uint32_t pflashTotalSize, regionBase;
    uint8_t pflashProtectionValue, pflashFPLS;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
    uint8_t pflashFPHS;
#endif    

    if ((config == NULL) || (info == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Clean up info Structure*/
    memset(info, 0, sizeof(flash_protection_config_t));

/* Note: KW40 has a secondary flash, but it doesn't have independent protection register*/
#if FLASH_SSD_IS_SECONDARY_FLASH_ENABLED && (!FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER)
    pflashTotalSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE +
                      FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SIZE;
    regionBase = FSL_FEATURE_FLASH_PFLASH_START_ADDRESS;
#else
    pflashTotalSize = config->PFlashTotalSize;
    regionBase = config->PFlashBlockBase;
#endif

    /* Calculate the flash protection region */
    pflashProtectionValue = FTMRx->FPROT & 0xA4U;
    pflashFPLS = (FTMRx->FPROT & 0x03U);
#if defined(FTMRx_FPROT_FPHDIS_MASK)
    pflashFPHS = (FTMRx->FPROT & 0x18U) >> 3U;
#endif
    
    switch(pflashProtectionValue)
    {
#if defined(FTMRx_FPROT_FPHDIS_MASK)
        case 0xA4U:
            /*!< All flash region is not protected.*/
            info->lowRegionStart = regionBase;
            info->lowRegionEnd = regionBase;
            info->highRegionStart = regionBase + pflashTotalSize;
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
#endif
        case 0x84U:
            info->lowRegionStart = regionBase;
            info->lowRegionEnd = regionBase;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
            /*!< Only flash high region is protected.*/
            info->highRegionStart = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS - ((1U << pflashFPLS) << 10U);
            info->highRegionEnd = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS;
#else
            /*!< All flash region is not protected.*/
            info->highRegionStart = regionBase + pflashTotalSize;
            info->highRegionEnd = regionBase + pflashTotalSize;
#endif
            break;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
        case 0xA0U:
            /*!< Only flash low region is protected.*/
            info->lowRegionStart = regionBase;
            info->lowRegionEnd = regionBase + ((2U << pflashFPLS) << 10U);
            info->highRegionStart = regionBase + pflashTotalSize;
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
#endif
        case 0x80U:
            info->lowRegionStart = regionBase;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
            /*!< Flash high and low region are protected.*/
            info->lowRegionEnd = regionBase + ((2U << pflashFPLS) << 10U);
            info->highRegionStart = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS - ((1U << pflashFPLS) << 10U);
            info->highRegionEnd = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS;
#else
            /*!< Only flash low region is protected.*/
            info->lowRegionEnd = regionBase + ((1U << pflashFPLS) << 10U);
            info->highRegionStart = regionBase + pflashTotalSize;
            info->highRegionEnd = regionBase + pflashTotalSize;
#endif
            break;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
        case 0x24U:
            /*!< All Flash region is protected.*/
            info->lowRegionStart = regionBase;
            info->lowRegionEnd = regionBase + pflashTotalSize;
            info->highRegionStart = regionBase;
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
#endif
        case 0x04U:
            info->lowRegionStart = regionBase;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
            /*!< Flash high region is not protected.*/
            info->lowRegionEnd = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS - ((1U << pflashFPHS) << 10U);
            info->highRegionStart = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS;
            
#else
            /*!< All Flash region is protected.*/            
            info->lowRegionEnd = regionBase + pflashTotalSize;
            info->highRegionStart = regionBase;
#endif
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
#if defined(FTMRx_FPROT_FPHDIS_MASK)
        case 0x20U:
            /*!< Flash low region is not protected.*/
            info->lowRegionStart = regionBase + ((2U << pflashFPLS) << 10U);
            info->lowRegionEnd = regionBase + pflashTotalSize;
            info->highRegionStart = regionBase + ((2U << pflashFPLS) << 10U);
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
#endif
        case 0x00U:
#if defined(FTMRx_FPROT_FPHDIS_MASK)
            /*!< Flash high and low region are not protected.*/
            info->lowRegionStart = regionBase + ((2U << pflashFPLS) << 10U);
            info->lowRegionEnd = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS - ((1U << pflashFPHS) << 10U);
            info->highRegionStart = FSL_FEATURE_FLASH_PFLASH_HIGH_START_ADDRESS;           
#else
            /*!< Flash low region is not protected.*/
            info->lowRegionStart = regionBase;
            info->lowRegionEnd = regionBase;
            info->highRegionStart = regionBase + ((1U << pflashFPLS) << 10U);
#endif
            info->highRegionEnd = regionBase + pflashTotalSize;
            break;
        default:
            break;
    }

    return kStatus_FLASH_Success;
}

/*! @brief Set the flash and eeprom to the specified user margin level.*/
status_t flash_setusermarginlevel(flash_config_t *config, uint32_t start, uint8_t iseeprom, flash_user_margin_value_t margin)
{
    status_t returnCode;
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre); 

    /* clear error flags */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    /* Write index to specify the command code to be loaded */
    flash_set_command(0U, start >> 16, FTMRx_SET_USER_MARGIN_LEVEL);

    /* memory address bits[23:16] with bit23 = 0 for Flash block, 1 for EEPROM block */	
    if(iseeprom)
    {
        FTMRx->FCCOBLO |= FTMRx_FCCOBLO_CCOB(128U);
    }
    flash_set_command(1U, start, start >> 8);

    flash_set_command(2U, margin, margin >> 8);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);
    return returnCode;
}

/*! @brief Set the flash and eeprom to the specified factory margin level.*/
status_t flash_setfactorymarginlevel(flash_config_t *config, uint32_t start, uint8_t iseeprom, flash_factory_margin_value_t margin)
{
    status_t returnCode;
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_cache_clear_process(config, kFLASH_CacheClearProcessPre);   
    /* clear error flags */
    FTMRx->FSTAT = FTMRx_FSTAT_ACCERR_MASK | FTMRx_FSTAT_FPVIOL_MASK;

    /* Write index to specify the command code to be loaded */
    flash_set_command(0U, start >> 16, FTMRx_SET_FACTORY_MARGIN_LEVEL);
    /* memory address bits[23:16] with bit23 = 0 for Flash block, 1 for EEPROM block */	
    if(iseeprom)
    {
        FTMRx->FCCOBLO |= FTMRx_FCCOBLO_CCOB(128U);
    }
    flash_set_command(1U, start, start >> 8);

    flash_set_command(2U, margin, margin >> 8);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);
    return returnCode;
}
