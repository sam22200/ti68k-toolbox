NAME := fftscn
SRC := scene.c
include ../../../runtime/rt.mk
PY := ../../../tools/pyenv/bin/python
BUNDLE_OUTPUT ?= x/fftcampaign.89g
BUNDLE_BUDGET ?= x/core-audit/budget.json
BUNDLE_NATIVE_BUDGET ?= x/core-audit/native-budget.json
PACKAGE_OUTPUT ?= x/packages
PACKAGE_CHAPTERS ?= packages/chapter-01.json
PACKAGE_BUDGET ?= 2700000
PACKAGE_RESERVE ?= 131072
PACKAGE_COMBINED ?=
PACKAGE_CORE ?= $(PACKAGE_OUTPUT)/fft-core.89g
DISC := $(ROOT)/roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue
BANKS := ffs1 ffs2 ffflow fftitle ffbm ffbn ffbo ffmg ffwm ffcj ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb
TI_FILES := $(addprefix data/,$(BANKS:=.89y))
.NOTPARALLEL:
.PHONY: core-area-impact-check
core-area-impact-check: $(TI_FILES)
	$(PY) check_area_impact.py
	$(PY) check_impact_bounds.py
.PHONY: area-impact-review
area-impact-review: data/ffcj.bin
	$(PY) review_area_impact.py
.PHONY: core-area-ui-check
core-area-ui-check: $(TI_FILES)
	$(PY) check_area_ui.py
	$(PY) check_area_ui_replay.py
	$(PY) check_ai_camera.py
.PHONY: area-target-review
area-target-review: data/ffcj.bin
	$(PY) review_area_target.py
.PHONY: core-ai-pause-check
core-ai-pause-check: $(TI_FILES)
	$(PY) check_ai_pause.py
.PHONY: core-audit core-audit-test core-audit-bench core-boundaries-check core-capacity-render-check port-limits
.PHONY: core-grid-check core-cell-check
core-cell-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_cell_state.py

core-grid-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_tactical_grid.py
	$(PY) check_grid_movement.py
port-limits:
	$(PY) ../tools/port_limits.py "$(DISC)"
core-audit:
	$(PY) ../tools/core_audit.py "$(DISC)"
	$(PY) ../tools/port_limits.py "$(DISC)"
	$(PY) ../tools/core_fixtures.py
core-audit-test:
	$(PY) ../tools/test_core_audit.py
core-boundaries-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_core_boundaries.py
core-capacity-render-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_capacity_render.py
core-audit-bench:
	$(PY) ../tools/core_budget.py --native --group $(BUNDLE_OUTPUT) --out $(BUNDLE_NATIVE_BUDGET)
.PHONY: range-cancel-check
range-cancel-check:
	$(PY) check_range_cancel.py
.PHONY: shared-font-check shared-font-review rotation-check
rotation-check:
	$(PY) check_rotation.py
shared-font-check:
	$(PY) check_shared_font.py
shared-font-review:
	$(PY) review_shared_font.py
.PHONY: qa-check
qa-check: $(TI_FILES)
	$(MAKE) -C ../opening banks
	$(PY) check_qa.py
.PHONY: assets bundle
check: pc test $(TI_FILES)
	$(PY) check.py
check: common-check
check: charge-check
.PHONY: world-storage-check
world-storage-check: data/ffwm.89y data/ffmg.89y
	$(PY) check_world_storage.py "$(DISC)"
check: world-storage-check
.PHONY: charge-check charge-source-check
CHARGE_STATE := $(ROOT)/sources/fft_ps1/gariland_t1.state
$(CHARGE_STATE):
	$(MAKE) -C .. ../../sources/fft_ps1/gariland_t1.state
x/charge/source.h: ../tools/charge_oracle.py $(CHARGE_STATE)
	$(PY) ../tools/charge_oracle.py "$(DISC)" $(CHARGE_STATE) x/charge/source.json
charge-source-check: $(CHARGE_STATE)
	$(PY) ../tools/charge_oracle.py "$(DISC)" $(CHARGE_STATE) x/charge/source.json
charge-check: x/charge/source.h $(TI_FILES)
	$(PY) check_charge.py
.PHONY: charge-review
charge-review: data/ffcj.bin data/ffgd.bin | contour-assets
	@mkdir -p x/charge
	$(CC) $(PC_CFLAGS) -o x/charge/preview preview_charge.c $(RT_CORE) $(RT_SW)
	cd data && ../x/charge/preview ../x/charge
	$(PY) -c "from PIL import Image,ImageDraw; from pathlib import Path; p=Path('x/charge'); b=Image.new('RGB',(1464,324),'white'); d=ImageDraw.Draw(b); [(b.paste(Image.open(p/n).resize((480,300),Image.Resampling.NEAREST),(492*i,24)),d.text((492*i+6,5),t,fill='black')) for i,(n,t) in enumerate([('first.png','First: more below'),('middle.png','Middle: more above and below'),('last.png','Last: more above')])]; b.save(p/'menu-review.png')"
.PHONY: common-check
common-check: data/ffcj.89y
	$(PY) check_common.py
test: status-test
test: save-test
test: abilities-test
test: manage-test
test: map-test
test: dialogue-test
test: scene-test
test: actor-cache-test
test: deploy-test
test: title-test
.PHONY: title-test
title-test: data/fftitle.bin data/ffmg.bin
	@mkdir -p x/qa
	$(CC) $(PC_CFLAGS) -o x/qa/title-test test_title.c $(RT_CORE) $(RT_SW)
	./x/qa/title-test
