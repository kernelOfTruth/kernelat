all:
	$(MAKE) -C kernelat-spawner
	$(MAKE) -C kernelat-child

install:
	$(MAKE) -C kernelat-spawner install
	$(MAKE) -C kernelat-child install

local-install:
	$(MAKE) -C kernelat-spawner local-install
	$(MAKE) -C kernelat-child local-install

clean:
	$(MAKE) -C kernelat-spawner clean
	$(MAKE) -C kernelat-child clean
	$(MAKE) -C kernelat-tester clean

