/** See a brief introduction (right-hand button) */
#include "laser.h"
/* Private include -----------------------------------------------------------*/
#include "stm32f4xx.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static LaserState_t _LLaserState = HIGH;
static LaserState_t _RLaserState = HIGH;

//4.16 wlys添加 Jitter 意为抖动
LaserState_t _LLaserAntiJitterState = HIGH;
LaserState_t _RLaserAntiJitterState = HIGH;


static LaserChangeState_t LLaserChangeState = UnChange;
static LaserChangeState_t RLaserChangeState = UnChange;


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Init laser's pin & interrupt.
  * @param  None
  * @retval None
  */
void Laser_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable External port clocks *****************************************/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);

    /* GPIO configuration *************************************************/
    /* GPIO configured as follows:
          - Pin -> PG11 PG13
          - Input Mode
          - No-Pull
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOG, &GPIO_InitStructure);


    /* EXTI configuration *************************************************/
    /* EXTI configured as follows:
          - Source -> PG13 PG15
          - Interrupt Mode
          - Rising & Falling Interrupt
          - Enable
    */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource13);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource11);

    EXTI_InitStructure.EXTI_Line = EXTI_Line13;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    EXTI_InitStructure.EXTI_Line = EXTI_Line11;
    EXTI_Init(&EXTI_InitStructure);

    /* NVIC configuration ***************************************************/
    /* NVIC configured as follows:
          - Interrupt function name = USART1_IRQHandler
          - pre-emption priority = 0 (Very low)
       ** - subpriority level = 2 (Very low)
          - NVIC_IRQChannel enable
    */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

/**
  * @brief  EXTI_IRQn's interrupt function (F12) EXTI15_10_IRQHandler().
  * @param  None
  * @retval None
  */
void _LaserEdgeTrigger_Interrupt(void)
{
    if(EXTI_GetITStatus(EXTI_Line13) == SET)
    {
        LLaserChangeState = Changed;

        if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == SET)
        {
            _LLaserState = HIGH;
        }
        else
        {
            _LLaserState = LOW;
        }
    }
    EXTI_ClearITPendingBit(EXTI_Line13);


    if(EXTI_GetITStatus(EXTI_Line11) == SET)
    {
        RLaserChangeState = Changed;

        if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_11) == SET)
        {
            _RLaserState = HIGH;
        }
        else
        {
            _RLaserState = LOW;
        }
    }
    EXTI_ClearITPendingBit(EXTI_Line11);

}

/**
  * @brief  Get Left Laser's state.
  * @param  None
  * @retval Laser State. Can be High or Low.
  */
LaserState_t GetLeftLaserState(void)
{
    return _LLaserState;
}


/**
  * @brief  Get Right Laser's state.
  * @param  None
  * @retval Laser State. Can be High or Low.
  */
LaserState_t GetRightLaserState(void)
{
    return _RLaserState;
}

/**
  * @brief  See if the Left Laser State Changed.
  * @param  None
  * @retval Laser Change State. Can be Changed or Unchange.
  */
LaserChangeState_t IsLLaserChange(void)
{
    return LLaserChangeState;
}

/**
  * @brief  See if the Right Laser State Changed.
  * @param  None
  * @retval Laser Change State. Can be Changed or Unchange.
  */
LaserChangeState_t IsRLaserChange(void)
{
    return RLaserChangeState;
}

/**
  * @brief  Clear the Pending Bit of LLaser Change State to Unchange.
  * @param  None
  * @retval None
  */
void ClearLLaserChangePendingBit(void)
{
    LLaserChangeState = UnChange;
}


/**
  * @brief  Clear the Pending Bit of RLaser Change State to Unchange..
  * @param  None
  * @retval None
  */
void ClearRLaserChangePendingBit(void)
{
    RLaserChangeState = UnChange;
}

/* WLYS 4.16日更新 */
/* 此行以上的激光传感器操作、反馈函数都没有带防抖 应该逐步弃用 */
/* 上面某些函数可能最多存在到比赛前10天 具体哪些函数会删除请联系WLYS 反正别用太多就是 */


/*
** 函数名称：GetLLaserStateAntiJitter()
** 函数作用：返回左边激光传感器当前的状态 带干扰抑制
** 函数输入：无
** 函数输出：LaserState_t 类型的枚举 状态为HIGH或者LOW 在绿毯上HIGH 白线上LOW

** 注意事项：由于采用了干扰抑制 所以该函数会比不带干扰抑制的状态读取函数慢一些 最多慢5ms
*/
LaserState_t GetLLaserStateAntiJitter(void)
{
    return _LLaserAntiJitterState;
}


/*
** 函数名称：GetRLaserStateAntiJitter()
** 函数作用：返回右边激光传感器当前的状态 带干扰抑制
** 函数输入：无
** 函数输出：LaserState_t 类型的枚举 状态为HIGH或者LOW 在绿毯上HIGH 白线上LOW

** 注意事项：由于采用了干扰抑制 所以该函数会比不带干扰抑制的状态读取函数慢一些 最多慢5ms
*/
LaserState_t GetRLaserStateAntiJitter(void)
{
    return _RLaserAntiJitterState;
}
