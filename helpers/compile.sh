clang++  -o bin/IntervalPass src/IntervalPass.cpp src/SeparationTracker.cpp src/ValueTracker.cpp `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
