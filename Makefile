
all:
	make -C src

test:
	cd tests; lua run-all-tests.lua
