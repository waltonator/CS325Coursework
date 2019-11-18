CXX=clang++
CFLAGS=-g -O3 `llvm-config --cxxflags --ldflags --system-libs --libs all` \
-Wno-unused-function -Wno-unknown-warning-option -o mccomp
SOURCES=mccomp.cpp parser.cpp tokens.cpp codegen.cpp

mccomp: $(SOURCES)
	$(CXX) $(SOURCES) $(CFLAGS) -o mccomp


clean:
	rm -rf mccomp
