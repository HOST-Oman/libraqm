# Prerequisite libraries
Before compiling, the following development libraries need to be installed.
   - [FreeType](https://www.freetype.org/ "FreeType")
   - [HarfBuzz](https://www.freedesktop.org/wiki/Software/HarfBuzz/ "HarfBuzz")
   - [FriBiDi](https://www.fribidi.org/ "FriBiDi")

You can build FreeType natively on windows, just follow the Freetype build instructions. Also you can build HarfBuzz natively on windows by following HarfBuzz build instructions for windows. Make sure that you build HarfBuzz with FreeType option on. You can get the  Freetype's headers and library from the first step.
For FriBiDi, there is no official way to build it on windows. Fortunately, [ShiftMediaProject](https://github.com/ShiftMediaProject/fribidi "ShiftMediaProject") provides a native Visual Studio project for it. You have to build all pre-build optional components before you build FriBiDi itself.

# Building Libraqm:
First get the source code using git:

    git clone https://github.com/HOST-Oman/libraqm.git
## preparing  prerequisite libraries:
To make build process easier, we make assumptions on the directory structure are made to automate the discovery of dependencies. This recommended directory structure is described below:
```
win/
	include / 
	lib /
```
If you do not use the recommended structure you will need to enter paths by hand in CmakeLists.txt. Source and build directories can be located anywhere.
After building all prerequisite libraries, copy their headers into include directory and their libraries (*.lib) into lib directory. 
You can use our precompiled dependencies [package](https://sourceforge.net/projects/omlx/files/raqm/raqm-dep-win23-debug-29-12-2016.zip/download "package") if you want just test build libraqm quickly. Please notice that this package is not recommended for production. 
## Building libraqm:
### GUI compilation
1. Set up a work directory as described above. 
2. Open the CMake gui. 
3. Set "Where is the source code" to wherever you put the Libraqm sources (from the released tarball or the git repository). 
4. Set "Where to build the binaries" to a new empty directory (could be anywhere and any name, for example naming it raqm-build, at the same folder location as your unzipped Libraqm source folder was put). 
5. Press Configure button. The first time that the project is configured, CMake will bring up a window asking you to "Specify the generator for this project" i.e. which compiler you wish to use. Select Visual Studio 14 2015 (or Visual Studio 14 2015 Win64 for 64-bit), and press Finish. CMake will now do a check on your system and will produce a preliminary build configuration. 
6. CMake adds new configuration variables in red. Some have a value ending with -NOTFOUND. These variables should receive your attention. Some errors will prevent Libraqm from building and others will simply invalidate some options without causing build errors. You can set the right path for each prerequisite libraries. 
7. Repeat the process from step 5, until Generate button is enabled. 
8. Press Generate button. 
9. Start Visual Studio 2015 and open the Libraqm solution (raqm.sln) located in "Where to build the binaries". 
10. Choose the "Release" build in the toolbar. The right menu should read Win32 for 32-bits or x64 for 64-bits. 
11. Generate the solution with F7 key or right-click the top level "Solution raqm" in the Solution Explorer and choose Build. 
12. If there are build errors, return to CMake, clear remaining errors,  Configure  button and Generate button . 
13. When Visual Studio is able to build everything without errors, right-click on the INSTALL project (further down within the "Solution raqm" solution) and choose Build, which will put the include and lib files in ${CMAKE_INSTALL_PREFIX}
14. Enjoy!

### Script compilation
If you want to use the command line (cmd or powershell) to build libraqm, follow the bleow steps:
1- set up the directory structure as above
2- navigate to the libraqm source code directory.
3- create a new directory for building in side source code directory , and name it build


    	mkdir build

4-  then 

    	cd build
5-  then run cmake command to make building instructions:

    	cmake ..
   
   if you want to enable debug mode, unit testing & static build use:

    	cmake -DUNIT_TEST=1 -DWANT_DEBUG=1 -DSTATIC_LIB=1
    	
6- the run build command:
   
	# build the library for Release and Debug
    	cmake --build . --config Debug
 
7- if you enable unit tests option, you can run tests:

    	ctest -C "Debug"
    