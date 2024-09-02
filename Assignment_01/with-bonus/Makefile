LOADER_DIR := loader
LAUNCHER_DIR := launcher
TEST_DIR := test
all:
	$(MAKE) -C $(LOADER_DIR)
	$(MAKE) -C $(LAUNCHER_DIR)
	$(MAKE) -C $(TEST_DIR)
clean:
	cd bin && rm launch lib_simpleloader.so
	cd loader && rm loader.o
	cd test && rm fib
	
	
#move the lib_simpleloader.so and launch binaries inside bin directory
#Provide the command for cleanup
