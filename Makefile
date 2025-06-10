all:
	@echo "..."

clean:
	@echo "Cleaning build directories..."
	rm -rf build_switch
	@echo "Clean complete."

build-switch:
	@echo "Building switch DEKO 3D..."
	cmake -B build_switch -DPLATFORM_SWITCH=ON -DUSE_DEKO3D=ON
	make -C build_switch pkDex.nro -j2
	@echo "Switch build complete."