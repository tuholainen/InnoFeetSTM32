/**
  ******************************************************************************
  * @file    stm32l4xx_hal_adc_ex.c
  * @author  MCD Application Team
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Operation functions
  *             ++ Start, stop, get result of conversions of ADC group injected,
  *                using 2 possible modes: polling, interruption.
  *             ++ Calibration
  *               +++ ADC automatic self-calibration
  *               +++ Calibration factors get or set
  *             ++ Multimode feature when available
  *           + Control functions
  *             ++ Channels configuration on ADC group injected
  *           + State functions
  *             ++ ADC group injected contexts queue management
  *          Other functions (generic functions) are available in file 
  *          "stm32l4xx_hal_adc.c".
  *
  @verbatim
  [..] 
  (@) Sections "ADC peripheral features" and "How to use this driver" are
      available in file of generic functions "stm32l4xx_hal_adc.c".
  [..]
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @defgroup ADCEx ADCEx
  * @brief ADC Extended HAL module driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @defgroup ADCEx_Private_Constants ADC Extended Private Constants
  * @{
  */

#define ADC_JSQR_FIELDS  ((uint32_t)(ADC_JSQR_JL | ADC_JSQR_JEXTSEL | ADC_JSQR_JEXTEN |\
                                     ADC_JSQR_JSQ1  | ADC_JSQR_JSQ2 |\
                                      ADC_JSQR_JSQ3 | ADC_JSQR_JSQ4 ))  /*!< ADC_JSQR fields of parameters that can be updated anytime
                                                                             once the ADC is enabled */
                                      
/* Fixed timeout value for ADC calibration.                                   */
/* Values defined to be higher than worst cases: low clock frequency,         */
/* maximum prescalers.                                                        */
/* Ex of profile low frequency : f_ADC at 0.14 MHz (minimum value             */
/* according to Data sheet), calibration_time MAX = 112 / f_ADC               */
/*           112 / 140,000 = 0.8 ms                                           */
/* At maximum CPU speed (80 MHz), this means                                  */
/*    0.8 ms * 80 MHz = 64000 CPU cycles                                      */  
#define ADC_CALIBRATION_TIMEOUT         (64000U)    /*!< ADC calibration time-out value */ 
                                    
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup ADCEx_Exported_Functions ADC Extended Exported Functions
  * @{
  */

/** @defgroup ADCEx_Exported_Functions_Group1 Extended Input and Output operation functions
  * @brief    Extended IO operation functions
  *
@verbatim
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      
      (+) Perform the ADC self-calibration for single or differential ending.
      (+) Get calibration factors for single or differential ending.
      (+) Set calibration factors for single or differential ending.

      (+) Start conversion of ADC group injected.
      (+) Stop conversion of ADC group injected.
      (+) Poll for conversion complete on ADC group injected.
      (+) Get result of ADC group injected channel conversion.
      (+) Start conversion of ADC group injected and enable interruptions.
      (+) Stop conversion of ADC group injected and disable interruptions.
      
      (+) When multimode feature is available, start multimode and enable DMA transfer.
      (+) Stop multimode and disable ADC DMA transfer.
      (+) Get result of multimode conversion.

@endverbatim
  * @{
  */

/**
  * @brief  Perform an ADC automatic self-calibration
  *         Calibration prerequisite: ADC must be disabled (execute this
  *         function before HAL_ADC_Start() or after HAL_ADC_Stop() ).
  * @param  hadc       ADC handle
  * @param  SingleDiff Selection of single-ended or differential input
  *         This parameter can be one of the following values:
  *           @arg @ref ADC_SINGLE_ENDED       Channel in mode input single ended
  *           @arg @ref ADC_DIFFERENTIAL_ENDED Channel in mode input differential ended
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t WaitLoopIndex = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Calibration prerequisite: ADC must be disabled. */
  
  /* Disable the ADC (if not already disabled) */
  tmp_hal_status = ADC_Disable(hadc);
  
  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State, 
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);

    

    /* Select calibration mode single ended or differential ended */
    MODIFY_REG(hadc->Instance->CR, ADC_CR_ADCALDIF, SingleDiff);
    
    /* Start ADC calibration */
    SET_BIT(hadc->Instance->CR, ADC_CR_ADCAL);
    
    /* Wait for calibration completion */
    while(HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADCAL))
    {
      WaitLoopIndex++;
      if (WaitLoopIndex >= ADC_CALIBRATION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_INTERNAL);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_ERROR;
      }
    }
    
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }
  else
  {
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
    
    /* Note: No need to update variable "tmp_hal_status" here: already set    */
    /*       to state "HAL_ERROR" by function disabling the ADC.              */
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Get the calibration factor.
  * @param hadc ADC handle.
  * @param SingleDiff This parameter can be only:
  *           @arg @ref ADC_SINGLE_ENDED       Channel in mode input single ended
  *           @arg @ref ADC_DIFFERENTIAL_ENDED Channel in mode input differential ended
  * @retval Calibration value.
  */
uint32_t HAL_ADCEx_Calibration_GetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff)); 
  
  /* Return the selected ADC calibration value */
  if (SingleDiff == ADC_DIFFERENTIAL_ENDED)
  {
    return ADC_CALFACT_DIFF_GET(hadc->Instance->CALFACT);
  }
  else
  {
    return ((hadc->Instance->CALFACT) & ADC_CALFACT_CALFACT_S);
  }
}

/**
  * @brief  Set the calibration factor to overwrite automatic conversion result.
  *         ADC must be enabled and no conversion is ongoing.
  * @param hadc ADC handle
  * @param SingleDiff This parameter can be only:
  *           @arg @ref ADC_SINGLE_ENDED       Channel in mode input single ended
  *           @arg @ref ADC_DIFFERENTIAL_ENDED Channel in mode input differential ended
  * @param CalibrationFactor Calibration factor (coded on 7 bits maximum)
  * @retval HAL state
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_SetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff, uint32_t CalibrationFactor)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff)); 
  assert_param(IS_ADC_CALFACT(CalibrationFactor)); 
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Verification of hardware constraints before modifying the calibration    */
  /* factors register: ADC must be enabled, no conversion on going.           */
  if ( (ADC_IS_ENABLE(hadc) != RESET)                            &&
       (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)  )
  {
    /* Set the selected ADC calibration value */
    if (SingleDiff == ADC_DIFFERENTIAL_ENDED)
    {
      MODIFY_REG(hadc->Instance->CALFACT, ADC_CALFACT_CALFACT_D, ADC_CALFACT_DIFF_SET(CalibrationFactor));
    }
    else
    {
      MODIFY_REG(hadc->Instance->CALFACT, ADC_CALFACT_CALFACT_S, CalibrationFactor);
    }
  }
  else
  {
    /* Update ADC state machine */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    /* Update ADC error code */
    SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
    
    /* Update ADC state machine to error */
    tmp_hal_status = HAL_ERROR;
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Enable ADC, start conversion of injected group.
  * @note   Interruptions enabled in this function: None.
  * @note   Case of multimode enabled when multimode feature is available: 
  *         HAL_ADCEx_InjectedStart() API must be called for ADC slave first, 
  *         then for ADC master. 
  *         For ADC slave, ADC is enabled only (conversion is not started).  
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param hadc ADC handle.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;  
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc))
  {
    return HAL_BUSY;
  }
  else
  {
  
    /* In case of software trigger detection enabled, JQDIS must be set 
      (which can be done only if ADSTART and JADSTART are both cleared).
       If JQDIS is not set at that point, returns an error
       - since software trigger detection is disabled. User needs to
       resort to HAL_ADCEx_DisableInjectedQueue() API to set JQDIS.  
       - or (if JQDIS is intentionally reset) since JEXTEN = 0 which means 
         the queue is empty */
    if ((READ_BIT(hadc->Instance->JSQR, ADC_JSQR_JEXTEN) == RESET)   
    && (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JQDIS) == RESET))  
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG); 
      return HAL_ERROR;
    }

    
    /* Process locked */
    __HAL_LOCK(hadc);
      
    /* Enable the ADC peripheral */
    tmp_hal_status = ADC_Enable(hadc);
    
    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Check if a regular conversion is ongoing */
      if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_REG_BUSY))
      {
        /* Reset ADC error code field related to injected conversions only */
        CLEAR_BIT(hadc->ErrorCode, HAL_ADC_ERROR_JQOVF);             
      }
      else
      {
        /* Set ADC error code to none */
        ADC_CLEAR_ERRORCODE(hadc);
      }
      
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to injected group conversion results  */
      /* - Set state bitfield related to injected operation                   */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                        HAL_ADC_STATE_INJ_BUSY);
      
      /* Reset HAL_ADC_STATE_MULTIMODE_SLAVE bit
        - by default if ADC is Master or Independent or if multimode feature is not available
        - if multimode setting is set to independent mode (no dual regular or injected conversions are configured) */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }
      
      /* Clear ADC group injected group conversion flag */
      /* (To ensure of no unknown state from potential previous ADC operations) */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JEOC | ADC_FLAG_JEOS));
      
      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);
      
      /* Enable conversion of injected group, if automatic injected conversion  */
      /* is disabled.                                                           */
      /* If software start has been selected, conversion starts immediately.    */
      /* If external trigger has been selected, conversion will start at next   */
      /* trigger event.                                                         */
      /* Case of multimode enabled (when multimode feature is available):       */
      /* if ADC is slave,                                                       */
      /*    - ADC is enabled only (conversion is not started).                  */
      /*    - if multimode only concerns regular conversion, ADC is enabled     */
      /*     and conversion is started.                                         */
      /* If ADC is master or independent,                                       */
      /*    - ADC is enabled and conversion is started.                         */
      
      /* Are injected conversions that of a dual Slave ? */
      if (ADC_INDEPENDENT_OR_NONMULTIMODEINJECTED_SLAVE(hadc))
      {
        /* hadc is not the handle of a Slave ADC with dual injected conversions enabled:
           set ADSTART only if JAUTO is cleared */
        if (HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO))
        {
          SET_BIT(hadc->Instance->CR, ADC_CR_JADSTART) ;
        }
      }
      else
      {
        /* hadc is the handle of a Slave ADC with dual injected conversions enabled:
           ADSTART is not set */
          SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }
    } 
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);    
    }
  
    /* Return function status */
    return tmp_hal_status;
  }
}

