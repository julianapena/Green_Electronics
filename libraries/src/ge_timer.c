/**
 * @file  ge_timer.c
 * @brief Source code for EE155 timer library
 * 
 * @author Ned Danyliw
 * @date  09.2015
 */

#include "ge_timer.h"


// private variables
// keeps track of maximum count before reseting
uint32_t _ge_tim_max_counter;
// keep track of number of used timers
int _ge_tim_num_timers;
// timer counter value
uint32_t _ge_tim_count;
// array of timer periods
uint32_t _ge_tim_periods[_GE_MAX_TIMERS];
// function array for callbacks
void (*_ge_tim_callbacks[_GE_MAX_TIMERS])(void);
// array of timer types (single shot or periodic)
int _ge_tim_type[_GE_MAX_TIMERS];
// array of timer offsets since the callback might
// be started at a non-zero time making the first period shorter
int _ge_tim_offsets[_GE_MAX_TIMERS];
// array of whether the timer is running
bool _ge_tim_state[_GE_MAX_TIMERS];

/**
 * @brief Initialize TIM3 to use for timing interrupts
 * @details Sets up TIM3 to use the appropriate time base and
 * set up the appropriate interrupts.
 * @return 0 on success, -1 on failure
 */
int initialize_timer() {
  _ge_tim_max_counter = 0;
  _ge_tim_num_timers = 0;
  _ge_tim_count = 0;

  //initialize arrays
  for (int i = 0; i < _GE_MAX_TIMERS; i++) {
    _ge_tim_periods[i] = 0;
    _ge_tim_callbacks[i] = NULL;
    _ge_tim_type[i] = GE_PERIODIC;
    _ge_tim_offsets[i] = 0;
    _ge_tim_state[i] = false;
  }

  //enable TIM3
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  //setup TIM3
  TIM_TimeBaseInitTypeDef TIM3_base;
  TIM_TimeBaseStructInit(&TIM3_base);
  TIM3_base.TIM_Period = _GE_TIM_PERIOD - 1;
  TIM3_base.TIM_Prescaler = 0;
  TIM3_base.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM3_base.TIM_CounterMode = TIM_CounterMode_Up;
  TIM3_base.TIM_RepetitionCounter = 0x00;
  TIM_TimeBaseInit(TIM3, &TIM3_base);

  //setup interrupt
  NVIC_InitTypeDef NVIC_init_struct;
  NVIC_init_struct.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_init_struct.NVIC_IRQChannelSubPriority = 0;
  NVIC_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_init_struct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_init_struct);

  //enable timer update interrupt
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  //enable counter
  TIM_Cmd(TIM3, ENABLE);
}


/**
 * @brief Disables TIM3 and removes the associated interrupts.
 * @details Fully disables TIM3 and removes the interrupts. This
 * function will stop all the timer callbacks from functioning
 * and only should be called if you are using TIM3 for something
 * else.
 * @return 0 on success
 */
int deinit_timer() {
  NVIC_DisableIRQ(TIM3_IRQn);
  TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
  TIM_Cmd(TIM3, DISABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
}


/**
 * @brief Registers a callback function to an internal Timer
 * @details Sets up a timer based on the required period. The
 * function will then assign the callback function to the
 * appropriate timer interrupt. The function uses TIM3 to generate
 * the interrupts.
 * 
 * @param ms Period in milliseconds (int)
 * @param function Callback function
 * @param type SINGLE_SHOT or PERIODIC
 * @return Timer ID of associate timer or error code.
 */
timer_id_t register_timer(uint32_t ms, void (*function)(void), uint8_t type) {
  //check if there are less than the max number of timers registered
  if (_ge_tim_num_timers == _GE_MAX_TIMERS)
    return _GE_TIM_ERROR;

  //if there is a free timer spot place in the first open spot
  _ge_tim_num_timers++;
  for (int i = 0; i < _GE_MAX_TIMERS; i++) {
    if (_ge_tim_periods[i] == 0) {
      _ge_tim_periods[i] = ms;
      _ge_tim_callbacks[i] = function;
      _ge_tim_type[i] = type;
      _ge_tim_offsets[i] = _ge_tim_count;
      _ge_tim_state[i] = false;

      return i; //return timer id
    }
  }
}

/**
 * @brief Starts the associated timer
 * @details Starts the associated timer.
 * 
 * @param timer ID of the timer to start
 * @return 0 on success
 */
int start_timer(timer_id_t timer) {
  _ge_tim_state[timer] = true;
  _ge_tim_offsets[timer] = _ge_tim_count;

  return 0;
}


/**
 * @brief Stops the associated timer.
 * @details Stops the associated timer and removes it from
 * the interrupt list. If you want to restart the timer,
 * you will need to reregister the callback.
 * 
 * @param timer ID of the timer to stop and remove
 * @return 0 on success
 */
int stop_timer(timer_id_t timer) {
  _ge_tim_state[timer] = false;
  _ge_tim_offsets[timer] = 0;
  _ge_tim_periods[timer] = 0;
  _ge_tim_callbacks[timer] = NULL;
  _ge_tim_num_timers--;
}


/**
 * @brief Handler for global TIM3 update interrupt
 * @details Interrupt handler for TIM3 update interrupt. This
 * function updates the current interrupt counter and checks
 * whether a timer function should be called.
 */
void TIM3_IRQHandler() {
  if (_ge_tim_num_timers == 0)
    return; //no registered callbacks
  
  //check if should call one of the registered callbacks
  _ge_tim_count++;
  for (int i = 0; i < _GE_MAX_TIMERS; i++) {
    if (_ge_tim_state[i] 
        && _ge_tim_periods[i] != 0 
        && (((int64_t)_ge_tim_count - _ge_tim_offsets[i]) % _ge_tim_periods[i]) == 0
        && ((int64_t)_ge_tim_count - _ge_tim_offsets[i]) != 0) {
      //check if single shot
      if (_ge_tim_type[i] == GE_SINGLESHOT) {
        _ge_tim_state[i] = false;
        _ge_tim_offsets[i] = 0;
        _ge_tim_periods[i] = 0;

        (*_ge_tim_callbacks[i])(); //call appropriate callback

        _ge_tim_callbacks[i] = NULL;
      } else {
        (*_ge_tim_callbacks[i])(); //call appropriate callback        
      }
      
    }
  }
}

