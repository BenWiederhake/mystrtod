all: mystrtod

mystrtod: mystrtod.cpp
	i686-linux-gnu-g++-10 --std=c++11 $< -o $@

.PHONY: run
run: mystrtod
	./mystrtod
