#ifndef __ARCH_DOVE_MPP_H
#define __ARCH_DOVE_MPP_H

enum dove_mpp_type {
	/*
	 * This MPP is unused.
	 */
	MPP_UNUSED,

	/*
	 * This MPP pin is used as a generic GPIO pin.
	 */
	MPP_GPIO,

        /*
         * This MPP is used as a SATA activity LED.
         */
        MPP_SATA_ACT,
        MPP_SATA_PRESENCE,
        /*
         * This MPP is used as a functional pad.
         */
//        MPP_FUNCTIONAL,


	MPP_SPI0,
	MPP_SPI1,
	MPP_LCD0_SPI,
	MPP_SDIO0,
	MPP_SDIO1,
	MPP_CAM,
	MPP_AUDIO1,
	MPP_GPIO_AUDIO1, /* this type is used only for MPP high[25], it means 
			  * that this MPP is GPIO, but the AU1 group is selected
			  */
	MPP_SSP,
	MPP_TWSI,
	MPP_UART1,
	MPP_UART2,
	MPP_UART3,
	MPP_NB_CLOCK,
	MPP_LCD,
	MPP_PMU,
	MPP_MII,
	MPP_PCIE,
	MPP_NFC8_15,
	/*
	 * This indicates end of table .
	 */
	MPP_END,

};

struct dove_mpp_mode {
	int			mpp;
	enum dove_mpp_type	type;
};

void dove_mpp_conf(struct dove_mpp_mode *mode);

#endif
