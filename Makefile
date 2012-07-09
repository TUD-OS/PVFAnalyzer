.PHONY : all clean cleanall

all clean cleanall :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
