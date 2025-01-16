#include <ESP32PWM.h>
#include "ESP32Tone.h"

QueueHandle_t toneQueue = NULL;

void tone(int APin,unsigned int frequency){
	if(frequency == 0) return noTone(APin);
	ESP32PWM* chan = pwmFactory(APin);
	if (chan == NULL) {
		chan = new ESP32PWM();
	}
	if(!chan->attached()){
		chan->attachPin(APin,frequency, 10); // This adds the PWM instance to the factory list
		//Serial.println("Attaching tone : "+String(APin)+" on PWM "+String(chan->getChannel()));
	}
	chan->writeTone(frequency);// update the time base of the PWM
}

void toneQueueReceiver(void* pvParameters) {
	for(;;) {
		int receivedToneParameters[3];
		if(xQueueReceive(toneQueue, receivedToneParameters, 20) == pdPASS) {
			int pin = receivedToneParameters[0];
			int freq = receivedToneParameters[1];
			int duration = receivedToneParameters[2];

			tone(pin, freq);
			vTaskDelay(duration);
			noTone(pin);
		}
	}
}

void tone(int pin, unsigned int frequency, unsigned long duration) {
	int parameters[3] = {
		pin,
		(int)frequency,
		(int)duration
	};

	if(toneQueue == NULL) {
		ESP_LOGI(__FILENAME__, "Creating tone queue & receiver");
		toneQueue = xQueueCreate(32, sizeof(parameters));
		xTaskCreate(
			toneQueueReceiver,
			"Tone Queue Receiver",
			4096,
			NULL,
			configMAX_PRIORITIES-4,
			NULL
		);
	}

	ESP_LOGI(__FILENAME__, "Queueing tone @ %dhz for %dms on pin %d", (int)frequency, (int)duration, pin);
	if(xQueueSend(toneQueue, parameters, pdMS_TO_TICKS(10000)) != pdPASS) {
		ESP_LOGW(__FILENAME__, "Could not queue tone @ %dhz for %dms on pin %d", (int)frequency, (int)duration, pin);
	}
}

void noTone(int pin){
	ESP32PWM* chan = pwmFactory(pin);
	if (chan != NULL) {
		if(chan->attached())
		{
			chan->detachPin(pin);
			delete chan;
		}
	}
}
