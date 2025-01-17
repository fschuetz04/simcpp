HEADER=simcpp.h protothread.h
SOURCE=simcpp.cpp
EXE=example-minimal example-twocars example-resource

.PHONY: all clean

all: $(EXE)

%: %.cpp $(HEADER) $(SOURCE)
	g++ -Wall -Wextra -std=c++11 $< $(SOURCE) -o $@

test: test.cpp $(HEADER) $(SOURCE)
	g++ -Wall -Wextra -std=c++11 --coverage $< $(SOURCE) -o $@ -lgtest_main -lgtest -lpthread

clean:
	rm -f $(EXE) test
