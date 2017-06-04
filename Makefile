# Makefile

CC ?= cc
LD ?= ld
AR ?= ar

C_ASSOC_ARRAY_DIR = cAssocArray
C_ASSOC_ARRAY_URL = https://github.com/Andrew-M-C/cAssocArray.git
PWD = $(shell pwd)

CFLAGS += -I$(PWD)/include -Wall -fPIC -DCFG_LOG_LEVEL=7 -fno-stack-protector
LDFLAGS += -pthread -lrt

# main rules
.PHONY: all
all: $(C_ASSOC_ARRAY_DIR) libamcepoll.a libamcepoll.so test
	@echo '<< make $@ done >>'

###########################
# test process
.PHONY: test
test: test_server

test_server: test_server.o
	$(CC) test_server.o -o $@ $(LDFLAGS) -lamcepoll -lrt -L./ -static
	@echo '<< make $@ done >>'


###########################
# clean
.PHONY: clean
clean:
	-find ./ -name "*.o" | xargs rm -f
	-find ./ -name "*.d" | xargs rm -f
	-rm -f *.a
	-rm -f *.so
	-rm -f test_server
	@echo '<< make $@ done >>'


###########################
# distclean
.PHONY: distclean
distclean: clean clean_$(C_ASSOC_ARRAY_DIR)
	@echo '<< make $@ done >>'

###########################
# cAssocArray
.PHONY: $(C_ASSOC_ARRAY_DIR)
$(C_ASSOC_ARRAY_DIR):
	@if [ -d $(C_ASSOC_ARRAY_DIR)/.git ]; then \
		echo '<< make $@ done >>'; \
	else \
		mkdir $(C_ASSOC_ARRAY_DIR); \
		echo "Now cloning $(C_ASSOC_ARRAY_URL)"; \
		git clone $(C_ASSOC_ARRAY_URL) $(PWD)/$(C_ASSOC_ARRAY_DIR); \
		echo '<< clone $@ done >>'; \
	fi

.PHONY: clean_$(C_ASSOC_ARRAY_DIR)
clean_$(C_ASSOC_ARRAY_DIR):
	-rm -rf $(C_ASSOC_ARRAY_DIR)/
	@echo '<< make $@ done >>'

.PHONY: update
update: clean_$(C_ASSOC_ARRAY_DIR) $(C_ASSOC_ARRAY_DIR)


###########################
# libamcepoll
AMC_EPOLL_OBJS = $(shell find ./src -name "*.c" | sed 's/\.c/\.o/')

libamcepoll.so: $(AMC_EPOLL_OBJS)
	$(CC) $(AMC_EPOLL_OBJS) -o $@ $(LDFLAGS) -shared
	@echo '<< make $@ done >>'

libamcepoll.a: $(AMC_EPOLL_OBJS)
	$(AR) r $@ $(AMC_EPOLL_OBJS)
	@echo '<< make $@ done >>'

###########################
# general rules
# .o files
PWD_C_FILES = ./$(wildcard *.c)
ALL_C_OBJ = $(AMC_EPOLL_OBJS) $(PWD_C_FILES:.c=.o)
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
	@echo '$(PWD_C_FILES)'
	@echo '$(ALL_C_OBJ)'
	@echo '$(AMC_EPOLL_OBJS)'


