#include <stddef.h>

#include "common/assert.h"
#include "exti.hpp"
#include "gpio/gpio.hpp"
#include "CMSIS/Device/STM32F4xx/Include/stm32f4xx.h"
#include "CMSIS/Include/core_cm4.h"

using namespace hal;

#define IRQ_PRIORITY 3

static IRQn_Type const irq_list[gpio::pins] =
{
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn
};

static exti *obj_list[gpio::pins];

exti::exti(gpio &gpio, trigger_t trigger):
	_gpio(gpio),
	_trigger(trigger),
	_ctx(NULL),
	_cb(NULL)
{
	ASSERT(_trigger <= TRIGGER_BOTH);
	ASSERT(_gpio.mode() == gpio::mode::DI);
	
	/* Enable clock */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
	uint8_t pin = _gpio.pin();
	
	/* Setup EXTI line source */
	uint8_t exticr_source_offset = (pin % 4) * 4;
	SYSCFG->EXTICR[pin / 4] &= ~(SYSCFG_EXTICR1_EXTI0 << exticr_source_offset);
	SYSCFG->EXTICR[pin / 4] |= _gpio.port() << exticr_source_offset;
	
	uint32_t line_bit = 1 << pin;
	/* Setup EXTI mask regs */
	EXTI->EMR &= ~line_bit;
	EXTI->IMR &= ~line_bit;
	
	/* Setup EXTI trigger */
	EXTI->RTSR |= line_bit;
	EXTI->FTSR |= line_bit;
	if(_trigger == TRIGGER_RISING)
		EXTI->FTSR &= ~line_bit;
	else if(_trigger == TRIGGER_FALLING)
		EXTI->RTSR &= ~line_bit;
	
	obj_list[pin] = this;
	
	NVIC_ClearPendingIRQ(irq_list[pin]);
	NVIC_SetPriority(irq_list[pin], IRQ_PRIORITY);
	NVIC_EnableIRQ(irq_list[pin]);
}

exti::~exti()
{
	uint8_t pin = _gpio.pin();
	
	obj_list[pin] = NULL;
	
	EXTI->IMR &= ~(1 << _gpio.pin());
	_cb = NULL;
	uint8_t exticr_source_offset = (pin % 4) * 4;
	SYSCFG->EXTICR[pin / 4] &= ~(SYSCFG_EXTICR1_EXTI0 << exticr_source_offset);
}

void exti::cb(cb_t cb, void *ctx)
{
	_cb = cb;
	_ctx = ctx;
}

void exti::on()
{
	ASSERT(_cb);
	
	uint8_t pin = _gpio.pin();
	uint32_t line_bit = 1 << pin;
	/* Clear EXTI line pending bits */
	EXTI->PR |= line_bit;
	/* Setup EXTI line configuration */
	EXTI->IMR |= line_bit;
	
	NVIC_ClearPendingIRQ(irq_list[pin]);
}

void exti::off()
{
	/* Clear EXTI line configuration */
	EXTI->IMR &= ~(1 << _gpio.pin());
}

void exti::trigger(trigger_t trigger)
{
	ASSERT(trigger <= TRIGGER_BOTH);
	
	_trigger = trigger;
	uint32_t line_bit = 1 << _gpio.pin();
	
	EXTI->RTSR |= line_bit;
	EXTI->FTSR |= line_bit;
	if(_trigger == TRIGGER_RISING)
		EXTI->FTSR &= ~line_bit;
	else if(_trigger == TRIGGER_FALLING)
		EXTI->RTSR &= ~line_bit;
}

extern "C" void exti_irq_hndlr(hal::exti *obj)
{
	/* Clear EXTI line pending bits */
	EXTI->PR = 1 << obj->_gpio.pin();
	
	if(obj->_cb)
		obj->_cb(obj, obj->_ctx);
}

extern "C" void EXTI0_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[0]);
}

extern "C" void EXTI1_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[1]);
}

extern "C" void EXTI2_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[2]);
}

extern "C" void EXTI3_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[3]);
}

extern "C" void EXTI4_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[4]);
}

extern "C" void EXTI9_5_IRQHandler(void)
{
	uint32_t pending_bit = EXTI->PR;
	
	for(uint8_t i = 5; i <= 9; i++)
	{
		if(pending_bit & (1 << i))
		{
			exti_irq_hndlr(obj_list[i]);
			break;
		}
	}
}

extern "C" void EXTI15_10_IRQHandler(void)
{
	uint32_t pending_bit = EXTI->PR;
	
	for(uint8_t i = 10; i <= 15; i++)
	{
		if(pending_bit & (1 << i))
		{
			exti_irq_hndlr(obj_list[i]);
			break;
		}
	}
}
