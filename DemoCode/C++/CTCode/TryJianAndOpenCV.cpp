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
	Mat full_target, flt_target, recolour, recolour_reshp, result_show;
	Mat A, Param, ctrl, PP, Nv;

	if (argc < 4)
	{
		std::cerr << "Usage : ./colour_transfer.x <target image> <palette image> <destination>" << std::endl; 
		return 1;
	}

	//read in images
	cout << "Reading in the input images.. " << endl;
	target = imread(argv[1], 1);
	palette = imread(argv[2], 1);
	cout << "Done. " << endl;

	//Initialise the directories of files
	const char* colour_new_ini = "/home/mairead/Code/ColourTransfer/CTCode/colour_new.ini";
	const char* colour_X_new = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_X_new.txt";
	const char* colour_Y_new = "/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_Y_new.txt";
	const char* final_colour_affine ="/home/mairead/Code/ColourTransfer/CTCode/final_colour_affine.txt";
	const char* final_colour_tps ="/home/mairead/Code/ColourTransfer/CTCode/final_colour_tps.txt";
	const char* colour_ctrl_pts ="/home/mairead/Code/ColourTransfer/CTCode/colour_data/colour_ctrl_pts.txt";
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

	
	//reshape target so can recolour
	target.convertTo(flt_target, CV_32F);
	full_target = flt_target.reshape(1, (target.rows)*(target.cols)); //reshape so of size nx3

	//read in the pre set parameters needed to transform the pixels using the TPS transformation (ie PP, control points, estimates parameters (param and A)).
	cout << "Reading in variables from file for recolouring.. " << endl;
	A = ReadMatFromTxt(final_colour_affine, 4);
	Param = ReadMatFromTxt(final_colour_tps, 121);
	ctrl = ReadMatFromTxt(colour_ctrl_pts, 125);
	PP = ReadMatFromTxt(PP_str, 125);
	Nv = PP*Param;
	cout << "Done. " << endl;

	
	cout << "Recolouring the target image in parallel.. " << endl;
	//recolour image
	recolour = recolourImage(A, full_target, Nv, ctrl); //full_target is the pixels, A is the affine transformation, Nv are the parameters, ctrl control points.
	recolour_reshp = recolour.reshape(3, (target.rows)); //reshape so of size nx3
	recolour_reshp.convertTo(result_show, CV_8UC1); //change format
	char* destination = argv[3];
	imwrite( destination, result_show );
	cout << "Done. Result image has been saved." << endl;
	//imshow("result image", result_show); //show result
	//waitKey(0);

return 1;


}
