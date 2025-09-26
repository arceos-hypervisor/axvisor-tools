# axvisor-tools Makefile
# This Makefile builds all components of axvisor-tools

# Directories
AXCLI_DIR = axcli
JAILHOUSE_DIR = jailhouse-axvisor
OUT_DIR = out

# Default target
all: build

# Create output directory
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Build axcli
build-axcli:
	@echo "Building axcli..."
	cd $(AXCLI_DIR) && cargo build --release

# Build jailhouse-axvisor
build-jailhouse:
	@echo "Building jailhouse-axvisor..."
	cd $(JAILHOUSE_DIR) && sudo make

# Build all components
build: build-axcli build-jailhouse

# Install all components to out directory
install: build $(OUT_DIR)
	@echo "Copying build artifacts to $(OUT_DIR)..."
	cp $(AXCLI_DIR)/target/release/axcli $(OUT_DIR)/
	cp $(JAILHOUSE_DIR)/tools/jailhouse $(OUT_DIR)/
	cp $(JAILHOUSE_DIR)/driver/jailhouse.ko $(OUT_DIR)/
	@echo "Build artifacts copied to $(OUT_DIR)/"

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	cd $(AXCLI_DIR) && cargo clean
	cd $(JAILHOUSE_DIR) && make clean
	rm -rf $(OUT_DIR)

# Clean only axcli
clean-axcli:
	cd $(AXCLI_DIR) && cargo clean

# Clean only jailhouse
clean-jailhouse:
	cd $(JAILHOUSE_DIR) && make clean

# Clean only output directory
clean-out:
	rm -rf $(OUT_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all components (default)"
	@echo "  build        - Build all components"
	@echo "  build-axcli  - Build only axcli"
	@echo "  build-jailhouse - Build only jailhouse-axvisor"
	@echo "  install      - Build and copy all artifacts to out/ directory"
	@echo "  clean        - Clean all build artifacts"
	@echo "  clean-axcli  - Clean only axcli build artifacts"
	@echo "  clean-jailhouse - Clean only jailhouse build artifacts"
	@echo "  clean-out    - Clean only output directory"
	@echo "  help         - Show this help message"

.PHONY: all build build-axcli build-jailhouse install clean clean-axcli clean-jailhouse clean-out help