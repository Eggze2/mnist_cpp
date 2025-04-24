CXX = g++
CXXFLAGS = -Wall -std=c++17 -g
INCLUDES = -I./include -I/usr/local/include/Eigen

# 指定目录
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# 源文件和目标文件路径
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = $(BIN_DIR)/mnist_cpp

.PHONY: all clean

all: $(TARGET)

# 可执行文件生成规则
$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)  # 确保 bin 目录存在
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# 中间文件生成规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)  # 确保 build 目录存在
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理规则
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)