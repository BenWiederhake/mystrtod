all: mystrtod

mystrtod: mystrtod.cpp
	i686-linux-gnu-g++-10 -Wall -Wextra -pedantic --std=c++17 $< -o $@

.PHONY: run
run: mystrtod
	./mystrtod

.PHONY: clean
clean:
	rm -f mystrtod
