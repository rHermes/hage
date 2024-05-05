@echo off
echo "Building %1..."

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -property installationPath`) do (
  set InstallDir=%%i
)

call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" amd64

REM This is the actual compiler commands.

cmake -B build -G Ninja -S . -DCMAKE_CXX_COMPILER=cl -DCMAKE_C_COMPILER=cl -DCMAKE_BUILD_TYPE="%1"
cmake --build build --config "%1"
ctest --build-config "%1" --output-on-failure --output-junit test-results.xml --test-dir build