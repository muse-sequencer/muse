# mf/GNUmakefile

depth = ..

STEPMAKE_TEMPLATES = metafont \
		     install \
		     install-out
LOCALSTEPMAKE_TEMPLATES = lilypond

include $(depth)/make/stepmake.make


EXTRA_DIST_FILES += README mf2pt1.mp

# We don't use $(MF_FILES), because there's more .mf cruft here
FETA_MF_FILES = $(call src-wildcard,feta[0-9]*.mf) \
		$(call src-wildcard,feta-braces-[a-z].mf) \
		$(call src-wildcard,feta-alphabet*[0-9].mf) \
		$(call src-wildcard,parmesan[0-9]*.mf)

STAFF_SIZES = 11 13 14 16 18 20 23 26
BRACES = a b c d e f g h i

OTF_FILES = $(STAFF_SIZES:%=$(outdir)/emmentaler-%.otf) \
	    $(outdir)/aybabtu.otf
PE_SCRIPTS = $(STAFF_SIZES:%=$(outdir)/emmentaler-%.pe)
OTF_TABLES = $(STAFF_SIZES:%=$(outdir)/feta%.otf-table) \
	     $(BRACES:%=$(outdir)/feta-braces-%.otf-table)
FETA_FONTS = $(FETA_MF_FILES:.mf=)
SVG_FILES = $(OTF_FILES:%.otf=%.svg)

FC_FIND = $(shell $(FCMATCH) --verbose 'Century Schoolbook L:style=$(1)' \
		  | grep 'file:' \
		  | sed 's/.*"\([^"]*\)".*/\1/g')

NCSB_OTFS = $(addprefix $(outdir)/,CenturySchL-Ital.otf \
				   CenturySchL-BoldItal.otf \
				   CenturySchL-Roma.otf \
				   CenturySchL-Bold.otf)
NCSB_INSTALL_DIR = $(local_lilypond_datadir)/fonts/otf

LOG_FILES = $(FETA_MF_FILES:%.mf=$(outdir)/%.log)
LISP_FILES = $(FETA_MF_FILES:%.mf=$(outdir)/%.lisp)
ENC_FILES = $(FETA_MF_FILES:%.mf=$(outdir)/%.enc)
TFM_FILES = $(FETA_MF_FILES:%.mf=$(outdir)/%.tfm)

$(outdir)/aybabtu.otf-table: $(BRACES:%=$(outdir)/feta-braces-%.otf-table)
	cat $^ > $@

$(outdir)/aybabtu.otf-gtable: $(BRACES:%=$(outdir)/feta-braces-%.otf-gtable)
	echo '(design_size . 20)' > $@


# only for fonts which
#
# 1. are mentioned in font.scm
#
# 2. are not included with teTeX
#

$(outdir)/emmentaler-%.otf \
$(outdir)/emmentaler-%.svg: $(outdir)/emmentaler-%.pe \
			    $(outdir)/feta%.pfb \
			    $(outdir)/feta-alphabet%.pfb \
			    $(outdir)/parmesan%.pfb \
			    $(outdir)/feta%.otf-table \
			    $(outdir)/feta%.otf-gtable
	cd $(outdir) && $(FONTFORGE) -script $(notdir $(basename ,$@).pe)

$(outdir)/aybabtu.otf \
$(outdir)/aybabtu.svg: $(outdir)/aybabtu.pe \
		       $(foreach s,$(BRACES),$(outdir)/feta-braces-$(s).pfb) \
		       $(outdir)/aybabtu.otf-table $(outdir)/aybabtu.otf-gtable
	cd $(outdir) && $(FONTFORGE) -script aybabtu.pe

$(outdir)/%.pfb: $(outdir)/%.log

$(outdir)/%.otf-table: $(outdir)/%.lisp
	cat $< $(if $(findstring brace,$<),,$(subst feta,parmesan,$<)) \
	       $(if $(findstring brace,$<),,$(subst feta,feta-alphabet,$<)) > $@


## ugh -- we want this to prevent failing -j2 compiles.
$(outdir)/feta26.otf-table: $(outdir)/feta26.lisp \
			    $(outdir)/parmesan26.lisp \
			    $(outdir)/feta-alphabet26.lisp
$(outdir)/feta23.otf-table: $(outdir)/feta23.lisp \
			    $(outdir)/parmesan23.lisp \
			    $(outdir)/feta-alphabet23.lisp
$(outdir)/feta20.otf-table: $(outdir)/feta20.lisp \
			    $(outdir)/parmesan20.lisp \
			    $(outdir)/feta-alphabet20.lisp
