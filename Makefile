all: g
	$(MAKE) cb
g:
	cmake -S . -B build 
cb:
	cmake --build build 
i:
	cmake --install build
t:
	ctest --test-dir build -j4
rt:
	ctest --test-dir build -j4 --rerun-failed --output-on-failure

format: 
	find src/ -iname *.h -o -iname *.cpp | xargs clang-format -i
