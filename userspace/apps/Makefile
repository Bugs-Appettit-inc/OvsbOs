APPS = editor paint fm shell

all:
	@for a in $(APPS); do $(MAKE) -C $$a || exit 1; done

static:
	@for a in $(APPS); do $(MAKE) -C $$a static || exit 1; done

install:
	@for a in $(APPS); do $(MAKE) -C $$a install || exit 1; done

clean:
	@for a in $(APPS); do $(MAKE) -C $$a clean; done

.PHONY: all static install clean