.PHONY: deploy-test
deploy-test: data/ffflow.bin data/ffmg.bin data/ffcj.bin
	@mkdir -p x/qa
	$(CC) $(PC_CFLAGS) -o x/qa/deploy-test test_deploy.c $(RT_CORE) $(RT_SW)
	./x/qa/deploy-test
.PHONY: scene-test
scene-test: data/ffs1.bin
	$(CC) $(PC_CFLAGS) -o fftscn_test test_scene.c $(RT_CORE) $(RT_SW)
	./fftscn_test
.PHONY: actor-cache-test
actor-cache-test:
	@mkdir -p x/qa
	$(CC) $(PC_CFLAGS) -o x/qa/actor-cache-test test_actor_cache.c
	./x/qa/actor-cache-test
.PHONY: dialogue-test
dialogue-test:
	$(CC) $(PC_CFLAGS) -o fftscn_dialogue_test test_dialogue.c $(RT_CORE) $(RT_SW)
	./fftscn_dialogue_test
.PHONY: map-test
map-test: data/ffwm.bin data/ffmg.bin
	$(CC) $(PC_CFLAGS) -o fftmap_test test_worldmap.c $(RT_CORE) $(RT_SW)
	./fftmap_test
.PHONY: manage-test
manage-test: data/ffmg.bin data/ffcj.bin
	$(CC) $(PC_CFLAGS) -o fftmng_test test_manage.c $(RT_CORE) $(RT_SW)
	./fftmng_test
fftmng_pc: manage.c pc_entry.c ../common.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ manage.c $(RT_CORE) $(RT_SW) $(RT)platform-sdl/rt_sdl.c $(SDL_LIBS)
.PHONY: save-test
save-test: data/ffcj.bin data/ffgd.bin
	$(CC) $(PC_CFLAGS) -o fftgar_save_test test_save.c $(RT_CORE) $(RT_SW)
	./fftgar_save_test
.PHONY: status-test
status-test: data/ffcj.bin data/ffgd.bin
	$(CC) $(PC_CFLAGS) -o fftgar_status_test test_status.c $(RT_CORE) $(RT_SW)
	./fftgar_status_test
.PHONY: abilities-test
abilities-test: data/ffcj.bin data/ffgd.bin ../oracle.h
	$(CC) $(PC_CFLAGS) -o fftgar_abilities_test test_abilities.c $(RT_CORE) $(RT_SW)
	./fftgar_abilities_test
../oracle.h:
	$(MAKE) -C .. oracle.h
assets: data/ffcj.bin
	$(PY) ../tools/campaign_assets.py "$(DISC)"
data/%.89y: data/%.bin
	cd data && ../../../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $*.bin $*
	python3 -c "from pathlib import Path; p=Path('$@'); b=bytearray(p.read_bytes()); b[0x49]=3; p.write_bytes(b)"
