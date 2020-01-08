// Standard IO stream
#include <iostream>

// OpenCV core: Fast matrix/image manipulation
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// OpenCV GUI functions
#include <opencv2/highgui/highgui.hpp>

using namespace cv; 
using namespace std;

// Struct for drawing settings passed to OpenCV functions
struct drawing_settings 
{
	const char* win = "CV_Window_"; // Window title / ID
	bool mouse_down = false; // Mouse pressed?
	int prev_x = 0; // Previous mouse x pos
	int prev_y = 0; // Previous mouse y pos
	Mat input_img; // Original (i.e. non-transformed) image data painted by user

	Rect button; // Erase button
	bool button_pressed = false;
};

// Compute Fourier transform and show image
void redresh_drawing(drawing_settings *data)
{
	// Create 2-channel img with first channel containing the input image and second one empty (black)
	Mat composite;
	Mat channels[] = {Mat_<float>(data->input_img), Mat::zeros(data->input_img.size(), CV_32F)};
	merge(channels, 2, composite);

	// Perform the Fourier transform in complex mode
	Mat transformed;
	dft(composite, transformed);

	// Get the real and imaginary component of the Fourier transform from the channels
	// and compute magnitude: mag_img = sqrt(Re(DFT(I))^2 + Im(DFT(I))^2)
	Mat components[2], mag_img;
	split(transformed, components);
	magnitude(components[0], components[1], mag_img);

	// Switch to logarithmic scale
	mag_img += Scalar::all(1);
	log(mag_img, mag_img);

	// Crop the spectrum if it has an odd number of rows or columns
	mag_img = mag_img(Rect(0, 0, mag_img.cols & -2, mag_img.rows & -2));

	// Re-arrange the quadrants of Fourier image  so that the origin is at the image center
	int cx = mag_img.cols/2;
	int cy = mag_img.rows/2;

	// Create regions of interest for the quadrants
	Mat q1(mag_img, Rect(0, 0, cx, cy)); // top left quadrant
	Mat q2(mag_img, Rect(cx, 0, cx, cy)); // top right quadrant
	Mat q3(mag_img, Rect(0, cy, cx, cy)); // btm left quadrant
	Mat q4(mag_img, Rect(cx, cy, cx, cy)); // btm right quadrant
	
	// Swap top-left with btm-right
	Mat tmp1, tmp2;
	q1.copyTo(tmp1);
	q4.copyTo(tmp2);
	tmp1.copyTo(q4);
	tmp2.copyTo(q1);

	// Swap top-right with btm-left
	q2.copyTo(tmp1);
	q3.copyTo(tmp2);
	tmp1.copyTo(q3);
	tmp2.copyTo(q2);

	// Normalize the image to [0,1] in order to make it viewable
	normalize(mag_img, mag_img, 0, 1, NORM_MINMAX); 

	// Display explanatory text on top of the final images 
	// (using a copy of "input_img" to avoid having the text Fourier transformed as well)
	Mat input_img_copy = data->input_img.clone();
	int font = FONT_HERSHEY_SIMPLEX;
	putText(input_img_copy, "Draw here (pencil):", {20, 30}, font, 0.7, Scalar::all(0.9), 2, 16);
	putText(input_img_copy, "Draw here (pencil):", {20, 30}, font, 0.7, Scalar::all(0), 1, 16);
	putText(mag_img, "Fourier magnitude:", {20, 30}, font, 0.7, Scalar::all(0.3), 3, 16);
	putText(mag_img, "Fourier magnitude:", {20, 30}, font, 0.7, Scalar::all(1), 1, 16);

	// Concatenate the two images in vertical direction
	Mat both_img;
	vconcat(input_img_copy, mag_img, both_img);

	// Show "erase" button
	data->button = Rect(2, both_img.rows+2, both_img.cols/2-4, 46);
	Rect button_bar(0, both_img.rows, both_img.cols, 50);

	// Define a canvas slightly larger than the image; and put the image on it
	Mat canvas = Mat::ones(both_img.rows + button_bar.height, both_img.cols, CV_32FC1);
	both_img.copyTo(canvas(Rect(0, 0, both_img.cols, both_img.rows)));

	// On the remaining area, draw the button
	string button_str = "Start over";
	if (data->button_pressed)
	{
		canvas(data->button) = Scalar::all(0.6);
		putText(canvas(data->button), button_str, Point(data->button.width*0.35, data->button.height*0.7), FONT_HERSHEY_SIMPLEX, 0.7, Scalar::all(0.5), 2, 16);
		putText(canvas(data->button), button_str, Point(data->button.width*0.35, data->button.height*0.7), FONT_HERSHEY_SIMPLEX, 0.7, Scalar::all(0), 1, 16);

	}
	else
	{
		canvas(data->button) = Scalar::all(0.8);
		putText(canvas(data->button), button_str, Point(data->button.width*0.35, data->button.height*0.7), FONT_HERSHEY_SIMPLEX, 0.7, Scalar::all(0.7), 2, 16);
		putText(canvas(data->button), button_str, Point(data->button.width*0.35, data->button.height*0.7), FONT_HERSHEY_SIMPLEX, 0.7, Scalar::all(0), 1, 16);
	}
	

	// Show everything and resize the screen to the image
	imshow(data->win, canvas);
	resizeWindow(data->win, canvas.cols, canvas.rows);

}

