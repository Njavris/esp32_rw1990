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
	uint8_t uid[8];
	while (1) {
		char ch = (char)0;
		fscanf(stdin, "%c", &ch);
		if (ch == 'w') {
			printf("Writing uid: ");
			for (int i = 0; i < 8; i++)
				printf("0x%02x: ", uid[i]);
			printf("\n");
			ibtn_write_uid(&dev, uid);
			printf("Done\n");
		} else if (ch == 'r') {
			printf("Reading uid\n");
			ibtn_read_uid(&dev, uid);
			for (int i = 0; i < 8; i++)
				printf("0x%02x: ", uid[i]);
			printf("\n");
		}

		fflush(stdin);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
