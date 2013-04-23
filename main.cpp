#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <highgui.h>


using namespace cv;
using namespace std;

#define LEFT 0
#define RIGHT 1

//#define LEFT_MIN_ANGLE  (-90)
//#define LEFT_MAX_ANGLE  (-20)
//#define RIGHT_MIN_ANGLE (20)
//#define RIGHT_MAX_ANGLE (90)

#define THRESHOLD 75
#define FADE_TIME 10

Vec4i linefilter(vector<Vec4i> lines, int side);
int findcenter(Vec4i leftline, Vec4i rightline, int imgwidth);


// Hough Transform parameters
int hough_threshold = 120;
int minLineLength = 50;
int maxLineGap = 220;

// Canny Edge Detection parameters
int threshold1 = 100;
int threshold2 = 200;
int blur_block_size = 0;

// Angle and searchspace parameters
int min_angle = 12;
int max_angle = 80;
int search_height = 25;


int main(int argc, char** argv)
{
    Mat src, greyscale, dst, src2, src3;
    Mat leftclone, rightclone, left, right;
    vector<Vec4i> lines;
    int fps = 15;
    
    // Persistance variables
    int counter_left = FADE_TIME+1, counter_right = FADE_TIME+1;
    Vec4i leftline_old, rightline_old;
    
    // Parameter window intitialization
    namedWindow( "Hough Parameters", 0 );
    cvResizeWindow( "Hough Parameters", 500, 40 );
    createTrackbar( "Threshold", "Hough Parameters", &hough_threshold, 200, NULL);
    createTrackbar( "Minimum Line Length", "Hough Parameters", &minLineLength, 150, NULL);
    createTrackbar( "Maximum Line Gap", "Hough Parameters", &maxLineGap, 300, NULL);
    
    // Parameter window intitialization
    namedWindow( "Canny Parameters", 0 );
    cvResizeWindow( "Canny Parameters", 500, 40 );
    createTrackbar( "Blurring", "Canny Parameters", &blur_block_size, 4, NULL);
    createTrackbar( "Threshold 1", "Canny Parameters", &threshold1, 300, NULL);
    createTrackbar( "Threshold 2", "Canny Parameters", &threshold2, 300, NULL);
    
    // Parameter window initialization
    namedWindow( "Angle searchspace", 0 );
    cvResizeWindow( "Angle searchspace", 500, 40 );
    createTrackbar( "Min Angle", "Angle searchspace", &min_angle, 90, NULL);
    createTrackbar( "Max Angle", "Angle searchspace", &max_angle, 90, NULL);
    createTrackbar( "Search Height", "Angle searchspace", &search_height, 100, NULL);

    
    // Statistics variables
    double ratio_2line = 0.0;
    double ratio_1line = 0.0;
    double ratio_0line = 0.0;
    double ratio_danger = 0.0;
    
    
    // Display variables
    int center = 0;
    int center_old=0;
    int distance_old = 0;
    int distance = 0;
    char distance_str[100];
    char attention[100] = "DANGER!";
    char filename[50] = "video.avi";
    char hough_param_str[100];
    char canny_param_str[100];
    char angle_str[100];

    Point distance_pt = cvPoint(50, 460);
    Point attention_pt = cvPoint(450,460);
    Point hough_param_pt = cvPoint(20, 10);
    Point canny_param_pt = cvPoint(20, 25);
    Point angle_pt = cvPoint(20, 40);

    // Start video capture
    VideoCapture capture(0);
    if( !capture.isOpened() )
        return -1;

    VideoWriter writer("1.avi", CV_FOURCC('M','J','P','G'), fps, cvSize( 640, 480 ));
    
    while(1){
 
        // Get frame and convert to greyscale
        capture >> src2;
        resize(src2, src, Size(), 0.5, 0.5, INTER_LINEAR);
        cvtColor( src, greyscale, CV_RGB2GRAY );

        // Perform canny edge detection
        // < Low threshold = not an edge
        // > High threshold = edge
        // between thresholds = edge if next to edge
        if(blur_block_size != 0)
            blur(greyscale, src3, Size(2*blur_block_size+1,2*blur_block_size+1));
        else
            src3 = greyscale;
        Canny(src3, dst, threshold1, threshold2, 3);
        
        // Afficher la ligne delimitant la zone de recherche
        line(src, Point(0, search_height*src.rows/100), Point(src.cols, search_height*src.rows/100), Scalar(150,100,100), 1, CV_AA);
        
        // Diviser l'image en deux (left and right)
        leftclone = dst(Range(search_height*dst.rows/100,dst.rows),Range(0,0.5*dst.cols));           // Copies header
        left = leftclone.clone();                                           // Copies data
        rightclone = dst(Range(search_height*dst.rows/100,dst.rows),Range(0.5*dst.cols,dst.cols));   // Copies header
        right = rightclone.clone();                                         // Copies data

        // LEFT SIDE (Hough transform and linefilter)
        HoughLinesP(left, lines, 1, CV_PI/180, hough_threshold, minLineLength, maxLineGap);
        Vec4i leftline = linefilter(lines, LEFT);
        if( leftline[0] != -1 )         // Check if line is detected
        {
            line( src, Point(leftline[0], leftline[1]+search_height*src.rows/100), Point(leftline[2], leftline[3]+search_height*src.rows/100), Scalar(0,255,0), 7, CV_AA); // draws best line to source
            leftline_old = leftline;
            counter_left = 0;
        }
        // If line is not detected, slowly fade the previously detected line away
        else if( counter_left <= FADE_TIME )
        {
            counter_left++;
            line( src, Point(leftline_old[0], leftline_old[1]+search_height*src.rows/100), Point(leftline_old[2], leftline_old[3]+search_height*src.rows/100), Scalar(0,255-counter_left*255/FADE_TIME,counter_left*255/FADE_TIME), 7, CV_AA); // draws best line to source
        }

        // RIGHT SIDE (Hough transform and linefilter)
        HoughLinesP(right, lines, 1, CV_PI/180, hough_threshold, minLineLength, maxLineGap);
        Vec4i rightline = linefilter(lines, RIGHT);
        if( rightline[0] != -1 )        // Check if line is detected
        {
            line( src, Point(rightline[0]+0.5*src.cols, rightline[1]+search_height*src.rows/100), Point(rightline[2]+0.5*src.cols, rightline[3]+search_height*src.rows/100), Scalar(0,255,0), 7, CV_AA); // draws best line to source
            rightline_old = rightline;
            counter_right = 0;
        }
        // If line is not detected, slowly fade the previously detected line away
        else if(counter_right <= FADE_TIME )
        {
            counter_right++;
            line( src, Point(rightline_old[0]+0.5*src.cols, rightline_old[1]+search_height*src.rows/100), Point(rightline_old[2]+0.5*src.cols, rightline_old[3]+search_height*src.rows/100), Scalar(0,255-counter_right*255/FADE_TIME,counter_right*255/FADE_TIME), 7, CV_AA); // draws best line to source
            
        }

        // Add center lines
        center = findcenter(leftline, rightline, src.cols);
        distance = (center - 0.5*src.cols);
        if( leftline[0] != -1 && rightline[0] != -1 )   // Check if both lines are detected
        {
            line( src, Point(center, search_height*src.rows/100), Point(center, src.rows), Scalar(255,0,0), 2, CV_AA);
        
            center_old = center;
            distance_old = distance;
        }
        else if(counter_left <= FADE_TIME && counter_right <= FADE_TIME)
        {
            line( src, Point(center_old, search_height*src.rows/100), Point(center_old, src.rows), Scalar(255,0,0), 2, CV_AA);
        }

        // Write text to image
        sprintf(distance_str, "distance to center: %d", distance);
        putText(src,distance_str,distance_pt,FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255));
        if ( fabs(distance_old) > THRESHOLD || (counter_right > FADE_TIME) || (counter_left > FADE_TIME)) 
        {
            putText(src,attention,attention_pt,FONT_HERSHEY_SIMPLEX,1,CV_RGB(255,0,0));
            ratio_danger += 1.0;
        }
        
        // Write Hough Transform parameters to image
        sprintf(hough_param_str, "Hough params: T = %d  Min Ln Lgth = %d  Max Ln Gap = %d", hough_threshold, minLineLength, maxLineGap);
        putText(src,hough_param_str,hough_param_pt,FONT_HERSHEY_SIMPLEX,0.3,CV_RGB(0,0,255));
        
        // Write Canny Edge Detection parameters to image
        sprintf(canny_param_str, "Canny params: T1 = %d  T2 = %d  Blur Matrix %dx%d", threshold1, threshold2, 2*blur_block_size+1, 2*blur_block_size+1);
        putText(src,canny_param_str,canny_param_pt,FONT_HERSHEY_SIMPLEX,0.3,CV_RGB(0,0,255));
        
        // Write angle searchspace to image
        sprintf(angle_str, "Search angles: min = %d  max = %d", min_angle, max_angle);
        putText(src,angle_str,angle_pt,FONT_HERSHEY_SIMPLEX,0.3,CV_RGB(0,0,255));
        

        // Write source to video file
        writer << src;

        imshow("src", src);
        imshow("left", left);
        imshow("right", right);
        //imshow("rblurry", src3);
        
        
        if( leftline[0] != -1 && rightline[0] != -1 )
            ratio_2line += 1.0;
        else if ( leftline[0] == -1 && rightline[0] == -1 )
            ratio_0line += 1.0;
        else
            ratio_1line += 1.0;

        if(waitKey(30) >= 0) break;
    }
    
    double total = ratio_0line + ratio_1line + ratio_2line;
    ratio_0line *= 100.0/total;
    ratio_1line *= 100.0/total;
    ratio_2line *= 100.0/total;
    ratio_danger *= 100.0/total;
    
    printf("\n2 lines : %lf percent \n1 line : %lf percent \n0 lines : %lf percent\ndanger : %lf percent\ntotal sample : %lf\n\n\n", ratio_2line, ratio_1line, ratio_0line,ratio_danger,total);

    return 0;
}



