## How to start

**If you have access to provided test data ('PHANTOM' folder), proceed like this:**
- go to 'File | Load scene...' and choose .ini configuration file ('scene\AP_90.ini') to load complete 3D scene
- use the options in the 'Display' menu
- press 'Registration' button in the 'Registration' panel of the 'NavigationGUI" to perform 3D-3D registration
- press 'Load Run' button in 'Reference 1' and/or 'Reference 2' windows to choose any of available XR image series from folder 'XRAY'

**Alternatively you can load all data separately:**
- go to 'File | Load 3D MRI dataset' and choose 3D volume image  'CT_Kugeln.dcm'
- press 'Add' button in 'Mesh settings' panel to load respective 3D models in polygonal data .vtk format (folder 'vtk_meshes')
- go to 'File | Open XRay viewer' to open image fusion window ' XRay Viewer'
- press 'Load Run' button in 'Reference 1' and 'Reference 2' windows to choose XR image series for registration (folder 'XRAY/9_LAO0_LAO90/frontal(lateral)')

**If you do not have test data, proceed like described above with your own data, except:**
- go to 'File | Select XRay Reference 1' and 'Select XRay Reference 2' to load XR projections for registration (in usual XR DICOM format)
- go to 'File | Select LIVE XRay sequence' to load any additional XR DICOM file


## Interaction
- go in 'Move objects' mode to interact with the meshes
- go in 'Move scene' mode to move the scene
- go to 'Marker Points | Set Marker point' to activate the cursor with additional popup menu appearing on the left mouse click  in 2D views (for marker points and tracking templates)
- use the options in the 'Interaction' menu

**How to interact with the mouse:**

- middle mouse + move = camera pan  (move objects/scene)
- left mouse button + 'ctrl' = rotate 2D  (in 3D scene or in 'Move objects' mode)
- left mouse+ move = rotate 3D ( in 3D scene or in 'Move objects' mode)
- right mouse + move = camera zoom
- middle mouse wheel scroll = zoom
- right mouse button = pause XR video (after choosing 'Set marker point')  

**Keyboard shortcuts in 3D view:**

'x' = side view  
'y' = top-down view  
'z' = front view  
'r' = reset camera (ensure that whole scene is visible)  


## Marker points reconstruction

- add corresponding points in the 2D windows via the cursor and left mouse click to 'Add marker point' in the popup menu
- press 'Marker Points | Reconstruct 3D point / Reconstruct all 3D points' to reconstruct the corresponding 2D XR points
- the points appear in the 3D window
- use 'Remove marker point' in the additional menu or press 'Marker Points | Remove all 3D points' to remove 3D points from the 3D scene


## Registration

- perform manual registration by ineracting with the meshes in 'Move objects' mode
- alternatively add corresponding registration points in the 3D scene via 'Registration' panel
- the points appear in the 3D scene after pressing 'Add' button
- adjust the point position by using the sliders for respective slice orientations in the 'MRI/CT Volume' panel
- press button 'Registration' to perform automatic registration (number of reconstructed points and registration points should be the same)


## Mesh

- use the options in the 'Mesh' menu to load default or saved mesh matrix, and to save the current mesh matrix


## Catheter tracking

- choose tracking method via ' Tracking | Select filter' and use 'Filter properties'' for additional settings
- use Crosscorrelation Motion Correction Filter in the 'LIVE' view
- use Crosscorrelation Biplane Filter in the 2D reference views (in both views templates should be set to use the functionality)
- activate the cursor with popup menu via 'Marker Points | Set Marker point' 
- left click on the point to be tracked or used for motion compensation and add/reset/remove motion compensation or tracking template using popup menu