/**
  * @brief  Stop conversion of injected channels. Disable ADC peripheral if
  *         no regular conversion is on going.
  * @note   If ADC must be disabled and if conversion is on going on 
  *         regular group, function HAL_ADC_Stop must be used to stop both
  *         injected and regular groups, and disable the ADC.
  * @note   If injected group mode auto-injection is enabled,
  *         function HAL_ADC_Stop must be used.
  * @note   In case of multimode enabled (when multimode feature is available), 
  *         HAL_ADCEx_InjectedStop() must be called for ADC master first, then for ADC slave.
  *         For ADC master, conversion is stopped and ADC is disabled. 
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @param hadc ADC handle.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential conversion on going on injected group only. */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_INJECTED_GROUP);
  
  /* Disable ADC peripheral if injected conversions are effectively stopped   */
  /* and if no conversion on regular group is on-going                       */
  if (tmp_hal_status == HAL_OK)
  {
    if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
    {
      /* 2. Disable the ADC peripheral */
      tmp_hal_status = ADC_Disable(hadc);
      
      /* Check if ADC is effectively disabled */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    /* Conversion on injected group is stopped, but ADC not disabled since    */
    /* conversion on regular group is still running.                          */
    else
    {
      /* Set ADC state */
      CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Wait for injected group conversion to be completed.
  * @param hadc ADC handle
  * @param Timeout Timeout value in millisecond.
  * @note   Depending on hadc->Init.EOCSelection, JEOS or JEOC is
  *         checked and cleared depending on AUTDLY bit status.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedPollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart = 0U;
  uint32_t tmp_Flag_End = 0U;
  uint32_t tmp_cfgr = 0U;
  ADC_TypeDef *tmpADC_Master;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* If end of sequence selected */
  if (hadc->Init.EOCSelection == ADC_EOC_SEQ_CONV)
  {
    tmp_Flag_End = ADC_FLAG_JEOS;
  }
  else /* end of conversion selected */
  {
    tmp_Flag_End = ADC_FLAG_JEOC;
  }
  
  /* Get timeout */
  tickstart = HAL_GetTick();  
     
   /* Wait until End of Conversion or Sequence flag is raised */
  while(HAL_IS_BIT_CLR(hadc->Instance->ISR, tmp_Flag_End))
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0) || ((HAL_GetTick()-tickstart) > Timeout))
      {
        /* Update ADC state machine to timeout */
        SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_TIMEOUT;
      }
    }
  }
  
  /* Get relevant register CFGR in ADC instance of ADC master or slave  */
  /* in function of multimode state (for devices with multimode         */
  /* available).                                                        */
  if (ADC_INDEPENDENT_OR_NONMULTIMODEINJECTED_SLAVE(hadc) == SET)
  {
    tmp_cfgr = READ_REG(hadc->Instance->CFGR); 
  }
  else
  {
    tmpADC_Master = ADC_MASTER_REGISTER(hadc);
    tmp_cfgr = READ_REG(tmpADC_Master->CFGR); 
  }
  
  /* Update ADC state machine */
  SET_BIT(hadc->State, HAL_ADC_STATE_INJ_EOC);
  
  /* Determine whether any further conversion upcoming on group injected      */
  /* by external trigger or by automatic injected conversion                  */
  /* from group regular.                                                      */
  if(ADC_IS_SOFTWARE_START_INJECTED(hadc)                   ||
     ((READ_BIT (tmp_cfgr, ADC_CFGR_JAUTO) == RESET)    &&
      (ADC_IS_SOFTWARE_START_REGULAR(hadc)          &&
      (READ_BIT (tmp_cfgr, ADC_CFGR_CONT) == RESET)   )   )   )
  {
    /* Check whether end of sequence is reached */
    if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOS) )
    {
      /* Particular case if injected contexts queue is enabled:             */
      /* when the last context has been fully processed, JSQR is reset      */
      /* by the hardware. Even if no injected conversion is planned to come */
      /* (queue empty, triggers are ignored), it can start again            */
      /* immediately after setting a new context (JADSTART is still set).   */
      /* Therefore, state of HAL ADC injected group is kept to busy.        */
      if(READ_BIT(tmp_cfgr, ADC_CFGR_JQM) == RESET)
      {
        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
        
        if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
        {
          SET_BIT(hadc->State, HAL_ADC_STATE_READY);
        }
      }
    }
  }
  
  /* Clear polled flag */
  if (tmp_Flag_End == ADC_FLAG_JEOS)
  {
    /* Clear end of sequence JEOS flag of injected group if low power feature */
    /* "LowPowerAutoWait " is disabled, to not interfere with this feature.   */
    /* For injected groups, no new conversion will start before JEOS is       */
    /* cleared.                                                               */
    if (READ_BIT (tmp_cfgr, ADC_CFGR_AUTDLY) == RESET)
    {
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JEOC | ADC_FLAG_JEOS));
    }
  }
  else
  {
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JEOC);
  }
  
  /* Return API HAL status */
  return HAL_OK;
}

