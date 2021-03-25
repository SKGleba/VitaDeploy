TITLE_ID = SKGD3PL0Y
TARGET   = VitaDeploy

ifdef mshell
VPKNAME  = VitaDeploy-mshell
SHELLF   = mshell.self
else
VPKNAME  = VitaDeploy
SHELLF   = vshell.self
endif

all: $(VPKNAME).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo;
	vita-pack-vpk -s param.sfo -b eboot.bin \
	-a res/tv-cfg.txt=rdparty/tv-cfg.txt \
	-a res/naavls.skprx=plugins/naavls.skprx \
	-a res/vs.sfo=sce_sys/vs.sfo \
	-a res/$(SHELLF)=vshell.self \
	-a res/imcunlock.skprx=plugins/imcunlock.skprx -a res/imcunlock.self=imcunlock.self \
	-a res/tiny_modoru.self=tiny_modoru.self -a res/tiny_modoru.suprx=rdparty/tiny_modoru.suprx -a res/tiny_modoru.skprx=plugins/tiny_modoru.skprx \
	-a res/enso_365.eo=rdparty/enso_365.eo -a res/enso_360.eo=rdparty/enso_360.eo \
	-a main.self=main.self -a cfgu.suprx=plugins/cfgu_v05.suprx -a cfgk.skprx=plugins/cfgk_v05.skprx \
	-a res/icon0.png=sce_sys/icon0.png \
	-a res/icon1.png=sce_sys/icon1.png \
	-a res/pic0.png=sce_sys/pic0.png \
	-a res/template.xml=sce_sys/livearea/contents/template.xml \
	-a res/hen.png=sce_sys/livearea/contents/hen.png \
	-a res/bg.png=sce_sys/livearea/contents/bg.png $@;