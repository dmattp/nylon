
GLIBCFLAGS=`pkg-config --cflags glib-2.0` 
GLIBLDFLAGS=`pkg-config --libs glib-2.0` 

TARGETSYS=linux-gtk

NDIR_OUT=../nylon

NYLON_TARGETS=$(NDIR_OUT)/syscore.so

all:  $(NYLON_TARGETS)

$(NDIR_OUT)/syscore.so: NylonSysCore.cpp mwsr-cbq.h $(TARGETSYS)/sys/nylon-sysport-eventq.h $(TARGETSYS)/sys/nylon-sysport-timer.h 
	g++ -DNYLON_TARGETSYS=$(TARGETSYS) -DNYLON_SYSCORE_GLIB -rdynamic -ggdb -fPIC --std=c++0x -shared -o $@ -I $(TARGETSYS) $(GLIBCFLAGS) $< -l luabind -llua5.1 -lrt $(GLIBLDFLAGS)

clean:
	rm -f $(NYLON_TARGETS)
