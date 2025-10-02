#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ibtn.h"
#include "driver/uart.h"


void app_main(void) {
	struct ibtn_dev dev;
	memset(&dev, 0, sizeof(struct ibtn_dev));
	dev.pin = CONFIG_BUTTON_IO_PIN;
	ibtn_setup(&dev);

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	while (1) {
		char ch = (char)0;
		fscanf(stdin, "%c", &ch);
		if (ch == 'w')
			ibtn_set_mode(&dev, true);
		else if (ch == 'r')
			ibtn_set_mode(&dev, false);
		fflush(stdin);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
