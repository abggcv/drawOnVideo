#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace std;
using namespace cv;

static void help()
{
    cout << "\nTO READ FROM VIDEO FILE: test_project.exe <path\to\video\>" << endl;
	cout << "\nTO READ FROM WEBCAM: test_project.exe " << endl;
}

const Scalar RED = Scalar(0,0,255);
const Scalar PINK = Scalar(230,130,255);
const Scalar BLUE = Scalar(255,0,0);
const Scalar LIGHTBLUE = Scalar(255,255,160);
const Scalar GREEN = Scalar(0,255,0);

const int BGD_KEY = EVENT_FLAG_CTRLKEY;
const int FGD_KEY = EVENT_FLAG_SHIFTKEY;

static void getBinMask( const Mat& comMask, Mat& binMask )
{
    if( comMask.empty() || comMask.type()!=CV_8UC1 )
        CV_Error( Error::StsBadArg, "comMask is empty or has incorrect type (not CV_8UC1)" );
    if( binMask.empty() || binMask.rows!=comMask.rows || binMask.cols!=comMask.cols )
        binMask.create( comMask.size(), CV_8UC1 );
    binMask = comMask & 1;
}

class GCApplication
{
public:
    enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
    static const int radius = 2;
    static const int thickness = -1;

    void reset();
    void setImageAndWinName( const Mat& _image, const string& _winName );
    void showImage() const;
    void mouseClick( int event, int x, int y, int flags, void* param );
    int nextIter();
    int getIterCount() const { return iterCount; }
private:
    void setRectInMask();
    void setLblsInMask( int flags, Point p, bool isPr );
	//void findBoundingBox();
    const string* winName;
	string winName2;
    const Mat* image;
    Mat mask, mask2;
    Mat bgdModel, fgdModel;

    uchar rectState, lblsState, prLblsState;
    bool isInitialized;

    Rect rect,  boundingBox;
    vector<Point> fgdPxls, bgdPxls, prFgdPxls, prBgdPxls;
    int iterCount;
};

void GCApplication::reset()
{
    if( !mask.empty() )
        mask.setTo(Scalar::all(GC_BGD));
    bgdPxls.clear(); fgdPxls.clear();
    prBgdPxls.clear();  prFgdPxls.clear();

    isInitialized = false;
    rectState = NOT_SET;
    lblsState = NOT_SET;
    prLblsState = NOT_SET;
    iterCount = 0;
}

void GCApplication::setImageAndWinName( const Mat& _image, const string& _winName  )
{
    if( _image.empty() || _winName.empty() )
        return;
    image = &_image;
    winName = &_winName;
	winName2 = "window2";
    mask.create( image->size(), CV_8UC1);
	mask2 = Mat::zeros( image->size(), CV_8UC1);
    reset();
}

void GCApplication::showImage() const
{
    if( image->empty() || winName->empty() )
        return;

    Mat res = Mat(image->size(), image->type(), Scalar(255, 255, 255));
    Mat binMask;
    if( !isInitialized )
        image->copyTo( res );
    else
    {
        getBinMask( mask, binMask );
        image->copyTo( res, binMask );
    }

	if( rectState == IN_PROCESS || rectState == SET || rect.width > 0)
	{
		
		Mat res2;		
		image->copyTo( res2, mask2);
		imshow(winName2, res2(rect));

		rectangle( res, Point( rect.x, rect.y ), Point(rect.x + rect.width, rect.y + rect.height ), GREEN, 2);
	}

    vector<Point>::const_iterator it;
    for( it = bgdPxls.begin(); it != bgdPxls.end(); ++it )
        circle( res, *it, radius, BLUE, thickness );
    /*for( it = fgdPxls.begin(); it != fgdPxls.end(); ++it )
        circle( res, *it, radius, RED, thickness );
    for( it = prBgdPxls.begin(); it != prBgdPxls.end(); ++it )
        circle( res, *it, radius, LIGHTBLUE, thickness );
    for( it = prFgdPxls.begin(); it != prFgdPxls.end(); ++it )
        circle( res, *it, radius, PINK, thickness );*/

    imshow( *winName, res );
}

void GCApplication::setRectInMask()
{
    CV_Assert( !mask.empty() );
    mask.setTo( GC_BGD );
    rect.x = max(0, rect.x);
    rect.y = max(0, rect.y);
    rect.width = min(rect.width, image->cols-rect.x);
    rect.height = min(rect.height, image->rows-rect.y);
    (mask(rect)).setTo( Scalar(GC_PR_FGD) );
}

void GCApplication::setLblsInMask( int flags, Point p, bool isPr )
{
    vector<Point> *bpxls, *fpxls;
    uchar bvalue, fvalue;
    //if( !isPr )
    //{
        bpxls = &bgdPxls;
        //fpxls = &fgdPxls;
        bvalue = GC_BGD;
        //fvalue = GC_FGD;
    //}
    
    /*if( flags & BGD_KEY )
    {
        bpxls->push_back(p);
        circle( mask, p, radius, bvalue, thickness );
    }
    if( flags & FGD_KEY )
    {
        fpxls->push_back(p);
        circle( mask, p, radius, fvalue, thickness );
    }*/

	bpxls->push_back(p);
    circle( mask, p, radius, bvalue, thickness );

}

