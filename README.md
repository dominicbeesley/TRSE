First of all: see http://www.turborascal.com

# TRSE
Turbo Rascal Syntax Error full repo 
- C++
- Qt

## Compiling
- On windows, install MSVC 2019. On macos, Xcode
- First, download the Qt framework from https://www.qt.io/download. Install the latest framework of Qt6 (desktop application). \[On Windows make sure to install the MSVC 2019 kit: Select Components->Qt->Qt 6.2.x-> MSVC 2019 64-bit\]
- Clone this repo to a TRSE directory
  - On linux, you need to install a library that contains gl.h like mesa-common-dev (sudo apt-get install mesa-common-dev) etc
- Select "Release", and under the qt project/build make sure you set the build directory to be **TRSE/Release**
- Copy the directory "themes" in **TRSE/Publish/source/** to the **TRSE/Release** build directory 
- Make a symbolic link called "tutorials from your build directory to point to Publish/tutorials to access tutorial projects from the front page 
- Make a symbolic link called "units from your build directory to point to TRSE/Units to access the TRSE library 
- Make a symbolic link "project_templates" from your build directory to point to Publish/project_templates in order to access the "New Project" templates
- Compile & run!

# Source code information
A compiler UML diagram can be found here: https://github.com/leuat/TRSE/blob/master/uml/compiler.png