/**
  * @brief  Enable ADC, start conversion of injected group with interruption.
  * @note   Interruptions enabled in this function according to initialization
  *         setting : JEOC (end of conversion) or JEOS (end of sequence) 
  * @note   Case of multimode enabled (when multimode feature is enabled): 
  *         HAL_ADCEx_InjectedStart_IT() API must be called for ADC slave first, 
  *         then for ADC master. 
  *         For ADC slave, ADC is enabled only (conversion is not started).  
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param hadc ADC handle.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc))
  {
    return HAL_BUSY;
  }
  else
  {
    /* In case of software trigger detection enabled, JQDIS must be set 
      (which can be done only if ADSTART and JADSTART are both cleared).
       If JQDIS is not set at that point, returns an error
       - since software trigger detection is disabled. User needs to
       resort to HAL_ADCEx_DisableInjectedQueue() API to set JQDIS.  
       - or (if JQDIS is intentionally reset) since JEXTEN = 0 which means 
         the queue is empty */
    if ((READ_BIT(hadc->Instance->JSQR, ADC_JSQR_JEXTEN) == RESET)   
    && (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JQDIS) == RESET))  
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG); 
      return HAL_ERROR;
    }

    /* Process locked */
    __HAL_LOCK(hadc);
    
    /* Enable the ADC peripheral */
    tmp_hal_status = ADC_Enable(hadc);
    
    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Check if a regular conversion is ongoing */
      if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_REG_BUSY))
      {
        /* Reset ADC error code field related to injected conversions only */
        CLEAR_BIT(hadc->ErrorCode, HAL_ADC_ERROR_JQOVF);            
      }
      else
      {
        /* Set ADC error code to none */
        ADC_CLEAR_ERRORCODE(hadc);
      }
      
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to injected group conversion results  */
      /* - Set state bitfield related to injected operation                   */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                        HAL_ADC_STATE_INJ_BUSY);
      
      /* Reset HAL_ADC_STATE_MULTIMODE_SLAVE bit
        - by default if ADC is Master or Independent or if multimode feature is not available
        - if multimode setting is set to independent mode (no dual regular or injected conversions are configured) */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }

      /* Clear ADC group injected group conversion flag */
      /* (To ensure of no unknown state from potential previous ADC operations) */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JEOC | ADC_FLAG_JEOS));
      
      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);
      
      /* Enable ADC Injected context queue overflow interrupt if this feature   */
      /* is enabled.                                                            */
      if ((hadc->Instance->CFGR & ADC_CFGR_JQM) != RESET)
      {
        __HAL_ADC_ENABLE_IT(hadc, ADC_FLAG_JQOVF);
      }
      
      /* Enable ADC end of conversion interrupt */
      switch(hadc->Init.EOCSelection)
      {
        case ADC_EOC_SEQ_CONV: 
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOC);
          __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOS);
          break;
        /* case ADC_EOC_SINGLE_CONV */
        default:
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOS);      
          __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOC);
          break;
      }
      
      /* Enable conversion of injected group, if automatic injected conversion  */
      /* is disabled.                                                           */
      /* If software start has been selected, conversion starts immediately.    */
      /* If external trigger has been selected, conversion will start at next   */
      /* trigger event.                                                         */
      /* Case of multimode enabled (when multimode feature is available):       */
      /* if ADC is slave,                                                       */
      /*    - ADC is enabled only (conversion is not started),                  */
      /*    - if multimode only concerns regular conversion, ADC is enabled     */
      /*     and conversion is started.                                         */
      /* If ADC is master or independent,                                       */
      /*    - ADC is enabled and conversion is started.                         */

      /* Are injected conversions that of a dual Slave ? */      
      if (ADC_INDEPENDENT_OR_NONMULTIMODEINJECTED_SLAVE(hadc))
      {
        /* hadc is not the handle of a Slave ADC with dual injected conversions enabled:
           set ADSTART only if JAUTO is cleared */
        if (HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO))
        {
          SET_BIT(hadc->Instance->CR, ADC_CR_JADSTART) ;
        }
      }
      else
      {
        /* hadc is the handle of a Slave ADC with dual injected conversions enabled:
           ADSTART is not set */
          SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
    
    /* Return function status */
    return tmp_hal_status;
  }
}

/**
  * @brief  Stop conversion of injected channels, disable interruption of 
  *         end-of-conversion. Disable ADC peripheral if no regular conversion
  *         is on going.
  * @note   If ADC must be disabled and if conversion is on going on 
  *         regular group, function HAL_ADC_Stop must be used to stop both
  *         injected and regular groups, and disable the ADC.
  * @note   If injected group mode auto-injection is enabled,
  *         function HAL_ADC_Stop must be used.
  * @note   Case of multimode enabled (when multimode feature is available): 
  *         HAL_ADCEx_InjectedStop_IT() API must be called for ADC master first, 
  *         then for ADC slave.
  *         For ADC master, conversion is stopped and ADC is disabled. 
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @note   In case of auto-injection mode, HAL_ADC_Stop() must be used.
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential conversion on going on injected group only. */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_INJECTED_GROUP);
  
  /* Disable ADC peripheral if injected conversions are effectively stopped   */
  /* and if no conversion on the other group (regular group) is intended to   */
  /* continue.                                                                */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC end of conversion interrupt for injected channels */
    __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_JEOC | ADC_IT_JEOS | ADC_FLAG_JQOVF));
    
    if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
    {
      /* 2. Disable the ADC peripheral */
      tmp_hal_status = ADC_Disable(hadc);
      
      /* Check if ADC is effectively disabled */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    /* Conversion on injected group is stopped, but ADC not disabled since    */
    /* conversion on regular group is still running.                          */
    else
    {
      /* Set ADC state */
      CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
    }
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

#if defined(ADC_MULTIMODE_SUPPORT)
/**
  * @brief  Enable ADC, start MultiMode conversion and transfer regular results through DMA.
  * @note   Multimode must have been previously configured using 
  *         HAL_ADCEx_MultiModeConfigChannel() function.
  *         Interruptions enabled in this function:
  *          overrun, DMA half transfer, DMA transfer complete. 
  *         Each of these interruptions has its dedicated callback function.
  * @note   State field of Slave ADC handle is not updated in this configuration: 
  *          user should not rely on it for information related to Slave regular 
  *         conversions.                                                                          
  * @param hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @param pData Destination Buffer address.
  * @param Length Length of data to be transferred from ADC peripheral to memory (in bytes).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeStart_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_HandleTypeDef tmphadcSlave;
  ADC_Common_TypeDef *tmpADC_Common;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_EXTTRIG_EDGE(hadc->Init.ExternalTrigConvEdge));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAContinuousRequests));
  
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc))
  {
    return HAL_BUSY;
  }
  else
  {
    /* Process locked */
    __HAL_LOCK(hadc);

    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);
  
    if (tmphadcSlave.Instance == NULL)
    {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    
      return HAL_ERROR;
    }
    
    /* Enable the ADC peripherals: master and slave (in case if not already   */
    /* enabled previously)                                                    */
    tmp_hal_status = ADC_Enable(hadc);
    if (tmp_hal_status == HAL_OK)
    {
      tmp_hal_status = ADC_Enable(&tmphadcSlave);
    }
  
    /* Start multimode conversion of ADCs pair */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        (HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP),
                        HAL_ADC_STATE_REG_BUSY);
      
      /* Set ADC error code to none */
      ADC_CLEAR_ERRORCODE(hadc);
      
      /* Set the DMA transfer complete callback */
      hadc->DMA_Handle->XferCpltCallback = ADC_DMAConvCplt;
         
      /* Set the DMA half transfer complete callback */
      hadc->DMA_Handle->XferHalfCpltCallback = ADC_DMAHalfConvCplt;
      
      /* Set the DMA error callback */
      hadc->DMA_Handle->XferErrorCallback = ADC_DMAError ;
      
      /* Pointer to the common control register  */
      tmpADC_Common = __LL_ADC_COMMON_INSTANCE(hadc->Instance);
      
      /* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC     */
      /* start (in case of SW start):                                           */
  
      /* Clear regular group conversion flag and overrun flag */
      /* (To ensure of no unknown state from potential previous ADC operations) */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));
      
      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);
      
      /* Enable ADC overrun interrupt */
      __HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);
      
      /* Start the DMA channel */
      HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&tmpADC_Common->CDR, (uint32_t)pData, Length);
          
      /* Enable conversion of regular group.                                    */
      /* If software start has been selected, conversion starts immediately.    */
      /* If external trigger has been selected, conversion will start at next   */
      /* trigger event.                                                         */
      /* Start ADC group regular conversion */
      LL_ADC_REG_StartConversion(hadc->Instance);
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
    
    /* Return function status */
    return tmp_hal_status;
  }
}

