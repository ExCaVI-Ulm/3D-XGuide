#ifndef SYSTEMGEOMETRYDEFINITIONS_H
#define SYSTEMGEOMETRYDEFINITIONS_H

/// Set the properties of the X-ray geometry
struct SystemGeometryDefinitions {
	static int CAMERA_SIZE_X; /// original width of the xray image (Live Signal format = monitor format)
	static int CAMERA_SIZE_Y; /// original height of the xray image
	static int TARGET_SIZE_X; /// width of the xray image (it will be scaled if necessary)
	static int TARGET_SIZE_Y; /// height of the xray image (it will be scaled if necessary)
	static int IMAGE_SIZE_X; /// displayed width of the xray image (used to position the image plane)
	static int IMAGE_SIZE_Y; /// displayed height of the xray image (used to position the image plane)

	static int LAPTOP_SIZE_X;
	static int LAPTOP_SIZE_Y;

	static int CROPPED_SIZE_X;	//
	static int CROPPED_SIZE_Y;

	static int FIRST_PIXEL_SIZE_X;
	static int FIRST_PIXEL_SIZE_Y;

	static int FIELD_DISTANCE;
	static double DETECTOR_SIZE;

	static double MILLIMETER_PER_PIXEL[2]; /// the X-ray scale
	//static double MILLIMETER_PER_PIXEL1; /// the X-ray scale
	static double SOURCE_DETECTOR_DISTANCE; /// im millimeters
	//static int DISTANCE_SOURCE_TO_PATIENT; // in millimeters 
	static int DISTANCE_SOURCE_TO_PATIENT[2]; // in millimeters 
	static int ISOCENTER_SHIFT_Z; // in millimeters 
	static int DISTANCE_SOURCE_TO_PATIENT_FRONTAL; // in millimeters 
	static int DISTANCE_SOURCE_TO_PATIENT_LATERAL; // in millimeters 


	static int TABLE_POSITION_X; // lat.
	static int TABLE_POSITION_Y; // long.
	static int TABLE_POSITION_Z; // vert.

	/// clipping range of video input
	static int CLIP_X_MIN;
	static int CLIP_X_MAX;
	static int CLIP_Y_MIN;
	static int CLIP_Y_MAX;
	static int CLIP_Z_MIN;
	static int CLIP_Z_MAX;

	static int CLIP_NAME;
};

#endif