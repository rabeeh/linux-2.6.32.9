#ifndef MV_EC_KEY_H
#define MV_EC_KEY_H


struct mv_ec_key_platform_data {
	int irq;
	int code; /* input event code (KEY_*) */
	int type; /* input event type (EV_KEY) */
};

#define EC_KEY_DRV_NAME "mv-ec-key"

#endif
