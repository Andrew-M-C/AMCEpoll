# Makefile

CC ?= cc
LD ?= ld
AR ?= ar

C_ASSOC_ARRAY_DIR = cAssocArray
C_ASSOC_ARRAY_URL = https://github.com/Andrew-M-C/cAssocArray.git
PWD = $(shell pwd)

CFLAGS += -I$(PWD)/include -Wall -fPIC -DCFG_LOG_LEVEL=7
LDFLAGS += -lpthread -lrt

# main rules
.PHONY: all
all: $(C_ASSOC_ARRAY_DIR) libamcepoll.a libamcepoll.so test

###########################
# test process
test: test.o
	$(CC) test.o -o $@ $(LDFLAGS) -lamcepoll -L./ -static


###########################
# clean
.PHONY: clean
clean:
	-find ./ -name "*.o" | xargs rm -f
	-find ./ -name "*.d" | xargs rm -f
	-rm -f *.a
	-rm -f *.so
	-rm -f test


###########################
# distclean
.PHONY: distclean
distclean: clean
	-rm -rf $(C_ASSOC_ARRAY_DIR)/

###########################
# cAssocArray
.PHONY: $(C_ASSOC_ARRAY_DIR)
$(C_ASSOC_ARRAY_DIR):
	@if [ -d $(C_ASSOC_ARRAY_DIR)/.git ]; then \
		echo ""; \
	else \
		mkdir $(C_ASSOC_ARRAY_DIR); \
		echo "Now cloning $(C_ASSOC_ARRAY_URL)"; \
		git clone $(C_ASSOC_ARRAY_URL) $(PWD)/$(C_ASSOC_ARRAY_DIR); \
	fi

###########################
# libamcepoll
AMC_EPOLL_OBJS = $(shell find ./src -name "*.c" | sed 's/\.c/\.o/')

libamcepoll.so: $(AMC_EPOLL_OBJS)
	$(CC) $(AMC_EPOLL_OBJS) -o $@ $(LDFLAGS) -shared

libamcepoll.a: $(AMC_EPOLL_OBJS)
	$(AR) r $@ $(AMC_EPOLL_OBJS)

###########################
# general rules
# .o files

ALL_C_OBJ = $(AMC_EPOLL_OBJS) ./test.o
-include $(ALL_C_OBJ:.o=.d)

$(ALL_C_OBJ): $(ALL_C_OBJ:.o=.c)
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) $*.c > $*.d  
	@mv -f $*.d $*.d.tmp  
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d  
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp 

.PHONY: debug
debug:
	@echo "$(ALL_C_OBJ)"


