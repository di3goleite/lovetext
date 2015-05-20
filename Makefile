CC ?= gcc

#ICON_PATH_48x48 = /usr/share/icons/hicolor/48x48/apps/lovetext.png
#ICON_PATH_svg   = /usr/share/icons/hicolor/scalable/apps/lovetext.svg

LDLIBS += `pkg-config --libs gtk+-3.0 gtksourceview-3.0 x11 lua`
CFLAGS += `pkg-config --cflags gtk+-3.0 gtksourceview-3.0 x11 lua`
BUILD_NUMBER = `date +%M%H%d%m%Y`

compile:
	@echo "Build number:"
	@echo "$(BUILD_NUMBER)"
	@echo ""
	@sed -i "s/\(#define\s_BUILD_NUMBER_\s\"\)[[:digit:]]\+\(\"\)/\1$(BUILD_NUMBER)\2/g" ./module.h
	$(CC) -Wall -Wshadow -g $(CFLAGS) -o module.o -c module.c
	$(CC) -Wall -Wshadow -g $(CFLAGS) -o main_window_preferences.o -c main_window_preferences.c
	$(CC) -Wall -Wshadow -g $(CFLAGS) -o main_window.o -c main_window.c
	$(CC) -Wall -Wshadow -g $(CFLAGS) -o main.o -c main.c
	@echo ''
	@echo 'Linking files.'
	$(CC) -Wall -Wshadow -o ./lovetext *.o $(LDLIBS)

install:
	install -m755 ./lovetext $(DESTDIR)/usr/bin/lovetext
	make clean

clean:
	@echo 'Cleaning up.'
	rm *.o
