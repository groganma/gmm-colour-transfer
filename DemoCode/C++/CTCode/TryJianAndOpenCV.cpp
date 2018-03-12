#include <opencv/cv.h> //new
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <omp.h>
#include <math.h>
#include <fstream>

#include "gmmreg_api.h"

using namespace cv;
using namespace std;

void writeMatToFile(cv::Mat& m, const char* filename) //this function writes a mat to file
{
	ofstream fout(filename);

	if (!fout)
	{
		cout << "File Not Opened" << endl;  return;
	}

	for (int i = 0; i < m.rows; i++)
	{
		for (int j = 0; j < m.cols; j++)
		{
			fout << m.at<float>(i, j) << "\t";
		}
		fout << endl;
	}

	fout.close();
}

Mat ReadMatFromTxt(string filename, int rows) //this function reads a mat from a .txt file
{
	ifstream in(filename.c_str());
	vector<float> nums;
	while (in.good()){
		float n;
		in >> n;
		if (in.eof()) break;
		nums.push_back(n);
	}
	// now make a Mat from the vector:
	Mat mat(nums);
	cv::Mat mat_re = mat.reshape(1, rows); //reshape it so it has the correct number of rows/cols
	Mat mat_clone = mat_re.clone(); // must clone it to return it, other wise it return a pointer to the values nums
	return mat_clone;
}

Mat findClusters(Mat src, int num_clusters)//find k-means clusters
{
	Mat rsz_src;
	Mat flt_rshp_src;
	Mat labels;
	Mat centers;
	const int kNewWidth = 50;
	const int kNewHeight = 100;
	int attempts = 5;

	resize(src, rsz_src, cvSize(kNewWidth, kNewHeight)); //resize input for kmeans
	Mat rshp_src = rsz_src.reshape(1, kNewWidth*kNewHeight); //reshape so of size nx3
	rshp_src.convertTo(flt_rshp_src, CV_32F); //convert to float

	kmeans(flt_rshp_src, num_clusters, labels, TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.0001), attempts, KMEANS_PP_CENTERS, centers); //apply k-means
	
	return centers;
}

Mat recolourImage(Mat A, Mat pix, Mat Nv, Mat ctrl)
{

	int num_pix, num_ctrl;
	//figure out dimensions   
	num_pix = pix.rows;
	num_ctrl = ctrl.rows;
	Mat recolour(num_pix, 3, CV_32F);
	Mat recolour2(num_pix, 3, CV_32F);

	long int i,k;
	int j;
	double norm;
	int nThreads = omp_get_max_threads();
	float* A0 = A.ptr<float>(0); 
	float* A1 = A.ptr<float>(1);
	float* A2 = A.ptr<float>(2);
	float* A3 = A.ptr<float>(3);
	float* ctrl_ptr;
	float* Nv_ptr;
	float* pix_ptr;
	float* recolour_ptr;
	cout << "Number of threads used for recolouring in parallel:" << nThreads << endl;
	#pragma omp parallel for shared(pix, A0, A1, A2, A3, Nv, ctrl, num_ctrl, num_pix) private(j,k,norm,pix_ptr,recolour_ptr,ctrl_ptr,Nv_ptr)
	for (i = 0; i < num_pix; i++)
	{
		pix_ptr = pix.ptr<float>(i);
		recolour_ptr = recolour.ptr<float>(i);

		for (k = 0; k < 3; k++)
		{
			recolour_ptr[k] = A0[k] + (A1[k] * pix_ptr[0]) + (A2[k] * pix_ptr[1]) + (A3[k] * pix_ptr[2]); // t+Ax1
		}

		for (j = 0; j < num_ctrl; j++)
		{
			ctrl_ptr = ctrl.ptr<float>(j);
			Nv_ptr = Nv.ptr<float>(j);
			norm = -sqrt((pix_ptr[0] - ctrl_ptr[0])*(pix_ptr[0] - ctrl_ptr[0]) + (pix_ptr[1] - ctrl_ptr[1])*(pix_ptr[1] - ctrl_ptr[1])+ (pix_ptr[2] - ctrl_ptr[2])*(pix_ptr[2] - ctrl_ptr[2])); //|| Xi - cj||
			for (k = 0; k < 3; k++)
			{
				recolour_ptr[k] += norm*(Nv_ptr[k]); //|| Xi - cj||*Nvj1
			}

		}
	}
	return recolour;
}

