# CSE 291-H Project 1: Tofu

* Based on learnopengl.com
* VS 2019
* OpenGL 3
* Binary libs `libs/`:
    * glew, glfw, assimp

## Build
1. Make a build folder and enter it
2. CMake
```
cmake ..
```
3. Commandline build (or VS 2019)
```
MSBuild.exe .\tofu.sln -property:Configuration=Release
```
or
```
cmake --build . --config Release
```

4. Run: `bin/Release/tofu.exe`

## Issues
1. Only small deformation allowed
2. Damping: velocity * 0.999 per iteration
3. Collision: energy loss
    * when y < 0: y = 0 and vy = 0