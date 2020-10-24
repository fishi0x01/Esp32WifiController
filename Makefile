CLI_VERSION := 0.13.0
CLI := ./bin/arduino-cli

venv:
	@rm -rf .venv
	@python3 -m venv .venv
	@. .venv/bin/activate && pip3 install -r requirements.txt

deps:
	@$(CLI) core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
	@$(CLI) core install esp32:esp32 --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
	@cp patch/esp32-hal-bt.c ~/.arduino15/packages/esp32/hardware/esp32/1.0.4/cores/esp32/esp32-hal-bt.c

cli:
	@rm -rf bin/
	@mkdir -p bin/
	@curl -sL https://downloads.arduino.cc/arduino-cli/arduino-cli_$(CLI_VERSION)_Linux_64bit.tar.gz | tar -xz -C bin/
	@rm bin/LICENSE.txt

compile:
	@. .venv/bin/activate && $(CLI) compile --fqbn esp32:esp32:esp32 --libraries $(shell pwd) $(shell pwd)/examples/Test --build-properties build.partitions=huge_app,upload.maximum_size=3145728