/**
  * @brief  Stop multimode ADC conversion, disable ADC DMA transfer, disable ADC peripheral.
  * @note   Multimode is kept enabled after this function. MultiMode DMA bits
  *         (MDMA and DMACFG bits of common CCR register) are maintained. To disable 
  *         Multimode (set with HAL_ADCEx_MultiModeConfigChannel()), ADC must be 
  *         reinitialized using HAL_ADC_Init() or HAL_ADC_DeInit(), or the user can
  *         resort to HAL_ADCEx_DisableMultiMode() API.
  * @note   In case of DMA configured in circular mode, function 
  *         HAL_ADC_Stop_DMA() must be called after this function with handle of
  *         ADC slave, to properly disable the DMA channel.
  * @param hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;
  ADC_HandleTypeDef tmphadcSlave;
  
  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  
  /* 1. Stop potential multimode conversion on going, on regular and injected groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {                                      
    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);
    
    if (tmphadcSlave.Instance == NULL)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
      
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
      
      return HAL_ERROR;
    }
    
    /* Procedure to disable the ADC peripheral: wait for conversions          */
    /* effectively stopped (ADC master and ADC slave), then disable ADC       */
    
    /* 1. Wait until ADSTP=0 for ADC master and ADC slave*/
    tickstart = HAL_GetTick();  
    
    while(ADC_IS_CONVERSION_ONGOING_REGULAR(hadc)          || 
          ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSlave)   )
    {
      if((HAL_GetTick()-tickstart) > ADC_STOP_CONVERSION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_ERROR;
      }
    }
    
    /* Disable the DMA channel (in case of DMA in circular mode or stop       */
    /* while DMA transfer is on going)                                        */
    /* Note: DMA channel of ADC slave should be stopped after this function   */
    /*       with HAL_ADC_Stop_DMA() API.                                     */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);
    
    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status == HAL_ERROR)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA); 
    }
    
    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
    
    /* 2. Disable the ADC peripherals: master and slave */
    /* Update "tmp_hal_status" only if DMA channel disabling passed, to keep in */
    /* memory a potential failing status.                                     */
    if (tmp_hal_status == HAL_OK)
    {
      /* Check if ADC are effectively disabled */
      if ((ADC_Disable(hadc) == HAL_OK)         &&
          (ADC_Disable(&tmphadcSlave) == HAL_OK)   )
      {
        tmp_hal_status = HAL_OK;
      }
    }
    else
    {
      ADC_Disable(hadc);
      ADC_Disable(&tmphadcSlave);
    }
    
    /* Set ADC state (ADC master) */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_READY);
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Return the last ADC Master and Slave regular conversions results when in multimode configuration.
  * @param hadc ADC handle of ADC Master (handle of ADC Slave must not be used)
  * @retval The converted data values.
  */
uint32_t HAL_ADCEx_MultiModeGetValue(ADC_HandleTypeDef* hadc)
{
  ADC_Common_TypeDef *tmpADC_Common;
  
  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  
  /* Pointer to the common control register  */
  tmpADC_Common = __LL_ADC_COMMON_INSTANCE(hadc->Instance);
  
  /* Return the multi mode conversion value */
  return tmpADC_Common->CDR;
}
#endif /* ADC_MULTIMODE_SUPPORT */

/**
  * @brief  Get ADC injected group conversion result.
  * @note   Reading register JDRx automatically clears ADC flag JEOC
  *         (ADC group injected end of unitary conversion).
  * @note   This function does not clear ADC flag JEOS 
  *         (ADC group injected end of sequence conversion)
  *         Occurrence of flag JEOS rising:
  *          - If sequencer is composed of 1 rank, flag JEOS is equivalent
  *            to flag JEOC.
  *          - If sequencer is composed of several ranks, during the scan
  *            sequence flag JEOC only is raised, at the end of the scan sequence
  *            both flags JEOC and EOS are raised.
  *         Flag JEOS must not be cleared by this function because
  *         it would not be compliant with low power features
  *         (feature low power auto-wait, not available on all STM32 families).
  *         To clear this flag, either use function: 
  *         in programming model IT: @ref HAL_ADC_IRQHandler(), in programming
  *         model polling: @ref HAL_ADCEx_InjectedPollForConversion() 
  *         or @ref __HAL_ADC_CLEAR_FLAG(&hadc, ADC_FLAG_JEOS).
  * @param hadc ADC handle
  * @param InjectedRank the converted ADC injected rank.
  *          This parameter can be one of the following values:
  *            @arg @ref ADC_INJECTED_RANK_1 ADC group injected rank 1
  *            @arg @ref ADC_INJECTED_RANK_2 ADC group injected rank 2
  *            @arg @ref ADC_INJECTED_RANK_3 ADC group injected rank 3
  *            @arg @ref ADC_INJECTED_RANK_4 ADC group injected rank 4
  * @retval ADC group injected conversion data
  */
uint32_t HAL_ADCEx_InjectedGetValue(ADC_HandleTypeDef* hadc, uint32_t InjectedRank)
{
  uint32_t tmp_jdr = 0;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_INJECTED_RANK(InjectedRank));
  
  /* Get ADC converted value */
  switch(InjectedRank)
  {  
    case ADC_INJECTED_RANK_4:
      tmp_jdr = hadc->Instance->JDR4;
      break;
    case ADC_INJECTED_RANK_3: 
      tmp_jdr = hadc->Instance->JDR3;
      break;
    case ADC_INJECTED_RANK_2: 
      tmp_jdr = hadc->Instance->JDR2;
      break;
    case ADC_INJECTED_RANK_1:
    default:
      tmp_jdr = hadc->Instance->JDR1;
      break;
  }
  
  /* Return ADC converted value */ 
  return tmp_jdr;
}

/**
  * @brief  Injected conversion complete callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_InjectedConvCpltCallback must be implemented in the user file.
  */
}

/**
  * @brief  Injected context queue overflow callback.
  * @note   This callback is called if injected context queue is enabled
            (parameter "QueueInjectedContext" in injected channel configuration)
            and if a new injected context is set when queue is full (maximum 2
            contexts).
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_InjectedQueueOverflowCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_InjectedQueueOverflowCallback must be implemented in the user file.
  */
}

/**
  * @brief  Analog watchdog 2 callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_LevelOutOfWindow2Callback must be implemented in the user file.
  */
}

/**
  * @brief  Analog watchdog 3 callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow3Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_LevelOutOfWindow3Callback must be implemented in the user file.
  */
}


/**
  * @brief  End Of Sampling callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_EndOfSamplingCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_EndOfSamplingCallback must be implemented in the user file.
  */
}

/**
  * @brief  Stop ADC conversion of regular group (and injected channels in 
  *         case of auto_injection mode), disable ADC peripheral if no 
  *         conversion is on going on injected group.
  * @param hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential regular conversion on going */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);
  
  /* Disable ADC peripheral if regular conversions are effectively stopped 
     and if no injected conversions are on-going */
  if (tmp_hal_status == HAL_OK)
  {
    /* Clear HAL_ADC_STATE_REG_BUSY bit */ 
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY); 
     
    if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
    {
      /* 2. Disable the ADC peripheral */
      tmp_hal_status = ADC_Disable(hadc);
      
      /* Check if ADC is effectively disabled */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    /* Conversion on injected group is stopped, but ADC not disabled since    */
    /* conversion on regular group is still running.                          */
    else
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}


/**
  * @brief  Stop ADC conversion of ADC groups regular and injected,
  *         disable interrution of end-of-conversion,
  *         disable ADC peripheral if no conversion is on going
  *         on injected group.
  * @param hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential regular conversion on going */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);
  
  /* Disable ADC peripheral if conversions are effectively stopped
    and if no injected conversion is on-going */
  if (tmp_hal_status == HAL_OK)
  {
    /* Clear HAL_ADC_STATE_REG_BUSY bit */ 
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);  
    
    /* Disable all regular-related interrupts */
    __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_EOC | ADC_IT_EOS | ADC_IT_OVR));
    
    /* 2. Disable ADC peripheral if no injected conversions are on-going */
    if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
    {     
      tmp_hal_status = ADC_Disable(hadc);
      /* if no issue reported */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    else
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC conversion of regular group (and injected group in 
  *         case of auto_injection mode), disable ADC DMA transfer, disable 
  *         ADC peripheral if no conversion is on going
  *         on injected group.
  * @note   HAL_ADCEx_RegularStop_DMA() function is dedicated to single-ADC mode only. 
  *         For multimode (when multimode feature is available), 
  *         HAL_ADCEx_RegularMultiModeStop_DMA() API must be used.
  * @param hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential regular conversion on going */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);
  
  /* Disable ADC peripheral if conversions are effectively stopped 
     and if no injected conversion is on-going */
  if (tmp_hal_status == HAL_OK)
  {
    /* Clear HAL_ADC_STATE_REG_BUSY bit */ 
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
    
    /* Disable ADC DMA (ADC DMA configuration ADC_CFGR_DMACFG is kept) */
    CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_DMAEN); 
    
    /* Disable the DMA channel (in case of DMA in circular mode or stop while */
    /* while DMA transfer is on going)                                        */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);   
    
    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status != HAL_OK)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);   
    }
    
    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
    
    /* 2. Disable the ADC peripheral */
    /* Update "tmp_hal_status" only if DMA channel disabling passed, to keep in */
    /* memory a potential failing status.                                     */
    if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
    {    
      if (tmp_hal_status == HAL_OK)
      {
        tmp_hal_status = ADC_Disable(hadc);
      }
      else
      {
        ADC_Disable(hadc);
      }
      
      /* Check if ADC is effectively disabled */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    else
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);  
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