$(outdir)/feta18.otf-table: $(outdir)/feta18.lisp \
			    $(outdir)/parmesan18.lisp \
			    $(outdir)/feta-alphabet18.lisp
$(outdir)/feta16.otf-table: $(outdir)/feta16.lisp \
			    $(outdir)/parmesan16.lisp \
			    $(outdir)/feta-alphabet16.lisp
$(outdir)/feta14.otf-table: $(outdir)/feta14.lisp \
			    $(outdir)/parmesan14.lisp \
			    $(outdir)/feta-alphabet14.lisp
$(outdir)/feta13.otf-table: $(outdir)/feta13.lisp \
			    $(outdir)/parmesan13.lisp \
			    $(outdir)/feta-alphabet13.lisp
$(outdir)/feta11.otf-table: $(outdir)/feta11.lisp \
			    $(outdir)/parmesan11.lisp \
			    $(outdir)/feta-alphabet11.lisp

$(outdir)/aybabtu.otf: $(outdir)/aybabtu.subfonts \
		       $(outdir)/aybabtu.fontname \
		       $(outdir)/aybabtu.otf-table \
		       $(outdir)/aybabtu.otf-gtable \
		       $(outdir)/aybabtu.pe

$(outdir)/aybabtu.otf \
$(outdir)/aybabtu.svg: $(BRACES:%=$(outdir)/feta-braces-%.pfb)

$(outdir)/aybabtu.fontname:
	echo -n 'aybabtu' > $@
$(outdir)/aybabtu.subfonts:
	echo $(subst .mf,,$(call src-wildcard,feta-braces-[a-z].mf)) > $@

$(PE_SCRIPTS): $(buildscript-dir)/gen-emmentaler-scripts
	$< --dir=$(outdir)

ALL_FONTS = $(FETA_FONTS)
PFB_FILES = $(ALL_FONTS:%=$(outdir)/%.pfb)

# Make tfm files first, log files last,
# so that normally log files aren't made twice
ALL_GEN_FILES = $(LOG_FILES) \
		$(ENC_FILES) \
		$(LISP_FILES) \
		$(OTF_TABLES) \
		$(NCSB_OTFS) \
		$(PFB_FILES) \
		$(OTF_FILES) \
		$(SVG_FILES)

# PRE_INSTALL=$(MAKE) "$(ALL_GEN_FILES)"

INSTALLATION_DIR = $(local_lilypond_datadir)/fonts/source
INSTALLATION_FILES = $(MF_FILES)

INSTALLATION_OUT_SUFFIXES = 1 2 3

INSTALLATION_OUT_DIR1 = $(local_lilypond_datadir)/fonts/otf
INSTALLATION_OUT_FILES1 = $(OTF_FILES) \
			  $(NCSB_OTFS)

INSTALLATION_OUT_DIR2 = $(local_lilypond_datadir)/fonts/svg
INSTALLATION_OUT_FILES2 = $(SVG_FILES)

INSTALLATION_OUT_DIR3 = $(local_lilypond_datadir)/fonts/type1
INSTALLATION_OUT_FILES3 = $(PFB_FILES)

export MFINPUTS := .:$(MFINPUTS)


default: $(ALL_GEN_FILES) \
	 $(outdir)/emmentaler-20.otf \
	 tree-regen \
	 $(outdir)/fonts.conf

.PHONY: tree-regen

# FIXME: temporary hack: must regenerate after building fonts
tree-regen: $(ALL_GEN_FILES)
	${MAKE} -C $(top-build-dir) link-mf-tree

##
## todo: this also depends on .tfm, FIXME.
$(outdir)/%.lisp \
$(outdir)/%.otf-gtable \
$(outdir)/%.enc \
$(outdir)/%.pe: $(outdir)/%.log
	$(buildscript-dir)/mf-to-table \
		--global-lisp=$(outdir)/$(<F:.log=.otf-gtable) \
		--lisp=$(outdir)/$(<F:.log=.lisp) \
		--outdir=$(outdir) \
		--enc $(outdir)/$(<F:.log=.enc) \
		$<

local-clean:
	rm -f mfplain.mem mfplain.log
	rm -f *.tfm *.log


$(outdir)/fonts.conf:
	echo '<fontconfig><dir>'$(shell cd $(outdir); pwd)'</dir></fontconfig>' > $@

$(NCSB_OTFS): $(NCSB_SOURCE_FILES) \
	      $(auxscript-dir)/pfx2ttf.fontforge
	$(foreach i, $(basename $(NCSB_SOURCE_FILES)), \
		$(FONTFORGE) -script $(auxscript-dir)/pfx2ttf.fontforge \
			$(i).pfb $(i).afm $(outdir)/ && ) true

# eof