//void GCApplication::findBoundingBox()
//{
//
//}



void GCApplication::mouseClick( int event, int x, int y, int flags, void* )
{
    // TODO add bad args check
    switch( event )
    {
    case EVENT_LBUTTONDOWN: // set rect or GC_BGD(GC_FGD) labels
        {
            /*bool isb = (flags & BGD_KEY) != 0,
                 isf = (flags & FGD_KEY) != 0;
            if( rectState == NOT_SET && !isb && !isf )
            {
                rectState = IN_PROCESS;
                rect = Rect( x, y, 1, 1 );
            }
            if ( (isb || isf) && rectState == SET )*/
			if(lblsState != SET)
				lblsState = IN_PROCESS;
        }
        break;
    //case EVENT_RBUTTONDOWN: // set GC_PR_BGD(GC_PR_FGD) labels
    //    {
    //        bool isb = (flags & BGD_KEY) != 0,
    //             isf = (flags & FGD_KEY) != 0;
    //        if ( (isb || isf) && rectState == SET )
    //            prLblsState = IN_PROCESS;
    //    }
    //    break;
    case EVENT_LBUTTONUP:
        if( rectState == IN_PROCESS )
        {
            rect = Rect( Point(rect.x, rect.y), Point(x,y) );
            rectState = SET;
            setRectInMask();
            CV_Assert( bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty() );
            showImage();
        }
        if( lblsState == IN_PROCESS )
        {
            setLblsInMask(flags, Point(x,y), false);
            lblsState = SET;
			rect = boundingRect(bgdPxls);
			cout << "rectangle found: " << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << endl;
			setRectInMask();
			rectState == SET;
			//mask2 = Mat::zeros(rect.height, rect.width, CV_8UC1);
            vector<vector<Point>> conts;
			conts.push_back(bgdPxls);
			drawContours(mask2, conts, 0, Scalar(255), -1);			
			showImage();
        }
        break;
    /*case EVENT_RBUTTONUP:
        if( prLblsState == IN_PROCESS )
        {
            setLblsInMask(flags, Point(x,y), true);
            prLblsState = SET;
            showImage();
        }
        break;*/
    case EVENT_MOUSEMOVE:
        if( rectState == IN_PROCESS )
        {
            rect = Rect( Point(rect.x, rect.y), Point(x,y) );
            CV_Assert( bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty() );
            showImage();
        }
        else if( lblsState == IN_PROCESS )
        {
            setLblsInMask(flags, Point(x,y), false);
            showImage();
        }
        else if( prLblsState == IN_PROCESS )
        {
            setLblsInMask(flags, Point(x,y), true);
            showImage();
        }
        break;
    }
}

int GCApplication::nextIter()
{
    if( isInitialized )
        grabCut( *image, mask, rect, bgdModel, fgdModel, 1 );
    else
    {
        if( rectState != SET )
            return iterCount;

        if( lblsState == SET || prLblsState == SET )
            grabCut( *image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK );
        else
            grabCut( *image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_RECT );

        isInitialized = true;
    }
    iterCount++;

    bgdPxls.clear(); fgdPxls.clear();
    prBgdPxls.clear(); prFgdPxls.clear();

    return iterCount;
}

GCApplication gcapp;

static void on_mouse( int event, int x, int y, int flags, void* param )
{
    gcapp.mouseClick( event, x, y, flags, param );
}

int main( int argc, char** argv )
{    
	VideoCapture cap;

	if(argc < 2)
		cap.open(0); // open the default camera
	else
		cap.open(argv[1]);

    if(!cap.isOpened()) { // check if we succeeded
        cout << "NO camera/video file found \n";
		help();
        return -1;
    }	

    const string winName = "Original";
    namedWindow( winName, WINDOW_AUTOSIZE );
    setMouseCallback( winName, on_mouse, 0 );    

	Mat image; // = imread( filename, 1 );

	cap >> image;

	if( image.empty() )
    {
        cout << "\n Durn, couldn't read ifrom camera " << endl;
        return 1;
    }
	
	gcapp.setImageAndWinName( image, winName );
	gcapp.showImage();

    for(;;)
    {
		cap >> image;
		
		if(image.empty())
		{
			cout << "Exiting ... " << endl;
			goto exit_main;
		}
		
		//gcapp.setImageAndWinName( image, winName );
		gcapp.showImage();

        int c = waitKey(1);
        switch( (char) c )
        {
        case '\x1b':
            cout << "Exiting ..." << endl;
            goto exit_main;
        /*case 'r':
            cout << endl;
            gcapp.reset();
            gcapp.showImage();
            break;
        case 'n':
            int iterCount = gcapp.getIterCount();
            cout << "<" << iterCount << "... ";
            int newIterCount = gcapp.nextIter();
            if( newIterCount > iterCount )
            {
                gcapp.showImage();
                cout << iterCount << ">" << endl;
            }
            else
                cout << "rect must be determined>" << endl;*/
            break;
        }

		
    }

exit_main:
    //destroyWindow( winName );
	//destroyWindow( &winName2 );
	destroyAllWindows;
    return 0;
}