#if defined(ADC_MULTIMODE_SUPPORT)
/**
  * @brief  Stop DMA-based multimode ADC conversion, disable ADC DMA transfer, disable ADC peripheral if no injected conversion is on-going.
  * @note   Multimode is kept enabled after this function. Multimode DMA bits
  *         (MDMA and DMACFG bits of common CCR register) are maintained. To disable 
  *         multimode (set with HAL_ADCEx_MultiModeConfigChannel()), ADC must be 
  *         reinitialized using HAL_ADC_Init() or HAL_ADC_DeInit(), or the user can
  *         resort to HAL_ADCEx_DisableMultiMode() API.  
  * @note   In case of DMA configured in circular mode, function 
  *         HAL_ADCEx_RegularStop_DMA() must be called after this function with handle of
  *         ADC slave, to properly disable the DMA channel.
  * @param hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_RegularMultiModeStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;
  ADC_HandleTypeDef tmphadcSlave;
  
  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  
  /* 1. Stop potential multimode conversion on going, on regular groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Clear HAL_ADC_STATE_REG_BUSY bit */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
    
    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);
    
    if (tmphadcSlave.Instance == NULL)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
      
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
      
      return HAL_ERROR;
    }
    
    /* Procedure to disable the ADC peripheral: wait for conversions          */
    /* effectively stopped (ADC master and ADC slave), then disable ADC       */
    
    /* 1. Wait until ADSTP=0 for ADC master and ADC slave*/
    tickstart = HAL_GetTick();  
    
    while(ADC_IS_CONVERSION_ONGOING_REGULAR(hadc)          || 
          ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSlave)   )
    {
      if((HAL_GetTick()-tickstart) > ADC_STOP_CONVERSION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_ERROR;
      }
    }
    
    /* Disable the DMA channel (in case of DMA in circular mode or stop       */
    /* while DMA transfer is on going)                                        */
    /* Note: DMA channel of ADC slave should be stopped after this function   */
    /* with HAL_ADCEx_RegularStop_DMA() API.                                  */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);
    
    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status != HAL_OK)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);  
    }
    
    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
    
    /* 2. Disable the ADC peripherals: master and slave if no injected        */
    /*   conversion is on-going.                                              */
    /* Update "tmp_hal_status" only if DMA channel disabling passed, to keep in */
    /* memory a potential failing status.                                     */
    if (tmp_hal_status == HAL_OK)
    {
       if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
       {  
         tmp_hal_status =  ADC_Disable(hadc);
         if (tmp_hal_status == HAL_OK)
         {
           if (ADC_IS_CONVERSION_ONGOING_INJECTED(&tmphadcSlave) == RESET)
           {
             tmp_hal_status =  ADC_Disable(&tmphadcSlave);
           }
         }
       }
       
       if (tmp_hal_status == HAL_OK)
       {
         /* Both Master and Slave ADC's could be disabled. Update Master State */
         /* Clear HAL_ADC_STATE_INJ_BUSY bit, set HAL_ADC_STATE_READY bit */
         ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY, HAL_ADC_STATE_READY);
       }
       else
       {
         /* injected (Master or Slave) conversions are still on-going,
            no Master State change */
       }
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}
#endif /* ADC_MULTIMODE_SUPPORT */

/**
  * @}
  */

/** @defgroup ADCEx_Exported_Functions_Group2 ADC Extended Peripheral Control functions
  * @brief    ADC Extended Peripheral Control functions
  *
@verbatim   
 ===============================================================================
             ##### Peripheral Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure channels on injected group
      (+) Configure multimode when multimode feature is available
      (+) Enable or Disable Injected Queue
      (+) Disable ADC voltage regulator
      (+) Enter ADC deep-power-down mode
      
@endverbatim
  * @{
  */

