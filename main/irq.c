#include "irq.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static QueueHandle_t irq_queue = NULL;

static void IRAM_ATTR irq_handler(void* arg)
{
	struct irq_dev *dev = (struct irq_dev *)arg;
	uint32_t val = 0x69;
	xQueueSendFromISR(irq_queue, &val, NULL);
	irq_dis(dev);
}

static void irq_bottom_half(void* arg)
{
	struct irq_dev *dev = (struct irq_dev *)arg;
	uint32_t val;
	for (;;) {
		if (xQueueReceive(irq_queue, &val, portMAX_DELAY)) {
			if (dev->irq_bh) {
				dev->irq_bh(dev->bh_data);
			}
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void irq_en(struct irq_dev *dev) {
	gpio_config_t conf = {};
	conf.intr_type = dev->edge;
	conf.pull_up_en = GPIO_PULLUP_DISABLE;
	conf.mode = GPIO_MODE_INPUT;
	conf.pin_bit_mask = 1ULL << dev->irq_pin;
	gpio_config(&conf);

	gpio_set_intr_type(dev->irq_pin, dev->edge);
	gpio_isr_handler_add(dev->irq_pin, irq_handler, (void *)dev);
}

void irq_dis(struct irq_dev *dev) {
	gpio_isr_handler_remove(dev->irq_pin);
}

void irq_init(struct irq_dev *dev) {
	irq_queue = xQueueCreate(10, sizeof(uint32_t));

	gpio_install_isr_service(0);

	xTaskCreate(irq_bottom_half, "irq_bh", 4096, dev, 10, NULL);
}
