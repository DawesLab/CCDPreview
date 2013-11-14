//Preview image and FFT data from a full-frame shot.
//The sample will open the first camera attached
//and acquire frames continuously until escaped

#define NUM_FRAMES  5
#define NO_TIMEOUT  -1

#include "stdio.h"
#include "picam.h"
#include <boost/program_options.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
namespace po = boost::program_options;

//TODO: put these Print functions into another file?

void PrintEnumString( PicamEnumeratedType type, piint value )
{
    const pichar* string;
    Picam_GetEnumerationString( type, value, &string );
    std::cout << string;
    Picam_DestroyString( string );
}


void PrintError (PicamError error)
{
    if ( error == PicamError_None )
	std::cout << "Succeeded" << std::endl;
    else
    {
        std::cout << "Failed (";
        PrintEnumString( PicamEnumeratedType_Error, error );
        std::cout << ")" << std::endl;
    }
}

Mat CollectShot(PicamHandle camera, PicamAvailableData data, PicamAcquisitionErrorsMask errors, bool verboseOutput)
{
	if (verboseOutput) std::cout << "Collecting 1 frame\n\n";
    if( Picam_Acquire( camera, 1, NO_TIMEOUT, &data, &errors ) )
        printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
    else
    {
    	if (verboseOutput) std::cout << "One frame collected\n";
    }
    
    Mat image = Mat(400,1340, CV_16U, data.initial_readout).clone();

    return image;
}

PicamHandle InitializeCamera (PicamCameraID id, PicamAvailableData data, PicamAcquisitionErrorsMask errors, bool verboseOutput)
{
	PicamHandle camera;

	if (verboseOutput) 
    	std::cout << "Initializing PIcam library\n";
    
    Picam_InitializeLibrary();

    // - open the first camera if any or create a demo camera



    if (verboseOutput) 
    	std::cout << "Opening camera...\n";

    const pichar* string;

    if( Picam_OpenFirstCamera( &camera ) == PicamError_None )
        Picam_GetCameraID( camera, &id );
    else
    {
    	printf( "Cannot load camera\n");
        // return(1);
        // TODO handle this the right way
    }
    Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
    printf( "%s", string );
    printf( " (SN:%s) [%s]\n", id.serial_number, id.sensor_name );
    Picam_DestroyString( string );

    return camera;
}

void ConfigureCamera (PicamHandle camera, bool verboseOutput)
{

    if (verboseOutput)
    	std::cout << "Configuring camera...\n";
    	std::cout << "Set ADC rate to 4 MHz: ";
    
    PicamError error;
    error = Picam_SetParameterFloatingPointValue(
                camera,
                PicamParameter_AdcSpeed,
                4.0 );
    PrintError( error );

    // if (verboseOutput)
    // 	std::cout << "Set exposure to triggered: ";

    // PicamTriggerResponse TriggerResponse =  PicamTriggerResponse_ExposeDuringTriggerPulse; 
    PicamTriggerDetermination TriggerDetermination = PicamTriggerDetermination_RisingEdge;

    // error = Picam_SetParameterIntegerValue(
    // 			camera,
    // 			PicamParameter_TriggerResponse,
    // 			TriggerResponse );
    // PrintError( error );

    if (verboseOutput)
    	std::cout << "Set trigger determination: ";

    error = Picam_SetParameterIntegerValue(
    			camera,
    			PicamParameter_TriggerDetermination,
    			TriggerDetermination );
    PrintError( error );

    
    pibln committed;
    Picam_AreParametersCommitted( camera, &committed );
    if( committed )
        if (verboseOutput)
    		std::cout << "Parameters have not changed" << std::endl;
    else
        if (verboseOutput)
    		std::cout << "Parameters have been modified" << std::endl;

    // apply changes to hardware
    if (verboseOutput)
    	std::cout << "Commit to hardware: ";
    const PicamParameter* failed_parameters;
    piint failed_parameters_count;
    error = 
        Picam_CommitParameters(
            camera,
            &failed_parameters,
            &failed_parameters_count );
    PrintError( error );

    if (verboseOutput)
    	std::cout << "Testing for invalid params\n";
    if( failed_parameters_count > 0 )
    {
        if (verboseOutput)
    		std::cout << "The following params are invalid:" << std::endl;
        for( piint i = 0; i < failed_parameters_count; ++i )
        {
            std::cout << "    ";
            PrintEnumString(
                PicamEnumeratedType_Parameter,
                failed_parameters[i] );
            std::cout << std::endl;
        }
    }
    if (verboseOutput)
    	std::cout << "Cleaning up resources\n";

    Picam_DestroyParameters( failed_parameters );
}

int main(int ac, char* av[])
{
	// Declare the supported command-line options.
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "explain each step")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
	    std::cout << desc << "\n";
	    return 1;
	}

	// Choose verbosity
	bool verboseOutput = false;
	if (vm.count("verbose")) {
	    std::cout << "Verbose output.\n";
		verboseOutput = true;
	} else {
	    std::cout << "Quiet output.\n";
	}

    PicamHandle camera;
    PicamCameraID id;
    PicamAvailableData data;
    PicamAcquisitionErrorsMask errors;
	
	camera = InitializeCamera(id, data, errors, verboseOutput);

    ConfigureCamera( camera, verboseOutput );

    const string specwin = "Spectrum";
    namedWindow( specwin, WINDOW_NORMAL);
    resizeWindow( specwin, 1000, 300);

    const string imwin = "Raw Image";
    namedWindow( imwin, WINDOW_NORMAL);
    resizeWindow( imwin, 1000, 300);



    bool go = true;
    while (go)
    {
    	// Collect one shot:
    	Mat image = CollectShot(camera, data, errors, verboseOutput);

    	// TODO: refactor this process so it only takes in the 
    	// image object and returns the planes object:
    	Mat padded;
	    int m = getOptimalDFTSize( image.rows );
	    int n = getOptimalDFTSize( image.cols );
	    copyMakeBorder(image, padded, 0, m - image.rows, 0, n - image.cols, BORDER_CONSTANT, Scalar::all(0));

	    Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
	    Mat complexI;

	    merge(planes, 2, complexI);  // Add to the expanded another plane with zeros

	    dft(complexI, complexI, DFT_ROWS);   // this way the result may fit in the source matrix

	    split(complexI, planes);   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))

	    Mat realI = planes[0];
	    Mat imagI = planes[1];

	    Mat magI = Mat::zeros(padded.size(), CV_32F);
        magnitude(realI, imagI, magI);

	    // crop the spectrum, if it has an odd number of rows or columns
	    realI = realI(Rect(0, 0, realI.cols & -2, realI.rows & -2));
	    imagI = imagI(Rect(0, 0, imagI.cols & -2, imagI.rows & -2));

        std::cout << magI.rowRange(Range(200,201));

        log(magI,magI);
        normalize(magI, magI, 0, 1, CV_MINMAX);

        imshow(imwin, image);
        imshow(specwin, magI);
        if (waitKey(20) >= 0) { 
            go = false;
        }

	}

	Picam_CloseCamera( camera );
    Picam_UninitializeLibrary();
}
