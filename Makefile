HEADER=simcpp.h protothread.h
SOURCE=simcpp.cpp
EXE=example-minimal example-twocars example-resource

.PHONY: clean

all: $(EXE)

%: %.cpp $(HEADER) $(SOURCE)
	g++ -Wall -std=c++11 $< $(SOURCE) -o $@

test: test.cpp $(HEADER) $(SOURCE)
	g++ -Wall -std=c++11 $< $(SOURCE) -o $@ -lgtest_main -lgtest -lpthread

clean:
	rm $(EXE) test
