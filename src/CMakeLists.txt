cmake_minimum_required(VERSION 2.8)
PROJECT(3D-XGuide)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 3D-XGuide)

# Visualization Toolkit
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
SET(DESIRED_QT_VERSION ${VTK_DESIRED_QT_VERSION} CACHE FILEPATH "")
FIND_PACKAGE(Qt)
IF(QT_USE_FILE)
  INCLUDE(${QT_USE_FILE})
ELSE(QT_USE_FILE)
  SET(QT_LIBRARIES   ${QT_QT_LIBRARY})
ENDIF(QT_USE_FILE)
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Gui)
FIND_PACKAGE(Qt5Widgets)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# OpenCV
FIND_PACKAGE(OpenCV REQUIRED)

include_directories (${OpenCV_INCLUDE_DIRS})

# GDCM (DICOM package)
#SET(GDCM_COMPILE_WITH_GDCM TRUE CACHE BOOL "Compile with DICOM file support (GDCM lib)")
#IF(GDCM_COMPILE_WITH_GDCM)
  FIND_PACKAGE(GDCM)
  SET(GDCM_LIBS Compute3DSpacing.lib ConvertSingleBitTo8Bits.lib CreateFakeRTDOSE.lib gdcm2pnm.lib
				gdcm2vtk.lib gdcmcharls.lib gdcmCommon.lib gdcmDICT.lib gdcmDSED.lib gdcmexpat.lib
				gdcmgetopt.lib gdcmIOD.lib gdcmjpeg12.lib gdcmjpeg16.lib gdcmjpeg8.lib gdcmMEXD.lib
				gdcmMSFF.lib gdcmopenjpeg.lib gdcmorthoplanes.lib gdcmreslice.lib gdcmrtionplan.lib
				gdcmrtplan.lib gdcmscene.lib gdcmtexture.lib gdcmviewer.lib gdcmvolume.lib gdcmzlib.lib
				GenerateRTSTRUCT.lib offscreenimage.lib rtstructapp.lib socketxx.lib vtkgdcm.lib) 

  INCLUDE_DIRECTORIES (${GDCM_INCLUDE_DIRS})
  link_directories (${GDCM_LIBRARY_DIRS})
 # ADD_DEFINITIONS(-DCOMPILE_WITH_GDCM)
#ENDIF(GDCM_COMPILE_WITH_GDCM)

#DicomWriter
find_package(DICOM QUIET)
if(DICOM_FOUND)
include(${DICOM_USE_FILE})
endif ()
set(VTK_DICOM_LIBRARIES vtkDICOM)

#set(CURL_DIR "b:/NAVIGATION/SW/vcpkg/installed/x86-windows/share/curl")

find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
###########################################################
# add remove files here

# GUI
set(GuiSrcs
  gui/MainWindow.cpp
  gui/MeshesDialog.cpp
  gui/AlgorithmPropertiesDialog.cxx
  gui/XRayViewer.cpp

)

set(GuiHeaders
  gui/MainWindow.h
  gui/MeshesDialog.h
  gui/AlgorithmPropertiesDialog.h
  gui/XRayViewer.h

)

set(GuiUIs
  gui/MainWindow.ui
  gui/MeshesDialog.ui
  gui/AlgorithmPropertiesDialog.ui
  gui/XRayViewer.ui
  gui/UsageDialog.ui
)

# pipeline
set(PipelineSrcs
  pipeline/DICOMVisualizer.cxx
  pipeline/XRAYReader.cxx
  pipeline/OverlayScene.cxx
)

set(PipelineHeaders
  pipeline/DICOMVisualizer.h
  pipeline/XRAYReader.h
  pipeline/OverlayScene.h
)

# interaction
set(InteractionSrcs
	interaction/SceneLabeling.cxx
	interaction/vtkInteractorStyleMy2D.cxx
	interaction/vktInteractorStyleTrackballJoystickHybridActor.cxx
	interaction/MoveMeshInteractionCommand.cxx
)

set(InteractionHeaders
	interaction/SceneLabeling.h
	interaction/vtkInteractorStyleMy2D.h
	interaction/vtkInteractorStyleOnlyZoomWithViewAngle.h
	interaction/vtkInteractorStyle2DRotateZoomWithViewAngle.h
	interaction/vktInteractorStyleTrackballJoystickHybridActor.h
	interaction/MoveMeshInteractionCommand.h
)

# geometry
set(GeometrySrcs
	geometry/SystemGeometryDefinitions.cxx
	geometry/BiplaneGeometry.cxx
	geometry/XRayGeometry.cxx
	geometry/Panel.cxx
)

set(GeometryHeaders
	geometry/SystemGeometryDefinitions.h
	geometry/BiplaneGeometry.h
	geometry/XRayGeometry.h
	geometry/Panel.h
)

# algorithms
set(AlgorithmsSrcs
	algorithms/FilterAlgorithmFactory.cxx
	algorithms/BiplaneFilterAlgorithmFactory.cxx

)

set(AlgorithmsHeaders
	algorithms/FilterAlgorithmFactory.h
	algorithms/BiplaneFilterAlgorithmFactory.h

)

