.PHONY : all clean cleanall valgrind

WVTEST = $(shell which wvtestrun)
ifeq ($(WVTEST),)
TESTRUN = ./cfgtest
else
TESTRUN = wvtestrun ./cfgtest
endif

all :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
	cd testing && $(TESTRUN);

clean cleanall :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
	$(RM) common/*.o common/*.d

valgrind :
	cd testing && valgrind --leak-check=full --show-reachable=yes ./cfgtest