// Handle incoming mouse events: Draw circles and lines when mouse pressed down
void mouse_callback_canvas(int event, int x, int y, int, void* screen_data)
{
	// Get data pushed from main()
	drawing_settings *screen = (drawing_settings*)screen_data;

	// If dragging is active (i.e. while mouse button is being pressed down), shift the lens
	if (screen->mouse_down and event == EVENT_MOUSEMOVE)
	{
		Point prev_coord(screen->prev_x, screen->prev_y);
		Point new_coord(x,y);
		line(screen->input_img, prev_coord, new_coord, Scalar::all(0), 2);
	
	}

	// On mouse up: Disable dragging mode and finally update trackbar slider
	else if (event == EVENT_LBUTTONUP)
	{
		screen->mouse_down = false;
		screen->button_pressed = false;
	}

	// On mouse down: Re-render and enable dragging mode
	else if (event == EVENT_LBUTTONDOWN)
	{
		screen->mouse_down = true;
		circle(screen->input_img, Point(x, y), 1, Scalar::all(0), -1);
		if (screen->button.contains(Point(x, y)))
		{
			screen->button_pressed = true;
			screen->input_img = Mat::ones(screen->input_img.rows, screen->input_img.cols, CV_32FC1);
		}
	}

	// Update mouse coordinate and refresh drawing
	screen->prev_x = x;
	screen->prev_y = y;
	redresh_drawing(screen);

}

int main(int argc, char** argv)
{
	// Create blank image
	Mat sourceImg(400, 900, CV_32FC1, Scalar::all(1));

	// Comment out to load an image
	/*
	sourceImg = imread("file.png", IMREAD_GRAYSCALE);
	if (!sourceImg.data)
        {
                cout << "Error opening the image file..." << endl;
                return 0;
        }
	sourceImg.convertTo(sourceImg, CV_32F);
	normalize(sourceImg, sourceImg, 1, 0, NORM_MINMAX);
	*/
	
	size_t sourceWidth = sourceImg.cols;
	size_t sourceHeight = sourceImg.rows;
	size_t optWidth = getOptimalDFTSize(sourceImg.cols);
	size_t optHeight = getOptimalDFTSize(sourceImg.rows);
	Mat enlarged;
	copyMakeBorder(sourceImg, enlarged, 0, optHeight - sourceHeight, 0, optWidth - sourceWidth, BORDER_CONSTANT, Scalar::all(0));

	drawing_settings data;
	data.input_img = enlarged;

	namedWindow(data.win, WINDOW_NORMAL);
	setMouseCallback(data.win, mouse_callback_canvas, &data);
	redresh_drawing(&data);

	// As long as window opened
	while (waitKey(0) != 113 and !(getWindowProperty(data.win, WND_PROP_AUTOSIZE) == -1))
	{};
	destroyAllWindows();

	return 0;

}
