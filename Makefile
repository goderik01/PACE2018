BUILD=debug

CC=g++
# CC=clang
# CFLAGS=-std=c++14 -pedantic -Wall -Wextra -Wno-unused-result -O3 -march=native -ggdb
cflags.common=-std=c++14 -O3 -Wall -Wextra -Wno-unused-result -Wfatal-errors
cflags.debug=-march=native -ggdb
cflags.release=-march=native -DNDEBUG

CFLAGS=${cflags.common} ${cflags.${BUILD}}
LDFLAGS=
INC=-I./include_override -I./include/boost_1_66_0/ -I./include/paal/include/

MAIN=star_contractions_test
 

HEADERS=$(wildcard src/*.hpp)


# clear implicit rules
.SUFFIXES: 

default: bin/test bin/star_contractions_test


bin/%: src/%.cpp $(HEADERS) | bin
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) -o $@ $<

bin:
	mkdir -p bin

.PHONY: package
package: pkg/cuib.tgz

pkg/cuib.tgz: bin/$(MAIN) CMakeLists.txt | pkg/staging
	cp --parents $$($(CC) $(CFLAGS) $(INC) -H -w src/$(MAIN).cpp 2>&1 | sed 's/\.* //' | grep -E '(src)|(^\.)') pkg/staging 2>/dev/null
	cp src/$(MAIN).cpp pkg/staging/src/
	cp CMakeLists.txt pkg/staging
	cd pkg/staging && \
	tar czf ../cuib.tgz *


pkg/staging:
	mkdir -p pkg/staging


.PHONY: boost
boost:
	mkdir -p include && \
	cd include && \
	wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz && \
	tar -xvzf boost_1_66_0.tar.gz

.PHONY: paal
paal:
	mkdir -p include && \
	cd include && \
	wget http://paal.mimuw.edu.pl/paal.zip && \
	unzip paal.zip  && \
	rm paal.zip && \
	mv home/paal/sources/paal/ paal/ && \
	rm -r home/

.PHONY: clean
clean:
	rm -rf bin/
	rm -rf pkg/


data:
	@if [ ! -d directory ]; then\
		wget http://www.lamsade.dauphine.fr/~sikora/pace18/heuristic.zip ;\
		unzip heuristic.zip ;\
		rm heuristic.zip ;\
		mv public/ data/ ;\
	fi

dot:
	mkdir dot;

pdf:
	mkdir pdf;

todot: data dot
	( cd data && for filename in *.gr; do \
		echo $$filename; \
		python3 ../todot.py $$filename > ../dot/$$filename ; \
	done)

topdf: data dot pdf
	( cd dot && for filename in *.gr; do \
		echo $$filename; \
		dot -Tps $$filename -o ../pdf/$$filename.ps; \
	done)

test_zero: data
	( cd data && for filename in *.gr; do \
		echo $$filename; \
		cat $$filename|grep " 0"|wc; \
	done)

