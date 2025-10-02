#include "ibtn.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

void ibtn_set_mode(struct ibtn_dev *dev, bool write) {
	dev->mode_write = write;
	if (write) {
		printf("Setting mode to write uid: ");
		for (int i = 0; i < 8; i++)
			printf("0x%02x: ", dev->last_uid[i]);
		printf(";\n");
	} else {
		printf("Setting mode to read uids\n");
	}
}

static void ibtn_send_byte(struct ibtn_dev *dev, uint8_t byte) {
	int i;

	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	for (i = 0; i < 8; i++, byte >>= 1) {
		gpio_set_level(dev->pin, 0);
		esp_rom_delay_us(byte & 1 ? 10 : 60);
		gpio_set_level(dev->pin, 1);
		esp_rom_delay_us(byte & 1 ? 60 : 10);
	}
}

static uint8_t ibtn_rcv_byte(struct ibtn_dev *dev) {
	int i;
	uint8_t ret = 0;
	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	for (i = 0; i < 8; i++) {
		gpio_set_level(dev->pin, 0);
		esp_rom_delay_us(10);
		gpio_set_level(dev->pin, 1);
		esp_rom_delay_us(10);

		gpio_set_direction(dev->pin, GPIO_MODE_INPUT);
		if (gpio_get_level(dev->pin))
			ret |= 1 << i;

		esp_rom_delay_us(45);
		gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);

	}
	return ret;
}

static void ibtn_rst(struct ibtn_dev *dev) {
	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	gpio_set_level(dev->pin, 0);
	esp_rom_delay_us(480);
	gpio_set_level(dev->pin, 1);
	esp_rom_delay_us(70);
}

static void ibtn_presence(struct ibtn_dev *dev) {
	gpio_set_direction(dev->pin, GPIO_MODE_INPUT);
	while(gpio_get_level(dev->pin));
	while(!gpio_get_level(dev->pin));
	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	gpio_set_level(dev->pin, 1);
}

void ibtn_read_uid(struct ibtn_dev *dev, uint8_t uid[8]) {
	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	ibtn_rst(dev);
	ibtn_presence(dev);

	ibtn_send_byte(dev, 0x33);

	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	for (int i = 0; i < 8; i++) {
		uid[i] = ibtn_rcv_byte(dev);
	}
}

void ibtn_write_uid(struct ibtn_dev *dev, uint8_t uid[8]) {
	gpio_set_direction(dev->pin, GPIO_MODE_OUTPUT);
	ibtn_rst(dev);
	ibtn_presence(dev);

	ibtn_send_byte(dev, 0xd5);
	gpio_set_level(dev->pin, 1);

	for (int i = 0; i < 8; i++) {
		uint8_t byte = uid[i];
		for (int b = 0; b < 8; b++, byte >>= 1) {
			gpio_set_level(dev->pin, 0);
			esp_rom_delay_us(byte & 1 ? 60 : 2);

			gpio_set_level(dev->pin, 1);

			esp_rom_delay_us(10 * 1000);
		}
	}
	ibtn_rst(dev);
	ibtn_presence(dev);
}

static void ibtn_irq_bh(void *data) {
	struct ibtn_dev *dev = (struct ibtn_dev *)data;

	if (!dev->mode_write) {
		ibtn_read_uid(dev, dev->last_uid);

		printf("Got uid: ");
		for (int i = 0; i < 8; i++)
			printf("0x%02x: ", dev->last_uid[i]);
		printf("\n");
	} else {
		ibtn_write_uid(dev, dev->last_uid);
		printf("Done!\n");
	}

	irq_en(&dev->irqdev);
}

void ibtn_setup(struct ibtn_dev *dev) {
	dev->irqdev.irq_pin = dev->pin;
	dev->irqdev.irq_bh = &ibtn_irq_bh;
	dev->irqdev.edge = GPIO_INTR_NEGEDGE;
	dev->irqdev.bh_data = (void *)dev;
	dev->mode_write = false;

	gpio_config_t conf = {};
	conf.intr_type = GPIO_INTR_DISABLE;
	conf.pull_up_en = GPIO_PULLUP_DISABLE;
	conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	conf.mode = GPIO_MODE_OUTPUT;
	conf.pin_bit_mask = (1ULL << dev->pin);
	gpio_config(&conf);

	irq_init(&dev->irqdev);
	irq_en(&dev->irqdev);
}
