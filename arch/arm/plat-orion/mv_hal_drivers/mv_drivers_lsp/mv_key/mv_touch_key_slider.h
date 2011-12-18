#ifndef MV_TOUCHKEY_H
#define MV_TOUCHKEY_H


struct mv_touch_key_platform_data {
	int irq;
	int code; /* input event code (KEY_*) */
	int type; /* input event type (EV_KEY) */
};

struct mv_touch_slider_platform_data {
	int irq;
	int code; /* input event code (KEY_*) */
	int type; /* input event type (EV_KEY) */
};


#define TOUCH_KEY_DRV_NAME "mv-touch-key"
#define TOUCH_SLIDER_DRV_NAME "mv-touch-slider"

#endif