/**
  * @brief  Configure a channel to be assigned to ADC group injected.
  * @note   Possibility to update parameters on the fly:
  *         This function initializes injected group, following calls to this 
  *         function can be used to reconfigure some parameters of structure
  *         "ADC_InjectionConfTypeDef" on the fly, without resetting the ADC.
  *         The setting of these parameters is conditioned to ADC state:
  *         Refer to comments of structure "ADC_InjectionConfTypeDef".
  * @note   In case of usage of internal measurement channels:
  *         Vbat/VrefInt/TempSensor.
  *         These internal paths can be disabled using function 
  *         HAL_ADC_DeInit().
  * @note   Caution: For Injected Context Queue use, a context must be fully 
  *         defined before start of injected conversion. All channels are configured 
  *         consecutively for the same ADC instance. Therefore, the number of calls to 
  *         HAL_ADCEx_InjectedConfigChannel() must be equal to the value of parameter 
  *         InjectedNbrOfConversion for each context.
  *  - Example 1: If 1 context is intended to be used (or if there is no use of the 
  *    Injected Queue Context feature) and if the context contains 3 injected ranks 
  *    (InjectedNbrOfConversion = 3), HAL_ADCEx_InjectedConfigChannel() must be  
  *    called once for each channel (i.e. 3 times) before starting a conversion.   
  *    This function must not be called to configure a 4th injected channel:   
  *    it would start a new context into context queue.
  *  - Example 2: If 2 contexts are intended to be used and each of them contains 
  *    3 injected ranks (InjectedNbrOfConversion = 3),  
  *    HAL_ADCEx_InjectedConfigChannel() must be called once for each channel and  
  *    for each context (3 channels x 2 contexts = 6 calls). Conversion can  
  *    start once the 1st context is set, that is after the first three 
  *    HAL_ADCEx_InjectedConfigChannel() calls. The 2nd context can be set on the fly.
  * @param hadc ADC handle
  * @param sConfigInjected Structure of ADC injected group and ADC channel for
  *         injected group.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tmpOffsetShifted;
  uint32_t wait_loop_index = 0U;
  
  uint32_t tmp_JSQR_ContextQueueBeingBuilt = 0U;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SAMPLE_TIME(sConfigInjected->InjectedSamplingTime));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(sConfigInjected->InjectedSingleDiff));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->AutoInjectedConv));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->QueueInjectedContext));
  assert_param(IS_ADC_EXTTRIGINJEC_EDGE(sConfigInjected->ExternalTrigInjecConvEdge));
  assert_param(IS_ADC_EXTTRIGINJEC(hadc, sConfigInjected->ExternalTrigInjecConv));
  assert_param(IS_ADC_OFFSET_NUMBER(sConfigInjected->InjectedOffsetNumber));
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), sConfigInjected->InjectedOffset));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjecOversamplingMode));
  
  if(hadc->Init.ScanConvMode != ADC_SCAN_DISABLE)
  {
    assert_param(IS_ADC_INJECTED_RANK(sConfigInjected->InjectedRank));
    assert_param(IS_ADC_INJECTED_NB_CONV(sConfigInjected->InjectedNbrOfConversion));
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjectedDiscontinuousConvMode));
  }
  
  
  /* if JOVSE is set, the value of the OFFSETy_EN bit in ADCx_OFRy register is  
     ignored (considered as reset) */
  assert_param(!((sConfigInjected->InjectedOffsetNumber != ADC_OFFSET_NONE) && (sConfigInjected->InjecOversamplingMode == ENABLE))); 
  
  /* JDISCEN and JAUTO bits can't be set at the same time  */
  assert_param(!((sConfigInjected->InjectedDiscontinuousConvMode == ENABLE) && (sConfigInjected->AutoInjectedConv == ENABLE))); 
  
  /*  DISCEN and JAUTO bits can't be set at the same time */
  assert_param(!((hadc->Init.DiscontinuousConvMode == ENABLE) && (sConfigInjected->AutoInjectedConv == ENABLE)));  
  
  /* Verification of channel number */
  if (sConfigInjected->InjectedSingleDiff != ADC_DIFFERENTIAL_ENDED)
  {
     assert_param(IS_ADC_CHANNEL(hadc, sConfigInjected->InjectedChannel));
  }
  else
  {
    assert_param(IS_ADC_DIFF_CHANNEL(hadc, sConfigInjected->InjectedChannel));
  }
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Configuration of injected group sequencer:                               */
  /* Hardware constraint: Must fully define injected context register JSQR    */
  /* before make it entering into injected sequencer queue.                   */
  /*                                                                          */
  /* - if scan mode is disabled:                                              */
  /*    * Injected channels sequence length is set to 0x00: 1 channel         */
  /*      converted (channel on injected rank 1)                              */
  /*      Parameter "InjectedNbrOfConversion" is discarded.                   */
  /*    * Injected context register JSQR setting is simple: register is fully */
  /*      defined on one call of this function (for injected rank 1) and can  */
  /*      be entered into queue directly.                                     */
  /* - if scan mode is enabled:                                               */
  /*    * Injected channels sequence length is set to parameter               */
  /*      "InjectedNbrOfConversion".                                          */
  /*    * Injected context register JSQR setting more complex: register is    */
  /*      fully defined over successive calls of this function, for each      */
  /*      injected channel rank. It is entered into queue only when all       */
  /*      injected ranks have been set.                                       */
  /*   Note: Scan mode is not present by hardware on this device, but used    */
  /*   by software for alignment over all STM32 devices.                      */
  
  if ((hadc->Init.ScanConvMode == ADC_SCAN_DISABLE)  ||
      (sConfigInjected->InjectedNbrOfConversion == 1U)  )
  {
    /* Configuration of context register JSQR:                                */
    /*  - number of ranks in injected group sequencer: fixed to 1st rank      */
    /*    (scan mode disabled, only rank 1 used)                              */
    /*  - external trigger to start conversion                                */
    /*  - external trigger polarity                                           */
    /*  - channel set to rank 1 (scan mode disabled, only rank 1 can be used) */
    
    if (sConfigInjected->InjectedRank == ADC_INJECTED_RANK_1)
    {
      /* Enable external trigger if trigger selection is different of         */
      /* software start.                                                      */
      /* Note: This configuration keeps the hardware feature of parameter     */
      /*       ExternalTrigInjecConvEdge "trigger edge none" equivalent to    */
      /*       software start.                                                */
      if (sConfigInjected->ExternalTrigInjecConv != ADC_INJECTED_SOFTWARE_START)
      {
         tmp_JSQR_ContextQueueBeingBuilt = (  ADC_JSQR_RK(sConfigInjected->InjectedChannel, ADC_INJECTED_RANK_1)
                                            | (sConfigInjected->ExternalTrigInjecConv & ADC_JSQR_JEXTSEL)
                                            | sConfigInjected->ExternalTrigInjecConvEdge
                                           );
      }
      else
      {
         tmp_JSQR_ContextQueueBeingBuilt = ( ADC_JSQR_RK(sConfigInjected->InjectedChannel, ADC_INJECTED_RANK_1) );
      }
      
      MODIFY_REG(hadc->Instance->JSQR, ADC_JSQR_FIELDS, tmp_JSQR_ContextQueueBeingBuilt); 
      /* For debug and informative reasons, hadc handle saves JSQR setting */
      hadc->InjectionConfig.ContextQueue = tmp_JSQR_ContextQueueBeingBuilt;
    
    }
  }
  else
  {
    /* Case of scan mode enabled, several channels to set into injected group */
    /* sequencer.                                                             */
    /*                                                                        */
    /* Procedure to define injected context register JSQR over successive     */
    /* calls of this function, for each injected channel rank:                */
    /* 1. Start new context and set parameters related to all injected        */
    /*    channels: injected sequence length and trigger.                     */
    
    /* if hadc->InjectionConfig.ChannelCount is equal to 0, this is the first */
    /*   call of the context under setting                                    */
    if (hadc->InjectionConfig.ChannelCount == 0U) 
    {
      /* Initialize number of channels that will be configured on the context */
      /*  being built                                                         */
      hadc->InjectionConfig.ChannelCount = sConfigInjected->InjectedNbrOfConversion;
      /* Handle hadc saves the context under build up over each HAL_ADCEx_InjectedConfigChannel()
         call, this context will be written in JSQR register at the last call.
         At this point, the context is merely reset  */
       hadc->InjectionConfig.ContextQueue = 0x00000000U;
      
      /* Configuration of context register JSQR:                              */
      /*  - number of ranks in injected group sequencer                       */
      /*  - external trigger to start conversion                              */
      /*  - external trigger polarity                                         */
      
      /* Enable external trigger if trigger selection is different of         */
      /* software start.                                                      */
      /* Note: This configuration keeps the hardware feature of parameter     */
      /*       ExternalTrigInjecConvEdge "trigger edge none" equivalent to    */
      /*       software start.                                                */
      if (sConfigInjected->ExternalTrigInjecConv != ADC_INJECTED_SOFTWARE_START)
      {
         tmp_JSQR_ContextQueueBeingBuilt = (  (sConfigInjected->InjectedNbrOfConversion - 1U)
                                            | (sConfigInjected->ExternalTrigInjecConv & ADC_JSQR_JEXTSEL)
                                            | sConfigInjected->ExternalTrigInjecConvEdge
                                           );
      }
      else
      {
        tmp_JSQR_ContextQueueBeingBuilt = ((sConfigInjected->InjectedNbrOfConversion - 1U) );
      }
      
    }
    
    /* 2. Continue setting of context under definition with parameter       */
    /*    related to each channel: channel rank sequence                    */
    /* Clear the old JSQx bits for the selected rank */
    tmp_JSQR_ContextQueueBeingBuilt &= ~ADC_JSQR_RK(ADC_SQR3_SQ10, sConfigInjected->InjectedRank);
    
    /* Set the JSQx bits for the selected rank */
    tmp_JSQR_ContextQueueBeingBuilt |= ADC_JSQR_RK(sConfigInjected->InjectedChannel, sConfigInjected->InjectedRank);
    
    /* Decrease channel count  */
    hadc->InjectionConfig.ChannelCount--;
    
    /* 3. tmp_JSQR_ContextQueueBeingBuilt is fully built for this HAL_ADCEx_InjectedConfigChannel()
          call, aggregate the setting to those already built during the previous  
          HAL_ADCEx_InjectedConfigChannel() calls (for the same context of course)  */
    hadc->InjectionConfig.ContextQueue |= tmp_JSQR_ContextQueueBeingBuilt;
    
    /* 4. End of context setting: if this is the last channel set, then write context
        into register JSQR and make it enter into queue                   */
    if (hadc->InjectionConfig.ChannelCount == 0U)
    {
      MODIFY_REG(hadc->Instance->JSQR, ADC_JSQR_FIELDS, hadc->InjectionConfig.ContextQueue); 
    }
  }

  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on injected group:                                   */
  /*  - Injected context queue: Queue disable (active context is kept) or     */
  /*    enable (context decremented, up to 2 contexts queued)                 */
  /*  - Injected discontinuous mode: can be enabled only if auto-injected     */
  /*    mode is disabled.                                                     */
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
  {
    /* If auto-injected mode is disabled: no constraint                       */
    if (sConfigInjected->AutoInjectedConv == DISABLE)
    {                               
      MODIFY_REG(hadc->Instance->CFGR, ADC_CFGR_JQM | ADC_CFGR_JDISCEN, 
                               ADC_CFGR_INJECT_CONTEXT_QUEUE(sConfigInjected->QueueInjectedContext)          | 
                               ADC_CFGR_INJECT_DISCCONTINUOUS(sConfigInjected->InjectedDiscontinuousConvMode) );                          
    }
    /* If auto-injected mode is enabled: Injected discontinuous setting is    */
    /* discarded.                                                             */
    else
    {
            MODIFY_REG(hadc->Instance->CFGR, ADC_CFGR_JQM | ADC_CFGR_JDISCEN, 
                               ADC_CFGR_INJECT_CONTEXT_QUEUE(sConfigInjected->QueueInjectedContext) );  
    }

  }
  
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular and injected groups:                      */
  /*  - Automatic injected conversion: can be enabled if injected group       */
  /*    external triggers are disabled.                                       */
  /*  - Channel sampling time                                                 */
  /*  - Channel offset                                                        */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {
    /* If injected group external triggers are disabled (set to injected      */
    /* software start): no constraint                                         */
    if ((sConfigInjected->ExternalTrigInjecConv == ADC_INJECTED_SOFTWARE_START)
       || (sConfigInjected->ExternalTrigInjecConvEdge == ADC_EXTERNALTRIGINJECCONV_EDGE_NONE))
    {
         if (sConfigInjected->AutoInjectedConv == ENABLE)
         {
           SET_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO);    
         }
         else
         {
           CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO);    
         }                                                                                  
    }
    /* If Automatic injected conversion was intended to be set and could not  */
    /* due to injected group external triggers enabled, error is reported.    */
    else
    {
      if (sConfigInjected->AutoInjectedConv == ENABLE)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
        
        tmp_hal_status = HAL_ERROR;
      }
      else
      {
        CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO); 
      }
    }
    
    if (sConfigInjected->InjecOversamplingMode == ENABLE)
    {
      assert_param(IS_ADC_OVERSAMPLING_RATIO(sConfigInjected->InjecOversampling.Ratio));
      assert_param(IS_ADC_RIGHT_BIT_SHIFT(sConfigInjected->InjecOversampling.RightBitShift));
      
      /*  JOVSE must be reset in case of triggered regular mode  */
      assert_param(!(READ_BIT(hadc->Instance->CFGR2, ADC_CFGR2_ROVSE|ADC_CFGR2_TROVS) == (ADC_CFGR2_ROVSE|ADC_CFGR2_TROVS)));
      
      /* Configuration of Injected Oversampler:                                 */
      /*  - Oversampling Ratio                                                  */
      /*  - Right bit shift                                                     */
      
      /* Enable OverSampling mode */
       MODIFY_REG(hadc->Instance->CFGR2,
                  ADC_CFGR2_JOVSE |
                  ADC_CFGR2_OVSR  |
                  ADC_CFGR2_OVSS,
                  ADC_CFGR2_JOVSE                                  | 
                  sConfigInjected->InjecOversampling.Ratio         |
                  sConfigInjected->InjecOversampling.RightBitShift
                 );
    }
    else
    {
      /* Disable Regular OverSampling */
       CLEAR_BIT( hadc->Instance->CFGR2, ADC_CFGR2_JOVSE);
    }
    