Vec4i linefilter(vector<Vec4i> lines, int side)
{
    vector<Vec4i> candidateLines;
    float angle, max_length, length;
    int longest;
    Vec4i error;
    error[0] = error[1] = error[2] = error[3] = -1;
    
    // Keep the lines that are in the angle acceptance window
    // LEFT  : 0 to 45 deg
    // RIGHT : -45 to 0 deg
    for( size_t i = 0; i < lines.size(); i++ ){
        Vec4i l = lines[i];
    
        // Calculate angle of vector (in degs), and convert it to keep it in the -PI/2 -to PI/2 range
        angle = atan2( l[3]-l[1], l[2]-l[0] ) * 180/M_PI;
        if( angle > 90 )
            angle -= 180;
        else if( angle < -90 )
            angle += 180;
        
        // If the angle is within the desired range, insert Vec4i in candidate vector
        if( side == LEFT && angle >= -max_angle && angle <= -min_angle )
            candidateLines.push_back(l);
        else if( side == RIGHT && angle >= min_angle && angle <= max_angle )
            candidateLines.push_back(l);
    }
    
    // If no lines were in the acceptance window, stop here
    if( candidateLines.empty() )
        return error;
    
    // Compute length of first line of the candidateLines vector and set it as default maximum
    max_length = pow(lines[0][3] - lines[0][1], 2.0) + pow(lines[0][2] - lines[0][0], 2.0);
    longest = 0;
    
    // Search among the candidate lines for the longest line
    // The square of the euclidian distances is calculated in order to reduce the computional needs
    for( size_t i = 0; i < candidateLines.size(); i++ )
    {
        Vec4i l = candidateLines[i];
        length = pow(lines[i][3] - lines[i][1], 2.0) + pow(lines[i][2] - lines[i][0], 2.0);
        
        if( length > max_length )
        {
            max_length = length;
            longest = i;
        }
    }
    
    return candidateLines[longest];
}


int findcenter(Vec4i lineleft, Vec4i lineright, int imgwidth)
{    
    return (lineleft[0] + lineleft[2] + lineright[0] + lineright[2] + imgwidth)/4;
}


