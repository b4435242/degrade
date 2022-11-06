ENV = bear llvm
BUILD = frontend 
ALL_MODULES = $(BUILD) $(ENV)
all: $(ALL_MODULES)
env: $(ENV)
build: $(BUILD)

LLVM_DIR = build/llvm-project
LLVM_BUILD_DIR = $(LLVM_DIR)/build
LIBCXX_DIR = $(DG_ROOT)/build/libcxx

# Frontend #

frontend: $(LLVM_DIR)/clang-tools-extra/angelix
	grep -q angelix "$(LLVM_DIR)/clang-tools-extra/CMakeLists.txt" || echo 'add_subdirectory(angelix)' >> "$(LLVM_DIR)/clang-tools-extra/CMakeLists.txt"
	cd "$(LLVM_BUILD_DIR)" && ninja
	mkdir -p $(DG_ROOT)/bin/angelix
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
	#mkdir -p build && cd build \
	#&& git clone https://github.com/llvm/llvm-project.git \
	#&&git checkout release/11.x
	
	# build with c++
	#mkdir -p build/llvm-project/build && cd build/llvm-project/build && \
	#cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release && \
    #   	ninja && \
	#sudo ninja install
	# rebuild with clang++
	mkdir -p build/llvm-project/build && cd build/llvm-project/build &&\
	cmake -G Ninja ../llvm -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/bin/clang -DCMAKE_CXX_STANDARD=17 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_ENABLE_RTTI=ON &&\
	ninja && sudo ninja install

# STP #

stp: minisat
	cd build && git clone https://github.com/stp/stp
	cd build/stp && git submodule init && git submodule update && ./scripts/deps/setup-gtest.sh && ./scripts/deps/setup-outputcheck.sh && mkdir build
	cd build/stp/build/ && cmake .. && cmake --build . && sudo cmake --install .

minisat:
	cd build && git clone https://github.com/stp/minisat.git
	cd build/minisat && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ ../ && sudo make install

gtest:
	cd build && curl -OL https://github.com/google/googletest/archive/release-1.11.0.zip && unzip release-1.11.0.zip

uclibc:
	cd build && git clone https://github.com/klee/klee-uclibc.git
	cd build/klee-uclibc && ./configure --make-llvm-lib --with-cc clang-11 --with-llvm-config llvm-config-11 && make

klee: #gtest klee_deps stp uclibc
	#cd build && git clone https://github.com/klee/klee.git
	mkdir -p build/libcxx
	#cd build/klee && LLVM_VERSION=11 BASE=$(DG_ROOT)/build/libcxx ./scripts/build/build.sh libcxx
	mkdir -p build/klee/build
	cd build/klee/build \
	&& cmake -DENABLE_SOLVER_STP=ON -DENABLE_POSIX_RUNTIME=ON \
	-DKLEE_UCLIBC_PATH=$(DG_ROOT)/build/klee-uclibc \
	-DENABLE_UNIT_TESTS=ON \
	-DGTEST_SRC_DIR=$(DG_ROOT)/build/googletest-release-1.11.0 \
	-DENABLE_KLEE_LIBCXX=ON \
	-DKLEE_LIBCXX_DIR=$(LIBCXX_DIR)/libc++-install-110 \
	-DKLEE_LIBCXX_INCLUDE_DIR=$(LIBCXX_DIR)/libc++-install-110/include/c++/v1 \
	-DENABLE_KLEE_EH_CXX=ON \
	-DKLEE_LIBCXXABI_SRC_DIR=$(LIBCXX_DIR)/llvm-110/libcxxabi \
	-DLLVM_CONFIG_BINARY=/usr/bin/llvm-config-11 \
	-DLLVMCC=/usr/bin/clang-11 \
	-DLLVMCXX=/usr/bin/clang++-11 \
	-DENABLE_TCMALLOC=OFF \
	../ \
	&& make \
	&& sudo make install
   
klee_deps:
	sudo apt-get install build-essential cmake curl file g++-multilib gcc-multilib git libcap-dev libgoogle-perftools-dev libncurses5-dev libsqlite3-dev libtcmalloc-minimal4 python3-pip unzip graphviz doxygen
	sudo pip3 install lit wllvm
	sudo apt-get install python3-tabulate
	sudo apt-get install clang-11 llvm-11 llvm-11-dev llvm-11-tools


runtime:
	export ANGELIX_SYMBOLIC_RUNTIME=true 
	cd src/runtime && make
	cp src/runtime/libangelix.bc bin

z3:
	#cd build && git clone https://github.com/Z3Prover/z3.git
	cd build/z3 && python3 scripts/mk_make.py --prefix=/usr/local --python --pypkgdir=/usr/local/lib/python-3.8/site-packages
	cd build/z3/build && make && sudo make install
