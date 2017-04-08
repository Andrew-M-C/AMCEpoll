# Makefile

C_ASSOC_ARRAY_DIR = cAssocArray
C_ASSOC_ARRAY_URL = https://github.com/Andrew-M-C/cAssocArray.git
PWD = `pwd`

CFLAGS += -I$(C_ASSOC_ARRAY_DIR)/src


# main rules
.PHONY: all
all: $(C_ASSOC_ARRAY_DIR)
	@echo "Nothing to be done for \"$@\""


.PHONY: clean
clean:
	@echo "Nothing to be done for \"$@\""


# distclean
.PHONY: distclean
distclean: clean
	-rm -rf $(C_ASSOC_ARRAY_DIR)/


# cAssocArray
.PHONY: $(C_ASSOC_ARRAY_DIR)
$(C_ASSOC_ARRAY_DIR):
	@if [ -d $(C_ASSOC_ARRAY_DIR)/.git ]; then \
		echo $(C_ASSOC_ARRAY_DIR)/.git exists; \
	else \
		mkdir $(C_ASSOC_ARRAY_DIR); \
		echo "Now cloning $(C_ASSOC_ARRAY_URL)"; \
		git clone $(C_ASSOC_ARRAY_URL) $(PWD)/$(C_ASSOC_ARRAY_DIR); \
	fi


# .o files
$(C_OBJS): $(C_OBJS:.o=.c)
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) $*.c > $*.d  
	@mv -f $*.d $*.d.tmp  
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d  
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp 


