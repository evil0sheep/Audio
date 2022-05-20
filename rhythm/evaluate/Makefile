CXX=clang++
RM=rm -f

CPPFLAGS=-std=c++14 -g -O3 -D TRAIN_PARAMETERS
LDFLAGS=-lkissfft-float -lmad

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $(addsuffix $2, $1)) $(foreach d,$(wildcard $(addsuffix *, $1)),$(call rwildcard,$d/,$2))

MAKEFILE_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR=$(MAKEFILE_DIR)/..
SRCS=$(shell find $(SOURCEDIR) -name '*.cpp')
INCLUDES=-I$(MAKEFILE_DIR) -I$(SOURCEDIR)

CC_SRCS := $(filter %.cpp,$(SRCS))
OBJS := $(patsubst %.cpp,%.o,$(CC_SRCS))
DEPFILES := $(patsubst %.cpp,%.d,$(CC_SRCS))


# $(warning CC_SRCS is $(CC_SRCS))
# $(warning OBJS is $(OBJS))
# $(warning DEPFILES is $(DEPFILES))
# $(warning INCLUDES is $(INCLUDES))

-include $(DEPFILES)

all: bin/evaluate

# run: bin/evaluate
#     ./bin/evaluate

%.o: %.cpp Makefile
	$(CXX) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

bin/evaluate: $(OBJS)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o bin/evaluate $^

clean:
	$(RM) bin/evaluate
	find $(SOURCEDIR) -type f -name '*.o' -delete