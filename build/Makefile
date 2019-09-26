CXX = g++

SRC = ../src
INC = ../include
SHADERDIR = ../shaders
OBJ = obj
SPIRVDIR = spirv

CFLAGS = -g -std=c++17 -Wall -I${VULKAN_SDK}/include -I$(INC)
LDFLAGS = -L${VULKAN_SDK}/lib -lvulkan

_OBJS = $(wildcard $(SRC)/*.cpp)
OBJS = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(_OBJS))

SHADERS = $(wildcard $(SHADERDIR)/*.*)

SPV = $(patsubst $(SHADERDIR)/%,$(SPIRVDIR)/%,$(SHADERS))

GLSLANG = glslangValidator

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

# compile GLSL shaders to SPIR-V
$(SPIRVDIR)/%: $(SHADERDIR)/%
	@mkdir -p $(SPIRVDIR)
	$(GLSLANG) -s -V $< -o $@

# compile regular cpp files
$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# compile final executable
$(BIN): $(OBJS) $(SPV)
	$(CXX) $(CFLAGS) $(DIRECTIVES) -o $@ $(OBJS) $(LDFLAGS) 

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ)

nuke:
	rm -rf $(OBJ) $(SPIRVDIR) $(BIN) 
