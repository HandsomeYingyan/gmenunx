PLATFORM := linux

BUILDTIME := $(shell date +%s)

CC			:= gcc
CXX			:= g++
STRIP		:= strip

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

CFLAGS = -DPLATFORM=\"$(PLATFORM)\" -D__BUILDTIME__="$(BUILDTIME)" -DLOG_LEVEL=4
CFLAGS += -O0 -ggdb -g -pg $(SDL_CFLAGS)
CFLAGS += -Wundef -Wno-deprecated -Wno-unknown-pragmas -Wno-format -Wno-narrowing
CFLAGS += -Isrc -Isrc/libopk
CFLAGS += -DTARGET_LINUX -DHW_TVOUT -DHW_UDC -DHW_EXT_SD -DHW_SCALER -DOPK_SUPPORT -DIPK_SUPPORT

LDFLAGS = -Wl,-Bstatic -Lsrc/libopk -l:libopk.a
LDFLAGS += -Wl,-Bdynamic -lz $(SDL_LIBS) -lSDL_image -lSDL_ttf

OBJDIR = /tmp/gmenunx/$(PLATFORM)
DISTDIR = dist/$(PLATFORM)
TARGET = dist/$(PLATFORM)/gmenunx

SOURCES := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp, $(OBJDIR)/src/%.o, $(SOURCES))

# File types rules
$(OBJDIR)/src/%.o: src/%.cpp src/%.h src/platform/linux.h
	$(CXX) $(CFLAGS) -o $@ -c $<

-include $(patsubst src/%.cpp, $(OBJDIR)/src/%.d, $(SOURCES))

all: dir libopk shared

dir:
	@mkdir -p $(OBJDIR)/src dist/$(PLATFORM)

libopk:
	make -C src/libopk clean
	make -C src/libopk

debug: $(OBJS)
	@echo "Linking gmenunx-debug..."
	$(CXX) -o $(TARGET)-debug $(OBJS) $(LDFLAGS)

shared: debug
	$(STRIP) $(TARGET)-debug -o $(TARGET)

clean:
	make -C src/libopk clean
	rm -rf $(OBJDIR) $(DISTDIR) *.gcda *.gcno $(TARGET) $(TARGET)-debug

dist: dir libopk shared
	install -m644 -D README.md $(DISTDIR)/README.txt
	install -m644 -D COPYING $(DISTDIR)/COPYING
	install -m644 -D ChangeLog.md $(DISTDIR)/ChangeLog
	cp -RH assets/translations $(DISTDIR)
	cp -RH assets/skins $(DISTDIR)
	cp -RH assets/$(PLATFORM)/input.conf $(DISTDIR)

zip: dist
	cd $(DISTDIR)/ && zip -r ../gmenunx.$(PLATFORM).zip skins translations ChangeLog COPYING gmenunx input.conf README.txt
