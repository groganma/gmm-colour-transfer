CC = g++ #-vt	Ichec Intel parallel compiler
CCFLAG = -O3 -g -W -Wall#-debug all # debug and optmization 
LIBFLAG = -fopenmp -lpthread -lopencv_core -lopencv_imgproc -lopencv_photo -lopencv_video -lopencv_objdetect -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio -lgmmreg_api -Wl,-rpath=gmmreg ## these are the libraries to link	
LIBS_DIR = /home/mairead/Code/Toolboxes/OpenCV
CT_DIR = /home/mairead/Code/ColourTransfer
OPENCVINCDIR = -I${LIBS_DIR}/opencv/include -I${LIBS_DIR}/opencv/modules/core/include -I${LIBS_DIR}/opencv/release -I${LIBS_DIR}/opencv/modules/imgproc/include -I${LIBS_DIR}/opencv/modules/photo/include -I${LIBS_DIR}/opencv/modules/video/include -I${LIBS_DIR}/opencv/modules/objdetect/include -I${LIBS_DIR}/opencv/modules/highgui/include -I${LIBS_DIR}/opencv/modules/imgcodecs/include -I${LIBS_DIR}/opencv/modules/videoio/include 
OPENCVLIBDIR = -L${LIBS_DIR}/opencv/release/lib 
GMMLIBDIR = -L${CT_DIR}/CTCode/gmmreg
GMMINCDIR = -I${CT_DIR}/CTCode/gmmreg
INCDIR = $(OPENCVINCDIR) $(GMMINCDIR) ### these are the direcotry additions necessary when using software on UBUNTU desktop
LIBDIR = $(OPENCVLIBDIR) $(GMMLIBDIR) # 

		
.PHONY: all

#########################
 
#default targets
all: colour_transfer.x 

colour_transfer.x: TryJianAndOpenCV.o
	$(CC) $(CCFLAG) TryJianAndOpenCV.o -o colour_transfer.x ${INCDIR} ${LIBDIR} $(LIBFLAG) 
	
##########################
	
TryJianAndOpenCV.o: TryJianAndOpenCV.cpp 
	$(CC) $(CCFLAG) -c TryJianAndOpenCV.cpp -o TryJianAndOpenCV.o ${INCDIR} ${LIBDIR} $(LIBFLAG)
	
##########################

#clean up
clean:
	rm *.x *.o
	
###########################
###########################