int main(int argc, char* argv[]) {
	//Init Mat
	Mat target, palette, centers_target, centers_palette;
	Mat A, Param, ctrl, PP, Nv;
	bool video_recolour;

	//Error messages if num arg wrong
	if (argc < 4)
	{
		std::cout << "Usage for image recoloring : ./colour_transfer.x <target image> <palette image> <destination video file>" << std::endl;
		std::cout << "Example : ./colour_transfer.x parrot-1.jpg parrot-2.jpg result.jpg" << std::endl;
		std::cout << "Usage for video recolouring : ./colour_transfer.x <target image (selected video frame)> <palette image> <target video file> <destination video file>" << std::endl;
		std::cout << "Example : ./colour_transfer.x video_frame.jpg palette.jpg video.avi result.avi" << std::endl;
		return 1;
	}

	//Image recolouring if 3 inputs given
	if (argc == 4)
	{
		std::cout << "Image recolouring started.. " << endl;
		video_recolour = 0;
	}

	//Video recolouring if 4 inputs given
	if (argc == 5)
	{
		std::cout << "Video recolouring started.. " << endl;
		video_recolour = 1;
	}

	//read in images 
	cout << "Reading in the target and palette images.. " << endl;
	target = imread(argv[1], 1);
	if (target.empty())                      // Check for invalid input
	{
		std::cerr << "Error: Could not open or find the target image" << std::endl;
		return -1;
	}
	palette = imread(argv[2], 1);
	if (palette.empty())                      // Check for invalid input
	{
		std::cerr << "Error: Could not open or find the palette image" << std::endl;
		return -1;
	}

	cout << "Done. " << endl;

	//Initialise the directories of files
	const char* colour_new_ini = "/home/mairead/Code/ColourTransfer/CTCode/colour_new.ini";
	const char* colour_X_new = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_X_new.txt";
	const char* colour_Y_new = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_Y_new.txt";
	const char* final_colour_affine = "/home/mairead/Code/ColourTransfer/CTCode/final_colour_affine.txt";
	const char* final_colour_tps = "/home/mairead/Code/ColourTransfer/CTCode/final_colour_tps.txt";
	const char* colour_ctrl_pts = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_ctrl_pts.txt";
	const char* PP_str = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/PP.txt";



	//find cluster centres
	cout << "Apply k-means to the colours in the target and palette images.. " << endl;
	centers_target = findClusters(target, 50); //find the k most dominant colours
	writeMatToFile(centers_target, colour_X_new);
	centers_palette = findClusters(palette, 50);//find the k most dominant colours
	writeMatToFile(centers_palette, colour_Y_new);
	cout << "Done. " << endl;


	//register GMMs
	cout << "Registering the colour distributions.. " << endl;
	gmmreg_api(colour_new_ini, "TPS_L2");

	//read in the pre set parameters needed to transform the pixels using the TPS transformation (ie PP, control points, estimates parameters (param and A)).
	cout << "Reading in variables from file for recolouring.. " << endl;
	A = ReadMatFromTxt(final_colour_affine, 4);
	Param = ReadMatFromTxt(final_colour_tps, 121);
	ctrl = ReadMatFromTxt(colour_ctrl_pts, 125);
	PP = ReadMatFromTxt(PP_str, 125);
	Nv = PP*Param;
	cout << "Done. " << endl;

	//Image recolouring
	if (video_recolour == 0) {

		cout << "Recolouring the target image in parallel.. " << endl;

		Mat flt_target, full_target, recolour, recolour_reshp, result_show;

		//reshape target image so can recolour
		target.convertTo(flt_target, CV_32F);
		full_target = flt_target.reshape(1, (target.rows)*(target.cols)); //reshape so of size nx3

																		  //recolour image
		recolour = recolourImage(A, full_target, Nv, ctrl); //full_target is the pixels, A is the affine transformation, Nv are the parameters, ctrl control points.
		recolour_reshp = recolour.reshape(3, (target.rows)); //reshape so of size nx3
		recolour_reshp.convertTo(result_show, CV_8UC1); //change format
		char* destination = argv[3];
		imwrite(destination, result_show);
	}

	//video recolouring
	else
	{
		//initialise some video variables 
		Mat frame, flt_frame, full_frame, recolour_frame, recolour_reshp, final_frame;
		double fps, frame_width, frame_height, num_frames;

		//load target video
		VideoCapture tar_vid(argv[3]);


		if (tar_vid.isOpened() == false)
		{
			std::cerr << "Error: Cannot open the video file" << endl;
			return 1;
		}

		//initialise some video parameters
		fps = tar_vid.get(CAP_PROP_FPS);
		frame_width = tar_vid.get(CV_CAP_PROP_FRAME_WIDTH);
		frame_height = tar_vid.get(CV_CAP_PROP_FRAME_HEIGHT);
		num_frames = tar_vid.get(CV_CAP_PROP_FRAME_COUNT);

		//recolour video
		VideoWriter video(argv[4], CV_FOURCC('M', 'J', 'P', 'G'), fps, Size(frame_width, frame_height));
		int count = 0;
		while (1)
		{

			count = count + 1;
			std::cout << "Processing frame number " << count << "/" << num_frames << ". ";
			// Capture target video frame-by-frame 
			tar_vid >> frame;

			// If the frame is empty, break immediately
			if (frame.empty())
				break;

			frame.convertTo(flt_frame, CV_32F);
			full_frame = flt_frame.reshape(1, (frame.rows)*(frame.cols));
			recolour_frame = recolourImage(A, full_frame, Nv, ctrl); //full_target is the pixels, A is the affine transformation, Nv are the parameters, ctrl control points.
			recolour_reshp = recolour_frame.reshape(3, (frame.rows)); //reshape so of size nx3
			recolour_reshp.convertTo(final_frame, CV_8UC1);

			// Write the frame into the file 'outcpp.avi'
			video.write(final_frame);

		}
		tar_vid.release();
		video.release();
	}

	cout << "Done. Results have been saved." << endl;
	return 1;


}
