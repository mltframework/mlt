default:
	@echo This Makefile is not used for building. Use CMake instead.
	@echo Rather, this makefile is purely for holding some maintenance routines.

dist:
	git archive --format=tar --prefix=mlt-$(version)/ v$(version) | gzip >mlt-$(version).tar.gz

validate-yml:
	for file in $$(find src/modules -maxdepth 2 -type f -name \*.yml \! -name resolution_scale.yml \! -name filter_info.yml); do \
		echo "validate: $$file"; \
		kwalify -f src/framework/metaschema.yaml $$file || exit 1; \
	done

codespell:
	codespell -w -q 3 \
	-L amin,boun,boundry,childs,deques,hsi,indx,ith,mis,nast,parms,percentil,readded,sav,seeked,shotcut,sinc,slin,uint,writen \
	-S ChangeLog,cJSON.c,cJSON.h,RtAudio.cpp,RtAudio.h,*.rej,mlt_wrap.*,blacklist.txt,filter_info.yml,generate_filter_info.py,\
	./build/*,./docs/html/*,./src/modules/decklink/darwin/*,./src/modules/decklink/linux/*,./src/modules/decklink/win/*,./src/modules/glaxnimate/glaxnimate/*,./src/modules/openfx/openfx/*,./src/swig/ruby/markdown/*

cppcheck:
	cppcheck src/ --force --quiet --inline-suppr --library=qt --error-exitcode=1 \
		-j $(shell nproc) \
		-i src/modules/decklink/darwin \
		-i src/modules/decklink/linux \
		-i src/modules/decklink/win \
		-i src/modules/glaxnimate/glaxnimate/ \
		-i src/modules/plus/ebur128/ \
		-i src/modules/xml/common.c \
		--include=src/framework/mlt_log.h \
		--include=src/framework/mlt_types.h \
		--library=cppcheck.cfg \
		--suppress=ctuOneDefinitionRuleViolation \
		--suppress=syntaxError:src/modules/xml/common.c \
    --suppress=syntaxError:src/modules/placebo/*.c

docker: Dockerfile
	docker build -t melt .
