TARGET=retrogame

BUILDTIME	:= $(shell date +%s)

CHAINPREFIX := /opt/mipsel-linux-uclibc
CROSS_COMPILE := $(CHAINPREFIX)/usr/bin/mipsel-linux-

CC			:= $(CROSS_COMPILE)gcc
CXX			:= $(CROSS_COMPILE)g++
STRIP		:= $(CROSS_COMPILE)strip

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

CFLAGS = $(SDL_CFLAGS) -ggdb -DTARGET_RETROGAME -DHW_UDC -DHW_EXT_SD -DHW_SCALER -DOPK_SUPPORT -DIPK_SUPPORT -DTARGET=$(TARGET) -D__BUILDTIME__="$(BUILDTIME)" -DLOG_LEVEL=3 -g3 -mhard-float -mips32 -mno-mips16 # -I$(CHAINPREFIX)/usr/include/ -I$(SYSROOT)/usr/include/  -I$(SYSROOT)/usr/include/SDL/
CFLAGS += -std=c++11 -fdata-sections -ffunction-sections -fno-exceptions -fno-math-errno -fno-threadsafe-statics -Os -Wno-narrowing
# CXXFLAGS = $(CFLAGS)
LDFLAGS = $(SDL_LIBS) -lSDL_image -lSDL_ttf # -lSDL -lSDL_gfx  -lfreetype
LDFLAGS +=-Wl,--as-needed -Wl,--gc-sections

OBJDIR = /tmp/gmenu2x/$(TARGET)
DISTDIR = dist/$(TARGET)/root/home/retrofw/apps/gmenu2x
APPNAME = dist/$(TARGET)/gmenu2x

SOURCES := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp, $(OBJDIR)/src/%.o, $(SOURCES))

# File types rules
$(OBJDIR)/src/%.o: src/%.cpp src/%.h src/hw/retrogame.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

-include $(patsubst src/%.cpp, $(OBJDIR)/src/%.d, $(SOURCES))

# $(OBJDIR)/src/%.d: src/%.cpp
# 	echo OBJDIR2
# 	@if [ ! -d $(OBJDIR)/src ]; then mkdir -p $(OBJDIR)/src; fi
# 	$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
# 	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
# 	rm -f $@.$$$$

all: dir shared

dir:
	@mkdir -p $(OBJDIR)/src dist/$(TARGET)

debug: $(OBJS)
	@echo "Linking gmenu2x-debug..."
	$(CXX) -o $(APPNAME)-debug $(LDFLAGS) $(OBJS)

shared: debug
	$(STRIP) $(APPNAME)-debug -o $(APPNAME)

clean:
	rm -rf $(OBJDIR) *.gcda *.gcno $(APPNAME) /tmp/.gmenu-ipk/ dist/$(TARGET)/root/home/retrofw/apps/gmenu2x/

dist: dir shared
	install -m755 -D $(APPNAME) $(DISTDIR)/gmenu2x
	install -m644 assets/$(TARGET)/input.conf $(DISTDIR)
	install -m755 -d $(DISTDIR)/sections
	install -m644 -D README.md $(DISTDIR)/README.txt
	# install -m644 -D COPYING $(DISTDIR)/COPYING
	install -m644 -D ChangeLog.md $(DISTDIR)/ChangeLog
	install -m644 -D about.txt $(DISTDIR)/about.txt
	cp -RH assets/skins assets/translations $(DISTDIR)
	cp -RH assets/$(TARGET)/BlackJeans.png assets/$(TARGET)/RetroFW.png assets/$(TARGET)/planet.jpg $(DISTDIR)/skins/Default/wallpapers
	touch $(DISTDIR)/skins/Default/skin.conf $(DISTDIR)/gmenu2x.conf
	# cp -RH assets/$(TARGET)/skin.conf $(DISTDIR)/skins/Default
	# cp -RH assets/$(TARGET)/gmenu2x.conf $(DISTDIR)
# 	cp -RH assets/$(TARGET)/font.ttf $(DISTDIR)/skins/Default

ipk: dist
	rm -rf /tmp/.gmenu-ipk/; mkdir -p /tmp/.gmenu-ipk/
	sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" assets/control > /tmp/.gmenu-ipk/control
	cp assets/conffiles /tmp/.gmenu-ipk/
	echo -e "#!/bin/sh\nsync; echo -e 'Installing gmenunx..'; mount -o remount,rw /; rm /var/lib/opkg/info/gmenunx.list; exit 0" > /tmp/.gmenu-ipk/preinst
	echo -e "#!/bin/sh\nsync; mount -o remount,ro /; echo -e 'Installation finished.\nRestarting gmenunx..'; sleep 1; killall gmenu2x; exit 0" > /tmp/.gmenu-ipk/postinst
	chmod +x /tmp/.gmenu-ipk/postinst /tmp/.gmenu-ipk/preinst
	tar --owner=0 --group=0 -czvf /tmp/.gmenu-ipk/control.tar.gz -C /tmp/.gmenu-ipk/ control conffiles postinst preinst
	tar --owner=0 --group=0 -czvf /tmp/.gmenu-ipk/data.tar.gz -C dist/$(TARGET)/root/ .
	echo 2.0 > /tmp/.gmenu-ipk/debian-binary
	ar r dist/$(TARGET)/gmenunx.ipk /tmp/.gmenu-ipk/control.tar.gz /tmp/.gmenu-ipk/data.tar.gz /tmp/.gmenu-ipk/debian-binary

zip: dist
	cd $(DISTDIR)/.. && zip -FSr ../../../../gmenunx.zip gmenu2x