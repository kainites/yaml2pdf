TARGET = src/myapp
SRC = src/yaml2pdf.c

CC = gcc
CFLAGS = -Ithirdparty/libharu/include -Ithirdparty/libyaml/build/install/include -Isrc -Wall -g
LDFLAGS = -Lthirdparty/libharu/build/src -Lthirdparty/libyaml/build/install/lib -lyaml -lhpdf -Wl,-rpath,thirdparty/libharu/build/src

.PHONY: all libharu clean

all: libharu $(TARGET)

libharu:
	mkdir -p thirdparty/libharu/build
	cd thirdparty/libharu/build && cmake .. \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DPNG_PNG_INCLUDE_DIR=/opt/homebrew/include \
		-DPNG_LIBRARY=/opt/homebrew/lib/libpng.dylib && \
	make

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf thirdparty/libharu/build
	rm -f $(TARGET)
