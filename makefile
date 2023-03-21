default:
	@echo This Makefile is not used for building. Use CMake instead.
	@echo Rather, this makefile is purely for holding some maintenance routines.

dist:
	git archive --format=tar --prefix=mlt-$(version)/ v$(version) | gzip >mlt-$(version).tar.gz

validate-yml:
	for file in $$(find src/modules -maxdepth 2 -type f -name \*.yml \! -name resolution_scale.yml); do \
		echo "validate: $$file"; \
		kwalify -f src/framework/metaschema.yaml $$file || exit 1; \
	done

codespell:
	codespell -w -q 3 \
	-L shotcut,sav,boundry,percentil,readded,uint,ith,sinc,amin,childs,seeked,writen \
	-S ChangeLog,cJSON.c,cJSON.h,RtAudio.cpp,RtAudio.h,*.rej,mlt_wrap.*
