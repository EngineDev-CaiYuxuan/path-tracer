\# 构建 

Remove-Item -Recurse -Force build 

cmake -S . -B build 

cmake --build build --config Debug

#运行

./build/bin/debug/path_tracer.exe

