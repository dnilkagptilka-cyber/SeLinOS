# SPDX-License-Identifier: GPL-2.0
# KDIR must point to the exact pinned Linux 6.18.44 KABI build output.
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
