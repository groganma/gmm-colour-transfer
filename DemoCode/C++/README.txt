%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
Author: Mairead Grogan
Contact: mgrogan@tcd.ie
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

The folder CTCode contains the C++ code for the paper 'L2 Divergence for robust colour transfer' published in Computer Vision and Image Understanding 2019.

It depends on the C++ library 'gmmreg' created by Jian et al. and provided at the following link:
https://github.com/bing-jian/gmmreg
The gmmreg library has been provided along with this code. 

Therefore, if using this colour transfer code please cite the following papers:

1.'L2 Divergence for robust colour transfer'
 Mairead Grogan and Rozenn Dahyot
 Computer Vision and Image Understanding 2019


2. A Robust Algorithm for Point Set Registration Using Mixture of Gaussians,
 Bing Jian and Baba C. Vemuri,
 10th IEEE International Conference on Computer Vision (ICCV 2005),
 17-20 October 2005, Beijing, China, pp. 1246-1251.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
HOW TO USE CODE:

1. Dependencies: This code depends on the OpenCV (https://opencv.org/) and VXL (https://github.com/vxl/vxl) libraries. These should be installed before using this code. It also depends on OpenMP, which is used in the 
file 'TryJianAndOpenCV.cpp'. The OpenMP dependency can be removed by removing '#include <omp.h>' in line 6 of 'TryJianAndOpenCV.cpp' and removing lines 85, 94 and 95. 



2. Compiling gmmreg: To compile gmmreg, you must first edit line 14 of the file CMakeLists.txt in CTCode/gmmreg/ so that it points to the VXL directory containing the file 'VXLConfig.cmake'.

Then run 'cmake' in the gmmreg directory to generate the make file, and 'make' to generate the library files, in particular libgmmreg_api.so.



3. Compiling the final executable colour_transfer.x: When the gmmreg library is compiled, edit the 'Makefile' in CTCode/ so that it is pointing to the correct library directories:
-In line 4 of CTCode/Makefile edit the path so that it points to the directory containing the OpenCV library. 
-In line 5 of CTCode/Makefile edit the path so that it points to the directory containing the CTCode folder. 

Then the paths in line 6 and 7 should correctly point to directories in the OpenCV library, and the paths in lines 8 and 9 point to the gmmreg folder within CTCode. 
Note that in line 3 we also define the path in which the libgmmreg_api.so can be found with -Wl,-rpath=gmmreg 

In TryJianAndOpenCV.cpp:
Change the paths in lines 169 - 175 from '/home/mairead/Code/ColourTransfer/' to the local directory of the folder CTCode.

Then run 'make' in the CTCode directory to compile the executable 'colour_transfer.x'



4. Running colour_transfer.x:

For IMAGE recolouring run the command 
./colour_transfer.x <target image name> <palette image name> <result image name>

eg. 
./colour_transfer.x parrot-1.jpg parrot-2.jpg result.jpg 

The TARGET image is the input image that is to be recoloured. The PALETTE image is the image with the desired colour palette. These colours will be mapped onto the target image.

For VIDEO recolouring run the command: 

./colour_transfer.x <target video frame> <palette image name> <target video file > <result image name>

eg. 
./colour_transfer.x video_frame.jpg palette.jpg video.avi result.avi

The TARGET VIDEO FRAME is a frame from the video to be recoloured, it is used to compute the colour transformation that will be applied to the video. The PALETTE image is the image with the desired colour palette.
The VIDEO file will be recoloured using the transformation estimated between the target video frame and the palette image. 


