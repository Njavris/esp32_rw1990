#ifndef __IBTN_H__
#define __IBTN_H__

#include "irq.h"
#include <string.h>

struct ibtn_dev {
	struct irq_dev irqdev;
	int pin;
	uint8_t last_uid[8];
	bool mode_write;
};

void ibtn_setup(struct ibtn_dev *dev);
void ibtn_read_uid(struct ibtn_dev *dev, uint8_t uid[8]);
void ibtn_write_uid(struct ibtn_dev *dev, uint8_t uid[8]);

void ibtn_set_mode(struct ibtn_dev *dev, bool write);

#endif // __IBTN_H__
