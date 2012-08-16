all:
	$(MAKE) -C kernelat-spawner
	$(MAKE) -C kernelat-child

install:
	$(MAKE) -C kernelat-spawner install
	$(MAKE) -C kernelat-child install

clean:
	$(MAKE) -C kernelat-spawner clean
	$(MAKE) -C kernelat-child clean

