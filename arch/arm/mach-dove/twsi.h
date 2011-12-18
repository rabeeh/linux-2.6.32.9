
typedef enum {
	DOVE_TWSI_OPTION1 = 0,
	DOVE_TWSI_OPTION2 = 1,
	DOVE_TWSI_OPTION3 = 2,
} dove_twsi_option;

int dove_select_exp_port(unsigned int port_id);
void dove_reset_exp_port(void);
