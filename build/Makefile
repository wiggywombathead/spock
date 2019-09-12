CXX = g++

SRC = ../src
INC = ../include
SHADERDIR = ../shaders
OBJ = obj

CFLAGS = -g -std=c++17 -I${VULKAN_SDK}/include -I$(INC)
LDFLAGS = -lvulkan

_OBJS = $(wildcard $(SRC)/*.cpp)
OBJS = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(_OBJS))

SHADERS = $(wildcard $(SHADERDIR)/*.vert)
SHADERS += $(wildcard $(SHADERDIR)/*.frag)

BIN = spock

ifeq ($(BUILD),debug)
	CFLAGS += -DDEBUG
endif

ifeq ($(WS),null)
	CFLAGS += -DUSE_NULLWS
else
	LDFLAGS += `pkg-config --static --libs glfw3`
endif

.PHONY: run clean nuke

$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(BIN): $(OBJS) $(SHADERS)
	#@echo "Compiling shaders"
	#@cd $(SHADERDIR) && ./compile.sh
	$(CXX) $(CFLAGS) $(DIRECTIVES) -o $@ $(OBJS) $(LDFLAGS) 

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ)

nuke:
	rm -rf $(OBJ) $(BIN)
