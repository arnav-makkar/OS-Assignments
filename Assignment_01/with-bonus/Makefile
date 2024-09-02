#invoke make inside following directories and in this order: loader, launch, fib
LOADER_DIR := loader
LAUNCH_DIR := launch
TEST_DIR := test

loader:
	$(MAKE) -C $(LOADER_DIR)

launch:
	$(MAKE) -C $(LAUNCH_DIR)

fib:
	$(MAKE) -C $(TEST_DIR)

#move the lib_simpleloader.so and launch binaries inside bin directory
SRC_DIR = .
DEST_DIR = ./bin

# List of files to move
FILES = lib_simpleloader.so launch

# Default target
all: move_files

# Rule to move files
move_files: $(FILES)
	@mkdir -p $(DEST_DIR)
	@mv $(addprefix $(SRC_DIR)/, $^) $(DEST_DIR)

# Clean up the destination directory
clean:
	rm -rf $(DEST_DIR)


#Provide the command for cleanup
clean:
	$(MAKE) -C $(LOADER_DIR) clean
	$(MAKE) -C $(LAUNCH_DIR) clean
	$(MAKE) -C $(FIB_DIR) clean