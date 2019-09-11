VULKAN_SDK_PATH = /home/thomas/vulkanSDK/x86_64

CXX = g++

SRC = ../src
INC = ../include
SHADERDIR = ../shaders
OBJ = obj

CFLAGS = -g -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(INC)
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

_OBJS = $(wildcard $(SRC)/*.cpp)
OBJS = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(_OBJS))

SHADERS = $(wildcard $(SHADERDIR)/*.vert)
SHADERS += $(wildcard $(SHADERDIR)/*.frag)

BIN = vktest

ifeq ($(NDEBUG),1)
	CFLAGS += -DNDEBUG
endif

ifeq ($(WS),null)
	CFLAGS += -DUSE_NULLWS
endif

.PHONY: run clean

$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(BIN): $(OBJS) $(SHADERS)
	#@echo "Compiling shaders"
	#@cd $(SHADERDIR) && ./compile.sh
	$(CXX) $(CFLAGS) $(DIRECTIVES) -o $@ $(OBJS) $(LDFLAGS) 

run: $(BIN)
	LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/vulkan/explicit_layer.d ./$(BIN)

clean:
	rm -rf $(BIN) $(OBJ)
