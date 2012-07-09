.PHONY : all clean cleanall

all :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
	cd testing && ./cfgtest

clean cleanall :
	$(MAKE) --no-print-directory -C analyzer $@
	$(MAKE) --no-print-directory -C testing $@
