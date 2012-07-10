.PHONY : all clean cleanall valgrind

all :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
	@if [ -f $(which wvtestrun) ]; then   \
		cd testing && wvtestrun ./cfgtest; \
	else                                   \
		cd testing && ./cfgtest;           \
	fi

clean cleanall :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
	$(RM) common/*.o common/*.d

valgrind :
	cd testing && valgrind --leak-check=full --show-reachable=yes ./cfgtest