#if defined(ADC_SMPR1_SMPPLUS)
      /* Manage specific case of sampling time 3.5 cycles replacing 2.5 cyles */
      if(sConfigInjected->InjectedSamplingTime == ADC_SAMPLETIME_3CYCLES_5)
      {
        /* Set sampling time of the selected ADC channel */
        LL_ADC_SetChannelSamplingTime(hadc->Instance, sConfigInjected->InjectedChannel, LL_ADC_SAMPLINGTIME_2CYCLES_5);
        
        /* Set ADC sampling time common configuration */
        LL_ADC_SetSamplingTimeCommonConfig(hadc->Instance, LL_ADC_SAMPLINGTIME_COMMON_3C5_REPL_2C5);
      }
      else
      {
        /* Set sampling time of the selected ADC channel */
        LL_ADC_SetChannelSamplingTime(hadc->Instance, sConfigInjected->InjectedChannel, sConfigInjected->InjectedSamplingTime);
        
        /* Set ADC sampling time common configuration */
        LL_ADC_SetSamplingTimeCommonConfig(hadc->Instance, LL_ADC_SAMPLINGTIME_COMMON_DEFAULT);
      }
#else
      /* Set sampling time of the selected ADC channel */
      LL_ADC_SetChannelSamplingTime(hadc->Instance, sConfigInjected->InjectedChannel, sConfigInjected->InjectedSamplingTime);
#endif
    
    /* Configure the offset: offset enable/disable, channel, offset value */
    
    /* Shift the offset with respect to the selected ADC resolution. */
    /* Offset has to be left-aligned on bit 11, the LSB (right bits) are set to 0 */
    tmpOffsetShifted = ADC_OFFSET_SHIFT_RESOLUTION(hadc, sConfigInjected->InjectedOffset);
    
    if(sConfigInjected->InjectedOffsetNumber != ADC_OFFSET_NONE)
    {
      /* Set ADC selected offset number */
      LL_ADC_SetOffset(hadc->Instance, sConfigInjected->InjectedOffsetNumber, sConfigInjected->InjectedChannel, tmpOffsetShifted);
       
    }
    else
    {
      /* Scan each offset register to check if the selected channel is targeted. */
      /* If this is the case, the corresponding offset number is disabled.       */
      if(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_GetOffsetChannel(hadc->Instance, LL_ADC_OFFSET_1)) == __LL_ADC_CHANNEL_TO_DECIMAL_NB(sConfigInjected->InjectedChannel))
      {
       LL_ADC_SetOffsetState(hadc->Instance, LL_ADC_OFFSET_1, LL_ADC_OFFSET_DISABLE);
      }
      if(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_GetOffsetChannel(hadc->Instance, LL_ADC_OFFSET_2)) == __LL_ADC_CHANNEL_TO_DECIMAL_NB(sConfigInjected->InjectedChannel))
      {
       LL_ADC_SetOffsetState(hadc->Instance, LL_ADC_OFFSET_2, LL_ADC_OFFSET_DISABLE);
      }
      if(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_GetOffsetChannel(hadc->Instance, LL_ADC_OFFSET_3)) == __LL_ADC_CHANNEL_TO_DECIMAL_NB(sConfigInjected->InjectedChannel))
      {
       LL_ADC_SetOffsetState(hadc->Instance, LL_ADC_OFFSET_3, LL_ADC_OFFSET_DISABLE);
      }
      if(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_GetOffsetChannel(hadc->Instance, LL_ADC_OFFSET_4)) == __LL_ADC_CHANNEL_TO_DECIMAL_NB(sConfigInjected->InjectedChannel))
      {
       LL_ADC_SetOffsetState(hadc->Instance, LL_ADC_OFFSET_4, LL_ADC_OFFSET_DISABLE);
      }
    }
  
  }
  
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated only when ADC is disabled:                */
  /*  - Single or differential mode                                           */
  /*  - Internal measurement channels: Vbat/VrefInt/TempSensor                */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Set mode single-ended or differential input of the selected ADC channel */
    LL_ADC_SetChannelSingleDiff(hadc->Instance, sConfigInjected->InjectedChannel, sConfigInjected->InjectedSingleDiff);
    
    /* Configuration of differential mode */
    if (sConfigInjected->InjectedSingleDiff == ADC_DIFFERENTIAL_ENDED)
    {
      /* Set sampling time of the selected ADC channel */
      LL_ADC_SetChannelSamplingTime(hadc->Instance, __LL_ADC_DECIMAL_NB_TO_CHANNEL(__LL_ADC_CHANNEL_TO_DECIMAL_NB(sConfigInjected->InjectedChannel) + 1), sConfigInjected->InjectedSamplingTime);
    }
    
    /* Management of internal measurement channels: Vbat/VrefInt/TempSensor   */
    /* internal measurement paths enable: If internal channel selected,       */
    /* enable dedicated internal buffers and path.                            */
    /* Note: these internal measurement paths can be disabled using           */
    /* HAL_ADC_DeInit().                                                      */

    /* Configuration of common ADC parameters                                 */
    /* If the requested internal measurement path has already been enabled,   */
    /* bypass the configuration processing.                                   */
      if (( (sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR) &&
            ((LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)) & LL_ADC_PATH_INTERNAL_TEMPSENSOR) == 0U)) ||
          ( (sConfigInjected->InjectedChannel == ADC_CHANNEL_VBAT)       &&
            ((LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)) & LL_ADC_PATH_INTERNAL_VBAT) == 0U))      ||
          ( (sConfigInjected->InjectedChannel == ADC_CHANNEL_VREFINT)    &&
            ((LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)) & LL_ADC_PATH_INTERNAL_VREFINT) == 0U))
         )
    {
      /* Configuration of common ADC parameters (continuation)                */
      /* Software is allowed to change common parameters only when all ADCs   */
      /* of the common group are disabled.                                    */
      if ((ADC_IS_ENABLE(hadc) == RESET)   &&
         (ADC_ANY_OTHER_ENABLED(hadc) == RESET) )              
      {
        if (sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR)
        {
          if (ADC_TEMPERATURE_SENSOR_INSTANCE(hadc)) 
          {
            LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance), LL_ADC_PATH_INTERNAL_TEMPSENSOR | LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)));
          
            /* Delay for temperature sensor stabilization time */
            /* Compute number of CPU cycles to wait for */
            wait_loop_index = (LL_ADC_DELAY_TEMPSENSOR_STAB_US * (SystemCoreClock / 1000000));
            while(wait_loop_index != 0)
            {
              wait_loop_index--;
            }
          }
        }
        else if (sConfigInjected->InjectedChannel == ADC_CHANNEL_VBAT)
        { 
          if (ADC_BATTERY_VOLTAGE_INSTANCE(hadc))
          {
            LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance), LL_ADC_PATH_INTERNAL_VBAT | LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)));
          }
        }
        else if (sConfigInjected->InjectedChannel == ADC_CHANNEL_VREFINT)
        { 
          if (ADC_VREFINT_INSTANCE(hadc))
          {
            LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance), LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(hadc->Instance)));
          }
        }
      }
      /* If the requested internal measurement path has already been enabled  */
      /* and other ADC of the common group are enabled, internal              */
      /* measurement paths cannot be enabled.                                 */
      else  
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
        
        tmp_hal_status = HAL_ERROR;
      }
    }
    
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

