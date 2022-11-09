TOPTARGETS := all clean

#SUBDIRS := $(wildcard */.)
SUBDIRS := avbperf vimbaperf

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
		$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
