#include "SystemGeometryDefinitions.h"

#include <vtkType.h>

// images size of the the framegrabber output
int SystemGeometryDefinitions::CAMERA_SIZE_X = 1280;
int SystemGeometryDefinitions::CAMERA_SIZE_Y = 1024;

//int SystemGeometryDefinitions::FIRST_PIXEL_SIZE_X = 258; // (259 - 1) since starts with 0 in VTK
int SystemGeometryDefinitions::FIRST_PIXEL_SIZE_X = 256; // (257 - 1) since starts with 0 in VTK
int SystemGeometryDefinitions::FIRST_PIXEL_SIZE_Y = 24;	// (25 - 1) since starts with 0 in VTK

int SystemGeometryDefinitions::LAPTOP_SIZE_X = 1080;	//
int SystemGeometryDefinitions::LAPTOP_SIZE_Y = 1920;

//int SystemGeometryDefinitions::CROPPED_SIZE_Y = CAMERA_SIZE_Y - (FIRST_PIXEL_SIZE_Y - 1);
//int SystemGeometryDefinitions::CROPPED_SIZE_X = FIRST_PIXEL_SIZE_X + CROPPED_SIZE_Y - 1;	

//int SystemGeometryDefinitions::CROPPED_SIZE_X = 1257;	// (1258 - 1) since starts with 0 in VTK
int SystemGeometryDefinitions::CROPPED_SIZE_X = 1255;	// (1256 - 1) since starts with 0 in VTK
int SystemGeometryDefinitions::CROPPED_SIZE_Y = 1000;

int SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[2] = { {810}, {765}}; // DistanceSourceToPatient in mm
//int SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[2] = { 810, 765 }; // DistanceSourceToPatient in mm
int SystemGeometryDefinitions::ISOCENTER_SHIFT_Z = 12; //isocenter for the lateral C-arm is shifted about 12mm away compared to frontal C-arm
//// default Tischposition for PHANTOM from 07.07.2018
//int SystemGeometryDefinitions::TABLE_POSITION_X = -110; // lat.
//int SystemGeometryDefinitions::TABLE_POSITION_Y = -760; // long.
//int SystemGeometryDefinitions::TABLE_POSITION_Z = 0; //vert. = höhe

//// default Tischposition for KUGEL PHANTOM from 31.08.2018
//int SystemGeometryDefinitions::TABLE_POSITION_X = -70; // lat.
//int SystemGeometryDefinitions::TABLE_POSITION_Y = -640; // long.
//int SystemGeometryDefinitions::TABLE_POSITION_Z = 0; //vert. = höhe

// shift der Referenzkoordinatensysteme der Tischposition
int SystemGeometryDefinitions::TABLE_POSITION_X = 0; // lat.
int SystemGeometryDefinitions::TABLE_POSITION_Y = 0; // long.
int SystemGeometryDefinitions::TABLE_POSITION_Z = 0; //vert. = höhe

int SystemGeometryDefinitions::FIELD_DISTANCE = 25;
double SystemGeometryDefinitions::DETECTOR_SIZE = 174.98;	//174.98 mm - Detector size at FD = 25 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 132.902;	//132.902 mm - Detector size at FD = 19 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 132.30;	//132.30 mm - Detector size at FD = 20 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 156.31;	//156.31 mm - Detector size at FD = 22 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 187.775;	//187.775 mm - Detector size at FD = 27 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 259.644;	//259.644 mm - Detector size at FD = 37 cm;
//double SystemGeometryDefinitions::DETECTOR_SIZE = 292.908;	//292.908 mm - Detector size at FD = 42 cm;

//double SystemGeometryDefinitions::MILLIMETER_PER_PIXEL = 0.17087;
double SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[2] = { DETECTOR_SIZE / CROPPED_SIZE_Y }; // in [mm/px] 
double SystemGeometryDefinitions::SOURCE_DETECTOR_DISTANCE = 1039.0; // in mm

// parameters that describe how to clip the scaled(!) image
int SystemGeometryDefinitions::CLIP_X_MIN = SystemGeometryDefinitions::FIRST_PIXEL_SIZE_X;
int SystemGeometryDefinitions::CLIP_X_MAX = SystemGeometryDefinitions::CROPPED_SIZE_X;
int SystemGeometryDefinitions::CLIP_Y_MIN = SystemGeometryDefinitions::FIRST_PIXEL_SIZE_Y;
int SystemGeometryDefinitions::CLIP_Y_MAX = SystemGeometryDefinitions::CAMERA_SIZE_Y - 1;
int SystemGeometryDefinitions::CLIP_Z_MIN = 0;
int SystemGeometryDefinitions::CLIP_Z_MAX = 1;

int SystemGeometryDefinitions::CLIP_NAME = -25;