# algorithms/filters
set(AlgorithmsFiltersSrcs
  algorithms/filters/GenericMotionCorrectionFilter.cxx
  algorithms/filters/MotionCorrectionFilter.cxx
  algorithms/filters/BiplaneAlgorithm.cxx
  algorithms/filters/SimpleBiplaneFilter.cxx
  algorithms/filters/MotionTrackingFilter.cxx
  algorithms/filters/PointListBasedFilter.cxx
  algorithms/filters/CrosscorrelationFilter.cxx
  algorithms/filters/CrosscorrelationMotionCorrectionFilter.cxx
  algorithms/filters/DeepLearningMotionCorrectionFilter.cxx
  
)

set(AlgorithmsFiltersHeaders
  algorithms/filters/GenericMotionCorrectionFilter.h
  algorithms/filters/MotionCorrectionFilter.h
  algorithms/filters/BiplaneAlgorithm.h
  algorithms/filters/SimpleBiplaneFilter.h
  algorithms/filters/MotionTrackingFilter.h
  algorithms/filters/PointListBasedFilter.h
  algorithms/filters/CrosscorrelationFilter.h
  algorithms/filters/CrosscorrelationMotionCorrectionFilter.h
  algorithms/filters/DeepLearningMotionCorrectionFilter.h
  
)

# algorithms/GUI
set(AlgorithmsGuiSrcs
  algorithms/gui/AbstractFilterWidget.cxx
  algorithms/gui/MotionCorrectionFilterWidget.cxx
  algorithms/gui/CrosscorrelationFilterWidget.cxx
  algorithms/gui/PreprocessingFilterWidget.cxx
  algorithms/gui/DLFilterWidget.cxx

)

set(AlgorithmsGuiHeaders
  algorithms/gui/AbstractFilterWidget.h
  algorithms/gui/MotionCorrectionFilterWidget.h
  algorithms/gui/CrosscorrelationFilterWidget.h
  algorithms/gui/PreprocessingFilterWidget.h
  algorithms/gui/DLFilterWidget.h

)

set(AlgorithmsGuiUIs
  algorithms/gui/MotionCorrectionFilterWidget.ui
  algorithms/gui/CrosscorrelationFilterWidget.ui
  algorithms/gui/PreprocessingFilterWidget.ui
  algorithms/gui/DLFilterWidget.ui
)


###########################################################
# make the include files available
include_directories(gui)
include_directories(pipeline)
include_directories(interaction)
include_directories(geometry)
include_directories(algorithms)
include_directories(algorithms/filters)
include_directories(algorithms/gui)

# generate folder structure in the project
source_group(GUI FILES ${GuiSrcs} ${GuiHeaders} ${GuiUIs})
source_group(Pipeline FILES ${PipelineSrcs} ${PipelineHeaders})
source_group(Interaction FILES ${InteractionSrcs} ${InteractionHeaders})
source_group(Geometry FILES ${GeometrySrcs} ${GeometryHeaders})
source_group(Algorithms FILES ${AlgorithmsSrcs} ${AlgorithmsHeaders})
source_group(Algorithms\\Filters FILES ${AlgorithmsFiltersSrcs} ${AlgorithmsFiltersHeaders})
source_group(Algorithms\\GUI FILES ${AlgorithmsGuiSrcs} ${AlgorithmsGuiHeaders} ${AlgorithmsGuiUIs})

# put all MOC'ed files into one folder
source_group(MOC REGULAR_EXPRESSION moc*)

###########################################################
# CXX files
SET (Srcs
  ${GuiSrcs}
  ${PipelineSrcs}
  ${InteractionSrcs}
  ${GeometrySrcs}
  ${AlgorithmsSrcs}
  ${AlgorithmsFiltersSrcs}
  ${AlgorithmsGuiSrcs}
  
  main.cxx
)

# UI files 
SET(UIS
  ${GuiUIs}
  ${AlgorithmsGuiUIs}
)

# Header files THAT NEED TO BE MOC'ED
SET(HeadersToMoc
  gui/MainWindow.h
  gui/MeshesDialog.h
  gui/XRayViewer.h
  
)

# all header files
SET(Headers
  ${GuiHeaders}
  ${PipelineHeaders}
  ${InteractionHeaders}
  ${GeometryHeaders}
  ${AlgorithmsHeaders}
  ${AlgorithmsFiltersHeaders}
  ${AlgorithmsGuiHeaders}
  
)

###########################################################
# Qt user interface compiling (UIC) and Meta Object Compiling (MOC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

QT5_WRAP_CPP(MOCSrcs ${HeadersToMoc} )
QT5_WRAP_UI(UIHeaders ${UIS})
ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT5_SUPPORT)
SET_SOURCE_FILES_PROPERTIES(${Srcs} PROPERTIES OBJECT_DEPENDS "${UIHeaders}")

# Make MS-VC not to complain about standard sprintf etc.
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

########## for setting target to windows console application
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")


# It's nice to have the ui in the windows project file...just double click on it
# and designer comes up on that ui file :)
IF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
  SET (SimpleViewSrcs ${Srcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
IF (${CMAKE_BUILD_TOOL} MATCHES "devenv")
  SET (SimpleViewSrcs ${Srcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "devenv")


add_executable(${CMAKE_PROJECT_NAME} WIN32 ${Srcs} ${Headers})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nlohmann_json nlohmann_json::nlohmann_json ${VTK_LIBRARIES} ${OpenCV_LIBS} ${QT_LIBRARIES} Qt5::Gui Qt5::Core Qt5::Widgets
					  ${GDCM_LIBS} ${VTK_DICOM_LIBRARIES} CURL::libcurl)

