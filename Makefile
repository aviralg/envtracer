R = R

.PHONY: all build check document test

all: document build check install

build: document
	$(R) CMD build .

check: build
	$(R) CMD check envtracer*tar.gz

clean:
	-rm -f envtracer*tar.gz
	-rm -fr envtracer.Rcheck
	-rm -rf src/*.o src/*.so

document:
	$(R) -e 'devtools::document()'

test:
	$(R) -e 'devtools::test()'

lintr:
	$(R) --slave -e "lintr::lint_package()"

install: clean
	$(R) CMD INSTALL .
