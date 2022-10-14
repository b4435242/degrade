ENV = bear llvm
BUILD = frontend 
ALL_MODULES = $(ENV) $(BUILD)
all: $(ALL_MODULES)
env: $(ENV)
build: $(BUILD)

LLVM_DIR = $(DG_ROOT)/build/llvm-project
LLVM_BUILD_DIR = $(LLVM_DIR)/build

# Frontend #

frontend: $(LLVM_DIR)/clang-tools-extra/angelix
	grep -q angelix "$(LLVM_DIR)/clang-tools-extra/CMakeLists.txt" || echo 'add_subdirectory(angelix)' >> "$(LLVM_DIR)/clang-tools-extra/CMakeLists.txt"
	cd "$(LLVM_BUILD_DIR)" && ninja
	cp $(LLVM_BUILD_DIR)/bin/instrument-suspicious $(DG_ROOT)/bin
	cp $(LLVM_BUILD_DIR)/bin/apply-patch $(DG_ROOT)/bin
	#cp -r "$(LLVM_DIR)/build/lib/clang/3.7.0/include/"* "$(DG_ROOT)/build/include"

$(LLVM_DIR)/clang-tools-extra/angelix:
	ln -fs "$(DG_ROOT)/src/frontend" "$(LLVM_DIR)/clang-tools-extra/angelix"

# bear #

bear:
	sudo apt update
	sudo apt install -y bear

# llvm #

llvm:
	# clone source
	#mkdir -p build && cd build && git clone https://github.com/llvm/llvm-project.git
	# build with c++
	#mkdir -p build/llvm-project/build && cd build/llvm-project/build && \
	#cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release && \
    #   	ninja && \
	#sudo ninja install
	# rebuild with clang++
	mkdir -p build/llvm-project/build && cd build/llvm-project/build && \
	cmake -G Ninja ../llvm -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/bin/clang -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_ENABLE_RTTI=ON -DCMAKE_BUILD_TYPE=Debug &&\
       	ninja && \
	sudo ninja install
	

