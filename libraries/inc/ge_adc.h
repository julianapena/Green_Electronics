/**
 * @file ge_adc.h
 * @brief Library to interface with the ADCs on the STM32
 * microcontroller and handle the DMA transfers for storing
 * the results.
 * 
 * @author Ned Danyliw
 * @date 9.2016
 */


#ifndef GE_ADC_H_
#define GE_ADC_H_

#ifdef __cplusplus
 extern "C" {
#endif

//Perform necessary includes
#include "common.h"
#include "stm32f30x_adc.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x.h"
#include "stm32f30x_dma.h"
#include "stm32f30x_tim.h"
#include "ge_pins.h"

// Definitions
/**
 * Enumerate ADC channels mapped to external pins on the STM32
 * Discovery
 */
// Number of available ADC channels
#define GE_NUM_ADC 39

// Enumerated type for available ADC channels.
typedef enum ADC_CHAN
{ 
  GE_ADC1_1, GE_ADC1_2, GE_ADC1_3, GE_ADC1_4, GE_ADC1_5,
  GE_ADC2_1, GE_ADC2_2, GE_ADC2_3, GE_ADC2_4, GE_ADC2_5,
  GE_ADC12_6, GE_ADC12_7, GE_ADC12_8, GE_ADC12_9, GE_ADC12_10,
  GE_ADC2_11, GE_ADC2_12, 
  GE_ADC3_1, GE_ADC3_2, GE_ADC3_3, GE_ADC3_5, GE_ADC3_12, GE_ADC3_13, GE_ADC3_14,
  GE_ADC3_15, GE_ADC3_16,
  GE_ADC34_6, GE_ADC34_7, GE_ADC34_8, GE_ADC34_9, GE_ADC34_10, GE_ADC34_11,
  GE_ADC4_1, GE_ADC4_2, GE_ADC4_3, GE_ADC4_4, GE_ADC4_5, GE_ADC4_12, GE_ADC4_13 
} ADC_CHAN_Type;


// GE breakout board pins
// A1 - instrumentation amp (default gain = 5) - PA1
#define GE_A1           GE_ADC1_2
// A2 - instrumentation amp (defualt gain = 5) - PA2
#define GE_A2           GE_ADC1_3
// A3 - PA3
#define GE_A3           GE_ADC1_4
// A4 - PA4
#define GE_A4           GE_ADC2_1


// physical pin mapping of ADC channels
extern GPIOPin adc_pin_map[GE_NUM_ADC];
extern ADC_TypeDef *adc_bank_map[GE_NUM_ADC];
extern uint8_t adc_chan_map[GE_NUM_ADC];

// structure to keep track of ADC channel parameters
typedef struct
{
  GPIOPin pin;
  ADC_CHAN_Type chan;
  ADC_TypeDef *STM_ADCx;
  uint8_t STM_ADC_chan;
  bool enabled;
  uint16_t period;
} GE_ADC_chan_info;

// Array that keeps track of the conversion order
GE_ADC_chan_info adc_conv_order[GE_NUM_ADC];

// Array to hold ADC conversion results
volatile unsigned short adc_readings[GE_NUM_ADC];

// conversion order
int chan_order1[16];
int chan_order2[16];
int chan_order3[16];
int chan_order4[16];

// number of channels per ADC
uint8_t num_chan_adc1;
uint8_t num_chan_adc2;
uint8_t num_chan_adc3;
uint8_t num_chan_adc4;

// registered callback
void (*adc_reg_callback)(uint16_t *);


// ADC Methods
// initialize ADC settings
void adc_init(void);
void adc_deinit(void); //resets all ADC settings

// startup ADCs
void adc_start(void);
void adc_stop(void);

// enable channels for conversion
void adc_enable_channels(ADC_CHAN_Type *channels, uint16_t num_conv);
void adc_initialize_channels();

// set sampling frequency
void adc_set_fs(float fs);

// attach callback function to be called when ADC finishes conversion
void adc_callback(void (*callback)(uint16_t *));

// get ADC state
uint8_t adc_get_state();

#ifdef __cplusplus
}
#endif

#endif /* GE_ADC_H_ */
