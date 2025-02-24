
OUTDIR	:= cmake-build

run: all
	@echo ""
	./cmake-build/lppm

all: 
	mkdir -p $(OUTDIR)
	cmake -B $(OUTDIR) -GNinja
	cmake --build $(OUTDIR)

install:
	cmake --build $(OUTDIR) --config Release
	cmake --install $(OUTDIR) --config Release

clean:
	rm -rf $(OUTDIR)