fftscn_pc: scene.c pc_entry.c $(RT_CORE) $(RT_SW)
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ scene.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
pc: fftintro_pc fftgar_pc fftbatflow_pc fftdep_pc fftmap_pc fftmng_pc ffttitle_pc
ffttitle_pc: title.c pc_title_entry.c save.h save_slots.h mgtext.inc ../choice_menu.h ../choice_menu.inc data/fftitle.bin data/ffmg.bin
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ title.c $(RT_CORE) $(RT_SW) pc_title_entry.c $(SDL_LIBS)
fftbatflow_pc: flow_battle.c ../fft.c ../fft.h save.h flow.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ flow_battle.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
fftdep_pc: deploy.c manage.c deployment.inc save.h flow.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ deploy.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
fftmap_pc: worldmap.c save.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ worldmap.c $(RT_CORE) $(RT_SW) pc_map_entry.c $(SDL_LIBS)
fftbatflow_pc: $(wildcard ../*.inc *.inc *.h) ../opening/opening.inc
fftdep_pc fftmng_pc: $(wildcard *.inc *.h)
fftbatflow_pc fftdep_pc fftmng_pc fftgar_pc fftintro_pc fftmap_pc: ../unit_card.h ../unit_card.inc ../portrait32.inc ../choice_menu.h ../choice_menu.inc
fftbatflow_pc fftdep_pc fftmng_pc fftgar_pc fftintro_pc fftmap_pc: save.h save_slots.h
fftbatflow_pc fftdep_pc fftmng_pc fftgar_pc fftintro_pc fftmap_pc ffttitle_pc fftscn_pc: save_types.h
fftscn_pc: flow.h flow_draw.inc save.h
fftgar_pc fftintro_pc: ../stats.inc ../unit_setup.inc ../formula.inc
fftgar_pc fftintro_pc fftbatflow_pc: ../action_effect.h ../action_resolve.inc
fftgar_pc fftintro_pc fftbatflow_pc: ../status_rules.h ../common.h
fftgar_pc fftintro_pc fftbatflow_pc: ../status_policy.h ../status_unit.h ../status_policy.inc ../status_lifetime.h ../status_control.h ../status_runtime.inc ../turn_order.inc ../setup_glyphs.inc
fftdep_pc fftmap_pc ffttitle_pc fftscn_pc fftmng_pc: ../common.h
fftgar_pc fftintro_pc fftbatflow_pc fftscn_pc: ../core_limits.h
fftgar_pc fftintro_pc fftbatflow_pc: ../chapter.h ../chapters.h ../module.h
fftintro_pc: ../opening/opening.c ../fft.c ../fft.h ../opening/opening.inc pc_entry.c
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ ../opening/opening.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
fftgar_pc: gariland.c ../fft.c ../fft.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ gariland.c $(RT_CORE) $(RT_SW) $(RT)platform-sdl/rt_sdl.c $(SDL_LIBS)
# AMS retains each archived executable's RAM copy until the supervisor returns.
# Keep every child entry a small packed stub, including repeatedly visited menus.
OPENING_BANKS := $(addprefix ../opening/data/,$(addsuffix .89y,ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffod ffs0))
.PHONY: packed package-probe
package-probe:
	@mkdir -p x/package
	$(ROOT)/tools/bin/ti-cc -o x/package/heapprob probe_package.c
packed: $(TI_FILES)
	$(MAKE) -C ../opening banks
	$(ROOT)/tools/bin/ti-cc -o fftbat -pack ffbcore native_battle.c $(RT_CORE) $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftscn -pack ffscore native_scene.c $(RT_CORE) $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftdep -pack ffpccore native_deploy.c $(RT_CORE) $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftmap -pack ffwmcore native_worldmap.c $(RT_CORE) $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftmng -pack ffmgcore native_manage.c $(RT_CORE) $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o ffttitle -pack ffttcore native_title.c $(RT_CORE) $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftstory launcher.c
	python3 -c "from pathlib import Path; files=[Path(n) for n in 'ffbcore.89y ffscore.89y ffpccore.89y ffwmcore.89y ffmgcore.89y ffttcore.89y fftdep.89z fftscn.89z fftbat.89z fftmng.89z fftmap.89z ffttitle.89z'.split()]; [(lambda p,b: (b.__setitem__(0x49,3),p.write_bytes(b)))(p,bytearray(p.read_bytes())) for p in files]"
bundle: PACKAGE_COMBINED = $(BUNDLE_OUTPUT)
bundle: packages
	$(PY) ../tools/core_budget.py --group $(BUNDLE_OUTPUT) --out $(BUNDLE_BUDGET)

.PHONY: packages chapter-package packages-check core-packages-check packages-routing-check
.PHONY: core-resources-check
core-resources-check: packages
	$(PY) check_chapter_resources.py --input $(PACKAGE_OUTPUT)/source.89g --out $(PACKAGE_OUTPUT)/resources
packages: packed
	@mkdir -p $(PACKAGE_OUTPUT)
	$(ROOT)/tools/bin/ti-group $(PACKAGE_OUTPUT)/source.89g fftstory.89z ffttitle.89z ffttcore.89y fftmap.89z ffwmcore.89y fftmng.89z ffmgcore.89y fftdep.89z ffpccore.89y fftbat.89z ffbcore.89y fftscn.89z ffscore.89y $(OPENING_BANKS) $(TI_FILES)
	$(PY) ../tools/chapter_packages.py --input $(PACKAGE_OUTPUT)/source.89g --out $(PACKAGE_OUTPUT) $(if $(PACKAGE_COMBINED),--combined $(PACKAGE_COMBINED),) $(foreach spec,$(PACKAGE_CHAPTERS),--chapter $(spec)) --budget $(PACKAGE_BUDGET) --reserve $(PACKAGE_RESERVE)
packages-check:
	$(PY) check_chapter_packages.py --out $(PACKAGE_OUTPUT)/check
core-packages-check: packages-check
chapter-package:
	$(PY) ../tools/chapter_packages.py --reuse-core $(PACKAGE_CORE) --data-dir data --data-dir ../opening/data --out $(PACKAGE_OUTPUT) $(foreach spec,$(PACKAGE_CHAPTERS),--chapter $(spec)) --budget $(PACKAGE_BUDGET) --reserve $(PACKAGE_RESERVE)
packages-routing-check: packages
	$(PY) check_package_routing.py --input $(PACKAGE_OUTPUT)/source.89g --out $(PACKAGE_OUTPUT)/routing

.PHONY: contour-assets
contour-assets:
	$(PY) ../tools/contour_assets.py --attach data/ffgv0.bin
pc test cycles ti packed regression-check visual-check: | contour-assets
fftgar_pc: ../config.inc ../contour.h ../contour_art.inc

../contour.h ../contour_art.inc: ../tools/contour_assets.py ../art.h ../setup_glyphs.inc ../fft.c ../config.inc
	$(PY) ../tools/contour_assets.py
	@touch ../contour.h ../contour_art.inc

fftintro_pc fftgar_pc fftgar_status_test: | contour-assets

# The selected style is an actual bank prerequisite, so .89y wrappers cannot
# silently retain older colours after a phony preparation target runs.
data/ffgv0.bin data/ffgv1.bin data/ffgv2.bin data/ffgv3.bin &: ../tools/campaign_scenery.py $(wildcard ../x/isometric-style/variant-*.raw)
	$(PY) ../tools/campaign_scenery.py
	$(PY) ../tools/contour_assets.py --attach data/ffgv0.bin
	touch data/ffgv0.bin data/ffgv1.bin data/ffgv2.bin data/ffgv3.bin

data/ffgd.bin: ../tools/chapter_desc.py ../tools/surface_data.py ../core_limits.h data/battle.h data/interface.h ../map.h data/actors.h data/ffcj.bin data/ffcp.bin
	$(PY) ../tools/chapter_desc.py campaign

data/ffbm.bin data/ffbn.bin data/ffbo.bin &: ../tools/module.py battle.c ../fft.c ../fft.h ../core_limits.h ../chapter.h ../module.h ../unit_card.h ../choice_menu.h ../action_effect.h ../status_rules.h ../status_policy.h ../status_unit.h ../status_lifetime.h ../status_control.h ../reflect_rules.h ../common.h $(wildcard ../*.inc ../*.h *.inc *.h) ../opening/opening.inc
	$(PY) ../tools/module.py data/ffbm.bin battle.c 28 --second data/ffbn.bin --third data/ffbo.bin --companion 27
	@touch data/ffbm.bin data/ffbn.bin data/ffbo.bin data/modules.h

data/ffcp.bin ../opening/data/ffcp.bin &: ../tools/common_portraits.py ../tools/identity_assets.py ../tools/management_data.py
	$(PY) ../tools/common_portraits.py "$(DISC)"
	@touch data/ffcp.bin ../opening/data/ffcp.bin
data/ffmg.bin: ../tools/management_data.py ../tools/battle.py ../tools/units.py ../tools/interface.py data/ffcp.bin
	$(PY) ../tools/management_data.py "$(DISC)" $@

data/ffcj.bin ../opening/data/ffcj.bin &: ../tools/common_tables.py ../tools/battle.py ../tools/management_data.py
	$(PY) ../tools/common_tables.py "$(DISC)"

data/ffwm.bin: ../tools/world_assets.py ../tools/management_data.py ../tools/units.py ../tools/interface.py
	$(PY) ../tools/world_assets.py "$(DISC)" $@

FLOW_STATE := x/mandalia/gariland-deployment.state
$(FLOW_STATE):
	$(PY) ../tools/mandalia_preflight.py "$(DISC)"
data/ffflow.bin: ../tools/flow_assets.py ../tools/shared_font_assets.py $(ROOT)/lib/bitmap_font.py $(FLOW_STATE)
	$(PY) ../tools/flow_assets.py "$(DISC)" $(FLOW_STATE)
data/fftitle.bin: ../tools/title_assets.py
	$(PY) ../tools/title_assets.py "$(DISC)"

data/ffbm.bin: $(ROOT)/lib/bitmap_font.c $(ROOT)/lib/bitmap_font.h
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../tactical_grid.h
fftscn_pc fftbatflow_pc: $(ROOT)/lib/bitmap_font.c $(ROOT)/lib/bitmap_font.h

data/ffs1.bin data/ffs2.bin &: ../tools/scenes.py data/ffac.bin data/academy.h
	$(PY) ../tools/scenes.py

.PHONY: white-title-review
white-title-review: $(TI_FILES) $(OPENING_BANKS)
	$(PY) review_white_titles.py

data/ffbm.bin fftbat_pc fftgar_pc fftintro_pc fftbatflow_pc: ../module_bank.h

.PHONY: core-full-grid-check
core-full-grid-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_full_grid.py

.PHONY: core-surface-check
core-surface-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_surface.py
	$(PY) check_surface_battle.py

.PHONY: core-jump-check
core-jump-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(MAKE) -C ../opening data/ffod.89y
	$(PY) check_jump_surface.py
	$(PY) check_jump_battle.py
	$(PY) check_jump_graph.py

.PHONY: core-profile-check core-profile-deep-check
core-profile-check:
	$(PY) check_surface_profile.py
core-profile-deep-check:
	$(PY) check_surface_profile.py --deep

.PHONY: core-knockback-check
core-knockback-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_knockback_geometry.py
	$(PY) check_knockback_battle.py

.PHONY: core-step-check
core-step-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_step_geometry.py
	$(PY) check_step_battle.py

.PHONY: core-step-cache-check core-long-jump-check
core-step-cache-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_step_cache.py
core-long-jump-check:
	$(PY) check_long_jump.py

.PHONY: core-span-search-check
core-span-search-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_tactical_grid.py --out x/core-span-search/kernel
	$(PY) check_span_battle.py

data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../surface_geometry.h
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../knockback_geometry.h
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../step_geometry.h
data/ffgd.bin: ../tools/surface_data.py ../surface_geometry.h

.PHONY: core-jump-actors-check
core-jump-actors-check:
	$(PY) check_jump_actors.py

.PHONY: core-jump-cost-check
core-jump-cost-check:
	$(PY) check_jump_cost.py

.PHONY: core-compact-check
.PHONY: core-actions-check
.PHONY: core-status-rules-check
.PHONY: core-status-state-check
x/core-status-state/oracle/fsp00.bin: ../tools/status_state_oracle.py ../tools/common_tables.py
	$(PY) $<
core-status-state-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-status-state/oracle/fsp00.bin
	$(PY) check_status_state.py

x/core-status-rules/oracle/source.h: ../tools/status_rules_oracle.py
	$(PY) $<
core-status-rules-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-status-rules/oracle/source.h
	$(PY) check_status_rules.py

core-actions-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_actions.py

core-compact-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffgd.89y
	$(PY) check_tactical_grid.py --out x/core-compact/kernel
	$(PY) check_compact_graph.py
	$(PY) check_full_grid.py

.PHONY: core-status-lifetime-check
.PHONY: core-sleep-check
.PHONY: core-context-check
.PHONY: core-reactions-check
.PHONY: core-strikes-check
.PHONY: core-hands-check
.PHONY: core-charge-hands-check
.PHONY: core-deferred-check
.PHONY: core-casting-check
.PHONY: core-incantation-check
.PHONY: core-affinity-check
.PHONY: core-elements-check
x/core-elements/oracle/source.h: ../tools/elements_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-elements-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-elements/oracle/source.h
	$(PY) check_elements.py
x/core-affinity/oracle/source.h: ../tools/affinity_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-affinity-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-affinity/oracle/source.h
	$(PY) check_affinity.py
.PHONY: core-magic-check
x/core-magic/oracle/source.h: ../tools/magic_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-magic-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-magic/oracle/source.h
	$(PY) check_magic.py
x/core-incantation/oracle/source.h: ../tools/incantation_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-incantation-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-incantation/oracle/source.h
	$(PY) check_incantation.py
x/core-casting/oracle/source.h: ../tools/casting_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-casting-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-casting/oracle/source.h
	$(PY) check_casting.py
x/core-deferred/oracle/source.h: ../tools/deferred_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-deferred-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-deferred/oracle/source.h
	$(PY) check_deferred.py

x/core-scheduler/oracle/source.h: ../tools/scheduler_oracle.py
	$(PY) ../tools/scheduler_oracle.py
.PHONY: core-scheduler-check
core-scheduler-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-scheduler/oracle/source.h
	$(PY) check_scheduler.py

x/core-haste-slow/oracle/source.h: ../tools/haste_slow_oracle.py
	$(PY) ../tools/haste_slow_oracle.py
.PHONY: core-haste-slow-check
core-haste-slow-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-haste-slow/oracle/source.h
	$(PY) check_haste_slow.py

x/core-incapacity/oracle/source.h: ../tools/incapacity_oracle.py
	$(PY) ../tools/incapacity_oracle.py
.PHONY: core-incapacity-check
core-incapacity-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-incapacity/oracle/source.h
	$(PY) check_incapacity.py
.PHONY: core-offensive-magic-check
core-offensive-magic-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y
	$(PY) check_offensive_magic.py
.PHONY: core-reflect-check
x/core-reflect/oracle/source.h: ../tools/reflect_oracle.py
	$(PY) ../tools/reflect_oracle.py
core-reflect-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-reflect/oracle/source.h
	$(PY) check_reflect.py
.PHONY: core-percent-check
.PHONY: core-cleanse-check
.PHONY: core-terminal-check
x/core-terminal/oracle/source.h: ../tools/terminal_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/terminal_oracle.py
core-terminal-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-terminal/oracle/source.h
	$(PY) check_terminal.py
.PHONY: core-talk-skill-check
x/core-talk-skill/oracle/source.h: ../tools/talk_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/talk_oracle.py
core-talk-skill-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-talk-skill/oracle/source.h
	$(PY) check_talk.py
.PHONY: core-ruin-check
x/core-ruin/oracle/source.h: ../tools/ruin_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/ruin_oracle.py
core-ruin-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-ruin/oracle/source.h
	$(PY) check_ruin.py
.PHONY: core-holy-sword-check
x/core-holy-sword/oracle/source.h: ../tools/holy_sword_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/holy_sword_oracle.py
core-holy-sword-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-holy-sword/oracle/source.h
	$(PY) check_holy_sword.py
.PHONY: core-punch-strikes-check
x/core-punch-strikes/oracle/source.h: ../tools/punch_strike_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/punch_strike_oracle.py
core-punch-strikes-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-punch-strikes/oracle/source.h
	$(PY) check_punch_strikes.py
.PHONY: core-punch-art-check
x/core-punch-art/oracle/source.h: ../tools/punch_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/punch_oracle.py
core-punch-art-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-punch-art/oracle/source.h
	$(PY) check_punch.py
.PHONY: core-protect-check
x/core-protect/oracle/source.h: ../tools/protect_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/protect_oracle.py
core-protect-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-protect/oracle/source.h
	$(PY) check_protect.py
.PHONY: core-control-status-check
x/core-control-status/oracle/source.h: ../tools/control_status_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/control_status_oracle.py
core-control-status-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-control-status/oracle/source.h
	$(PY) check_control_status.py
.PHONY: core-restoration-check
x/core-restoration/oracle/source.h: ../tools/restoration_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/restoration_oracle.py
core-restoration-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-restoration/oracle/source.h
	$(PY) check_restoration.py
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../item_restore.h ../restoration_ai.h ../menu_input.inc
x/core-cleanse/oracle/source.h: ../tools/cleanse_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/cleanse_oracle.py
core-cleanse-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-cleanse/oracle/source.h
	$(PY) check_cleanse.py
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../cleanse_rules.h ../facing_arrows.inc
x/core-percent/oracle/source.h: ../tools/percent_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/percent_oracle.py
core-percent-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-percent/oracle/source.h
	$(PY) check_percent.py

.PHONY: core-buffs-check
x/core-buffs/oracle/source.h: ../tools/buffs_oracle.py
	$(PY) ../tools/buffs_oracle.py
core-buffs-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-buffs/oracle/source.h window_reference.inc
	$(PY) check_buffs.py

.PHONY: core-healing-check
x/core-healing/oracle/source.h: ../tools/healing_oracle.py
	$(PY) ../tools/healing_oracle.py
core-healing-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-healing/oracle/source.h banner_reference.inc
	$(PY) check_healing.py
x/core-charge-hands/oracle/source.h: ../tools/charge_hands_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-charge-hands-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-charge-hands/oracle/source.h
	$(PY) check_charge_hands.py
x/core-hands/oracle/source.h: ../tools/hands_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-hands-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-hands/oracle/source.h
	$(PY) check_hands.py
	$(PY) check_hands_manage.py
x/core-strikes/oracle/source.h: ../tools/strike_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-strikes-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-strikes/oracle/source.h
	$(PY) check_strikes.py
x/core-reactions/oracle/source.h: ../tools/reaction_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-reactions-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-reactions/oracle/source.h
	$(PY) check_reactions.py
x/core-context/oracle/source.h: ../tools/context_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-context-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-context/oracle/source.h
	$(PY) check_context.py

data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../action_context.h ../action_start.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../reaction_policy.h ../reaction_runtime.inc ../action_module.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../rewards.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../strike_runtime.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../hands.inc ../command.inc ../ability_access.inc ../reaction_policy_runtime.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../charge_math.inc ../loot.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../cast_rules.h ../skill_range.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../cast_target.h ../incantation.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../magic_math.inc
x/core-sleep/oracle/source.h: ../tools/sleep_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-sleep-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-sleep/oracle/source.h
	$(PY) check_sleep.py

x/core-status-lifetime/oracle/fsl00.bin: ../tools/status_lifetime_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-status-lifetime-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-status-lifetime/oracle/fsl00.bin
	$(PY) check_status_lifetime.py

.PHONY: core-control-check
x/core-control/oracle/source.h: ../tools/control_oracle.py ../tools/status_state_oracle.py
	$(PY) $<
core-control-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-control/oracle/source.h
	$(PY) check_control.py

.PHONY: core-magic-evasion-check
core-magic-evasion-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_magic_evasion.py
../contour_art.inc: ../window_label.inc

data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../weather_rules.h ../forecast_panel.inc
.PHONY: core-weather-check
core-weather-check: data/ffbm.89y data/ffbn.89y
	$(PY) check_weather.py

.PHONY: core-targets-check
core-targets-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-targets/oracle/source.json
	$(PY) check_targets.py
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../area_targets.inc ../area_geometry.h

x/core-targets/oracle/source.json: ../tools/area_oracle.py
	$(PY) ../tools/area_oracle.py

.PHONY: core-batch-check
core-batch-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-batch/oracle/source.json
	$(PY) check_batch.py
x/core-batch/oracle/source.json: ../tools/batch_oracle.py
	$(PY) ../tools/batch_oracle.py
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../cast_batch.h ../batch_prepare.inc ../batch_commit.inc ../magic_prepare.inc ../magic_commit.inc

.PHONY: core-banks-check
core-banks-check: $(TI_FILES)
	$(PY) ../tools/test_module.py
	$(PY) check_banks.py

.PHONY: core-area-ct-check
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../chapter_validate.h
.PHONY: core-headroom-check
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../magic_quantity.inc ../batch_forecast.inc
data/ffbm.bin fftgar_pc fftintro_pc fftbatflow_pc: ../area_ai.h ../area_ui.h ../area_ai.inc ../area_ai_reduce.inc ../ai_evaluation.inc ../magic_forecast.inc
.PHONY: core-area-ai-check
core-area-ai-check: $(TI_FILES) x/core-area-ai/oracle/fai00.bin
	$(PY) check_area_ai.py

x/core-area-ai/oracle/source.json x/core-area-ai/oracle/source.h x/core-area-ai/oracle/fai00.bin &: ../tools/area_ai_oracle.py
	$(PY) ../tools/area_ai_oracle.py

.PHONY: core-area-forecast-check
core-area-forecast-check: $(TI_FILES) x/core-area-forecast/oracle/fbf00.bin
	$(PY) check_area_forecast.py

x/core-area-forecast/oracle/source.json x/core-area-forecast/oracle/fbf00.bin &: ../tools/area_forecast_oracle.py x/core-batch/oracle/source.json
	$(PY) ../tools/area_forecast_oracle.py

core-headroom-check: $(TI_FILES) $(OPENING_BANKS)
	$(PY) check_banks.py --out x/core-headroom/banks
	$(PY) check_core_boundaries.py --out x/core-headroom/boundaries

core-area-ct-check: $(TI_FILES) $(OPENING_BANKS)
	$(PY) check_area_ct.py
.PHONY: core-guts-check
x/core-guts/oracle/source.h: ../tools/guts_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/guts_oracle.py
core-guts-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-guts/oracle/source.h
	$(PY) check_guts.py
.PHONY: core-equip-breaks-check
x/core-equip-breaks/oracle/source.h: ../tools/equip_break_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/equip_break_oracle.py
core-equip-breaks-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-equip-breaks/oracle/source.h
	$(PY) check_equip_breaks.py
.PHONY: core-breaks-check
x/core-breaks/oracle/source.h: ../tools/break_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/break_oracle.py
core-breaks-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-breaks/oracle/source.h
	$(PY) check_breaks.py
.PHONY: core-steal-equip-check
x/core-steal-equip/oracle/source.h: ../tools/steal_equip_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/steal_equip_oracle.py
core-steal-equip-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-steal-equip/oracle/source.h
	$(PY) check_steal_equip.py
.PHONY: core-gil-check
x/core-gil/oracle/source.h: ../tools/gil_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/gil_oracle.py
core-gil-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-gil/oracle/source.h
	$(PY) check_gil.py

.PHONY: core-geomancy-check
.PHONY: core-secret-fist-check
x/core-secret-fist/oracle/source.h: ../tools/secret_fist_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/secret_fist_oracle.py
core-secret-fist-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-secret-fist/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_secret_fist.py

x/core-geomancy/oracle/source.h: ../tools/geomancy_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/geomancy_oracle.py
core-geomancy-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-geomancy/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y data/ffod.89y
	$(PY) check_geomancy.py
.PHONY: core-mp-break-check
x/core-mp-break/oracle/source.h: ../tools/mp_break_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/mp_break_oracle.py
core-mp-break-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-mp-break/oracle/source.h
	$(PY) check_mp_break.py
.PHONY: core-steal-exp-check
x/core-steal-exp/oracle/source.h: ../tools/steal_exp_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/steal_exp_oracle.py
core-steal-exp-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-steal-exp/oracle/source.h
	$(PY) check_steal_exp.py

.PHONY: core-line-check
x/core-line/oracle/source.h: ../tools/line_oracle.py ../tools/status_state_oracle.py ../tools/area_oracle.py
	$(PY) ../tools/line_oracle.py
core-line-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-line/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_line.py

.PHONY: core-draw-out-check
x/core-draw-out/oracle/source.h: ../tools/draw_out_oracle.py ../tools/status_state_oracle.py ../tools/area_oracle.py
	$(PY) ../tools/draw_out_oracle.py
core-draw-out-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-draw-out/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_draw_out.py

.PHONY: core-performance-check
x/core-performance/oracle/source.h: ../tools/performance_oracle.py ../tools/status_state_oracle.py ../tools/surface_data.py
	$(PY) ../tools/performance_oracle.py
core-performance-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-performance/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_performance.py

.PHONY: core-allegiance-check
x/core-allegiance/oracle/source.h: ../tools/allegiance_oracle.py ../tools/status_state_oracle.py ../tools/surface_data.py ../tools/common_tables.py
	$(PY) ../tools/allegiance_oracle.py
core-allegiance-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-allegiance/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_allegiance.py

.PHONY: core-float-check
x/core-float/oracle/source.h: ../tools/float_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/float_oracle.py
core-float-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-float/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_float.py

.PHONY: core-throw-check
x/core-throw/oracle/source.h: ../tools/throw_oracle.py
	$(PY) ../tools/throw_oracle.py
core-throw-check: $(TI_FILES) x/core-throw/oracle/source.h
	$(PY) check_throw.py

.PHONY: core-math-check
x/core-math/oracle/source.h x/core-math/oracle/spells.h &: ../tools/math_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/math_oracle.py
core-math-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y x/core-math/oracle/source.h x/core-math/oracle/spells.h
	$(PY) check_math.py

.PHONY: core-stat-save-check
x/core-stat-save/oracle/source.h: ../tools/stat_save_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/stat_save_oracle.py
core-stat-save-check: $(TI_FILES) x/core-stat-save/oracle/source.h
	$(PY) check_stat_save.py

.PHONY: core-critical-reactions-check
x/core-critical-reactions/oracle/source.h: ../tools/critical_reactions_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/critical_reactions_oracle.py
core-critical-reactions-check: $(TI_FILES) x/core-critical-reactions/oracle/source.h
	$(PY) check_critical_reactions.py

# Lancer Jump (distinct from ordinary movement jumps).
.PHONY: core-lancer-jump-check
core-lancer-jump-check: $(TI_FILES) x/core-lancer-jump/oracle/source.h
	$(MAKE) -C ../opening data/ffcj.89y
	$(PY) check_lancer_jump.py
x/core-lancer-jump/oracle/source.h: ../tools/lancer_jump_oracle.py
	$(PY) ../tools/lancer_jump_oracle.py

.PHONY: core-status-reactions-check
x/core-status-reactions/oracle/source.h: ../tools/status_reactions_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/status_reactions_oracle.py
core-status-reactions-check: $(TI_FILES) x/core-status-reactions/oracle/source.h
	$(PY) check_status_reactions.py

.PHONY: core-brave-faith-reactions-check
x/core-brave-faith-reactions/oracle/source.h: ../tools/brave_faith_reactions_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/brave_faith_reactions_oracle.py
core-brave-faith-reactions-check: $(TI_FILES) x/core-brave-faith-reactions/oracle/source.h
	$(PY) check_brave_faith_reactions.py

.PHONY: core-counter-reactions-check
x/core-counter-reactions/oracle/source.h: ../tools/counter_reactions_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/counter_reactions_oracle.py
core-counter-reactions-check: $(TI_FILES) x/core-counter-reactions/oracle/source.h
	$(PY) check_counter_reactions.py

.PHONY: core-movement-check
x/core-movement/oracle/source.h: ../tools/movement_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/movement_oracle.py
core-movement-check: $(TI_FILES) x/core-movement/oracle/source.h
	$(PY) check_movement.py

.PHONY: core-equipment-check
x/core-equipment/oracle/ffeq0.bin x/core-equipment/oracle/ffeq1.bin &: ../tools/equipment_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/equipment_oracle.py
core-equipment-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-equipment/oracle/ffeq0.bin x/core-equipment/oracle/ffeq1.bin
	$(PY) check_equipment.py
	$(PY) check_equipment_manage.py
	$(PY) check_equipment_ui.py
	$(PY) check_equipment_save.py

.PHONY: core-combat-check
x/core-combat/oracle/ffco0.bin x/core-combat/oracle/ffco1.bin x/core-combat/oracle/counts.h &: ../tools/combat_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/combat_oracle.py
core-combat-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y x/core-combat/oracle/ffco0.bin x/core-combat/oracle/ffco1.bin x/core-combat/oracle/counts.h
	$(PY) check_combat.py
	$(PY) check_combat_ui.py

.PHONY: core-discovery-check
x/core-discovery/oracle/ffdn0.bin: ../tools/discovery_oracle.py
	$(PY) $<
core-discovery-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y data/ffgd.89y data/ffmg.89y data/ffflow.89y x/core-discovery/oracle/ffdn0.bin
	$(PY) check_discovery.py
	$(PY) check_discovery_manage.py

.PHONY: core-mime-check core-mime-source-check
core-mime-source-check:
	$(PY) ../tools/mime_oracle.py
x/core-mime/oracle/rotations.h: ../tools/mime_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/mime_oracle.py
core-mime-check: x/core-mime/oracle/rotations.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_mime.py

.PHONY: core-hamedo-check core-hamedo-source-check
core-hamedo-source-check:
	$(PY) ../tools/hamedo_oracle.py
x/core-hamedo/oracle/source.h: ../tools/hamedo_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/hamedo_oracle.py
core-hamedo-check: x/core-hamedo/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_hamedo.py

.PHONY: core-weapon-reach-check core-weapon-reach-source-check
core-weapon-reach-source-check:
	$(PY) ../tools/weapon_reach_oracle.py
x/core-weapon-reach/oracle/source.h: ../tools/weapon_reach_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/weapon_reach_oracle.py
core-weapon-reach-check: x/core-weapon-reach/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_weapon_reach.py

.PHONY: core-common-check
core-common-check: common-check

.PHONY: core-weapon-formulas-source-check core-weapon-formulas-check
core-weapon-formulas-source-check:
	$(PY) ../tools/weapon_formula_oracle.py
x/core-weapon-formulas/oracle/source.h: ../tools/weapon_formula_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/weapon_formula_oracle.py
core-weapon-formulas-check: x/core-weapon-formulas/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_weapon_formulas.py

.PHONY: core-weapons-check core-weapons-source-check
core-weapons-source-check:
	$(PY) ../tools/weapon_oracle.py
x/core-weapons/oracle/source.h: ../tools/weapon_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/weapon_oracle.py
core-weapons-check: x/core-weapons/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_weapons.py

.PHONY: core-ai-interactions-check
core-ai-interactions-check: data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_ai_interactions.py

.PHONY: core-melee-collision-source-check core-melee-collision-check
core-melee-collision-source-check:
	$(PY) ../tools/melee_oracle.py
x/core-melee-collision/oracle/source.h: ../tools/melee_oracle.py
	$(PY) ../tools/melee_oracle.py
core-melee-collision-check: x/core-melee-collision/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_melee_collision.py

.PHONY: core-arcs-source-check core-arcs-check
core-arcs-source-check:
	$(PY) ../tools/arc_oracle.py
x/core-arcs/oracle/source.h: ../tools/arc_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/arc_oracle.py
core-arcs-check: x/core-arcs/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_arcs.py

.PHONY: core-panel-victims-source-check core-panel-victims-check
core-panel-victims-source-check:
	$(PY) ../tools/panel_victims_oracle.py
x/core-panel-victims/oracle/source.h: ../tools/panel_victims_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/panel_victims_oracle.py
core-panel-victims-check: x/core-panel-victims/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_panel_victims.py

.PHONY: core-direct-interception-source-check core-direct-interception-check
core-direct-interception-source-check:
	$(PY) ../tools/direct_interception_oracle.py
x/core-direct-interception/oracle/source.h: ../tools/direct_interception_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/direct_interception_oracle.py
core-direct-interception-check: x/core-direct-interception/oracle/source.h data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_direct_interception.py

.PHONY: core-final-combat-source-check core-final-combat-check
core-final-combat-source-check:
	$(PY) ../tools/height_donor_oracle.py
x/core-final-combat/oracle/source.h x/core-final-combat/oracle/source.json x/core-final-combat/oracle/fheight.bin &: ../tools/height_donor_oracle.py ../tools/status_state_oracle.py
	$(PY) ../tools/height_donor_oracle.py
core-final-combat-check: x/core-final-combat/oracle/source.h x/core-final-combat/oracle/fheight.bin data/ffbm.89y data/ffbn.89y data/ffbo.89y data/ffcj.89y
	$(PY) check_final_combat.py

.PHONY: core-identity-check
core-identity-check: data/ffcp.89y
	$(PY) check_identity.py
