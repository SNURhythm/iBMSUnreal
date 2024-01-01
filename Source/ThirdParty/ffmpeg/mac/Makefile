# input: arm64/*.a + x86_64/*.a, output: *_uni.a
LIBS = $(patsubst arm64/%.a,%.a,$(wildcard arm64/*.a))
%.a:
	lipo -create -output $*_uni.a x86_64/$*.a arm64/$*.a

all: $(LIBS)

print_libs:
	@echo $(LIBS)

clean:
	rm -f *_uni.a