#if defined(ADC_MULTIMODE_SUPPORT)
/**
  * @brief  Enable ADC multimode and configure multimode parameters
  * @note   Possibility to update parameters on the fly:
  *         This function initializes multimode parameters, following  
  *         calls to this function can be used to reconfigure some parameters 
  *         of structure "ADC_MultiModeTypeDef" on the fly, without resetting 
  *         the ADCs.
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure 
  *         "ADC_MultiModeTypeDef".
  * @note   To move back configuration from multimode to single mode, ADC must
  *         be reset (using function HAL_ADC_Init() ).
  * @param hadc Master ADC handle
  * @param multimode Structure of ADC multimode configuration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeConfigChannel(ADC_HandleTypeDef* hadc, ADC_MultiModeTypeDef* multimode)
{
  HAL_StatusTypeDef  tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef  tmphadcSlave;
  
  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_MULTIMODE(multimode->Mode));
  if(multimode->Mode != ADC_MODE_INDEPENDENT)
  {
    assert_param(IS_ADC_DMA_ACCESS_MULTIMODE(multimode->DMAAccessMode));
    assert_param(IS_ADC_SAMPLING_DELAY(multimode->TwoSamplingDelay));
  }
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  ADC_MULTI_SLAVE(hadc, &tmphadcSlave);
  
  if (tmphadcSlave.Instance == NULL)
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    
    /* Process unlocked */
    __HAL_UNLOCK(hadc);
    
    return HAL_ERROR;
  }
  
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Multimode DMA configuration                                           */
  /*  - Multimode DMA mode                                                    */
  if ( (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET) 
    && (ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSlave) == RESET) )
  {
    /* Pointer to the common control register */
    tmpADC_Common = __LL_ADC_COMMON_INSTANCE(hadc->Instance);
    
    /* If multimode is selected, configure all multimode paramaters.          */
    /* Otherwise, reset multimode parameters (can be used in case of          */
    /* transition from multimode to independent mode).                        */
    if(multimode->Mode != ADC_MODE_INDEPENDENT)
    {
      MODIFY_REG(tmpADC_Common->CCR, ADC_CCR_MDMA | ADC_CCR_DMACFG, 
                 multimode->DMAAccessMode |
                 ADC_CCR_MULTI_DMACONTREQ(hadc->Init.DMAContinuousRequests));
      
      /* Parameters that can be updated only when ADC is disabled:                */
      /*  - Multimode mode selection                                              */
      /*  - Multimode delay                                                       */
      /*    Note: Delay range depends on selected resolution:                     */
      /*      from 1 to 12 clock cycles for 12 bits                               */
      /*      from 1 to 10 clock cycles for 10 bits,                              */
      /*      from 1 to 8 clock cycles for 8 bits                                 */
      /*      from 1 to 6 clock cycles for 6 bits                                 */
      /*    If a higher delay is selected, it will be clipped to maximum delay    */
      /*    range                                                                 */
      if ((ADC_IS_ENABLE(hadc) == RESET)                             &&
          (ADC_IS_ENABLE(&tmphadcSlave) == RESET)  )
      {
          MODIFY_REG(tmpADC_Common->CCR,
                     ADC_CCR_DUAL |
                     ADC_CCR_DELAY,
                     multimode->Mode |
                     multimode->TwoSamplingDelay
                    );
      }
    }
    else /* ADC_MODE_INDEPENDENT */
    {
      CLEAR_BIT(tmpADC_Common->CCR, ADC_CCR_MDMA | ADC_CCR_DMACFG);
      
      /* Parameters that can be updated only when ADC is disabled:                */
      /*  - Multimode mode selection                                              */
      /*  - Multimode delay                                                       */
      if ((ADC_IS_ENABLE(hadc) == RESET)                             &&
          (ADC_IS_ENABLE(&tmphadcSlave) == RESET)  )
      {
        CLEAR_BIT(tmpADC_Common->CCR, ADC_CCR_DUAL | ADC_CCR_DELAY);
      }
    }
  }
  /* If one of the ADC sharing the same common group is enabled, no update    */
  /* could be done on neither of the multimode structure parameters.          */
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    
    tmp_hal_status = HAL_ERROR;
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
} 
#endif /* ADC_MULTIMODE_SUPPORT */

/**
  * @brief  Enable Injected Queue
  * @note   This function resets CFGR register JQDIS bit in order to enable the
  *         Injected Queue. JQDIS can be written only when ADSTART and JDSTART
  *         are both equal to 0 to ensure that no regular nor injected 
  *         conversion is ongoing. 
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_EnableInjectedQueue(ADC_HandleTypeDef* hadc)
{
  /* Parameter can be set only if no conversion is on-going                   */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {
    CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_JQDIS);
    
    /* Update state, clear previous result related to injected queue overflow */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_JQOVF);
    
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @brief  Disable Injected Queue
  * @note   This function sets CFGR register JQDIS bit in order to disable the
  *         Injected Queue. JQDIS can be written only when ADSTART and JDSTART
  *         are both equal to 0 to ensure that no regular nor injected 
  *         conversion is ongoing. 
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_DisableInjectedQueue(ADC_HandleTypeDef* hadc)
{
  /* Parameter can be set only if no conversion is on-going                   */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {
    SET_BIT(hadc->Instance->CFGR, ADC_CFGR_JQDIS);
    return HAL_OK;    
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @brief  Disable ADC voltage regulator.
  * @note   Disabling voltage regulator allows to save power. This operation can
  *         be carried out only when ADC is disabled.
  * @note   To enable again the voltage regulator, the user is expected to 
  *         resort to HAL_ADC_Init() API.           
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_DisableVoltageRegulator(ADC_HandleTypeDef* hadc)
{
  /* ADVREGEN can be written only when the ADC is disabled  */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    CLEAR_BIT(hadc->Instance->CR, ADC_CR_ADVREGEN);
    return HAL_OK;    
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @brief  Enter ADC deep-power-down mode
  * @note   This mode is achieved in setting DEEPPWD bit and allows to save power 
  *         in reducing leakage currents. It is particularly interesting before 
  *         entering stop modes.
  * @note   Setting DEEPPWD automatically clears ADVREGEN bit and disables the
  *         ADC voltage regulator. This means that this API encompasses
  *         HAL_ADCEx_DisableVoltageRegulator(). Additionally, the internal
  *         calibration is lost.    
  * @note   To exit the ADC deep-power-down mode, the user is expected to 
  *         resort to HAL_ADC_Init() API as well as to relaunch a calibration
  *         with HAL_ADCEx_Calibration_Start() API or to re-apply a previously
  *         saved calibration factor.
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_EnterADCDeepPowerDownMode(ADC_HandleTypeDef* hadc)
{
  /* DEEPPWD can be written only when the ADC is disabled  */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    SET_BIT(hadc->Instance->CR, ADC_CR_DEEPPWD);
    return HAL_OK;    
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_ADC_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
