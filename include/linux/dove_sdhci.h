#ifndef __LINUX_DOVE_SDHCI_H
#define __LINUX_DOVE_SDHCI_H

struct sdhci_dove_int_wa {
	int			gpio;
	int			irq;
	int			func_select_bit;
	int			status;
};
#endif

