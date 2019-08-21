# state file generated using paraview version 5.7.0-RC1
import sys

if (len(sys.argv) < 3):
    print("ERROR: must include: <input filename> <output filename> ...")
    print("")
    print("  pvpython paraview_vectors.py <input> <output>")
    print("")
    exit(1)

dataFileName   = sys.argv[1] 
outputFilename = sys.argv[2]

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

# trace generated using paraview version 5.7.0-RC1
#
# To ensure correct image size when batch processing, please search 
# for and uncomment the line `# renderView*.ViewSize = [*,*]`

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# get the material library
materialLibrary1 = GetMaterialLibrary()

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [1000, 1000]
renderView1.AxesGrid = 'GridAxes3DActor'
renderView1.CenterOfRotation = [1.9999999999999998e-05, 1.9999999999999998e-05, 0.0053734999999999755]
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [-2.082834064054565, 1.9669068546487183, 0.7281570050750422]
renderView1.CameraFocalPoint = [2.000000000000014e-05, 1.9999999999999602e-05, 0.005373499999999976]
renderView1.CameraViewUp = [0.28586842864904943, 0.5823724581342313, -0.7610003689279108]
renderView1.CameraParallelScale = 1.3779980681832709
renderView1.Background = [0.32, 0.34, 0.43]
renderView1.BackEnd = 'OSPRay raycaster'
renderView1.OSPRayMaterialLibrary = materialLibrary1

# Create a new 'SpreadSheet View'
spreadSheetView1 = CreateView('SpreadSheetView')
spreadSheetView1.ColumnToSort = ''
spreadSheetView1.BlockSize = 1024L
# uncomment following to set a specific view size
# spreadSheetView1.ViewSize = [400, 400]

SetActiveView(None)

# ----------------------------------------------------------------
# setup view layouts
# ----------------------------------------------------------------

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.500000)
layout1.AssignView(1, renderView1)
layout1.AssignView(2, spreadSheetView1)

# ----------------------------------------------------------------
# restore active view
SetActiveView(renderView1)
# ----------------------------------------------------------------

# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

# create a new 'CSV Reader'
vectorscsv = CSVReader(FileName=[dataFileName])
vectorscsv.HaveHeaders = 0

# create a new 'Table To Points'
tableToPoints1 = TableToPoints(Input=vectorscsv)
tableToPoints1.XColumn = 'Field 0'
tableToPoints1.YColumn = 'Field 1'
tableToPoints1.ZColumn = 'Field 2'

# create a new 'Calculator'
calculator1 = Calculator(Input=tableToPoints1)
calculator1.ResultArrayName = 'vector'
calculator1.Function = 'Field 3*iHat + Field 4*jHat+Field 5*kHat'

# create a new 'Glyph'
glyph1 = Glyph(Input=calculator1,
    GlyphType='Arrow')
glyph1.OrientationArray = ['POINTS', 'vector']
glyph1.ScaleArray = ['POINTS', 'vector']
glyph1.ScaleFactor = 0.1947761
glyph1.GlyphTransform = 'Transform2'
glyph1.GlyphMode = 'All Points'

# ----------------------------------------------------------------
# setup the visualization in view 'renderView1'
# ----------------------------------------------------------------

# show data from calculator1
calculator1Display = Show(calculator1, renderView1)

# trace defaults for the display properties.
calculator1Display.Representation = 'Surface'
calculator1Display.ColorArrayName = [None, '']
calculator1Display.OSPRayScaleArray = 'Field 3'
calculator1Display.OSPRayScaleFunction = 'PiecewiseFunction'
calculator1Display.SelectOrientationVectors = 'vector'
calculator1Display.ScaleFactor = 0.1947761
calculator1Display.SelectScaleArray = 'Field 3'
calculator1Display.GlyphType = 'Arrow'
calculator1Display.GlyphTableIndexArray = 'Field 3'
calculator1Display.GaussianRadius = 0.009738805
calculator1Display.SetScaleArray = ['POINTS', 'Field 3']
calculator1Display.ScaleTransferFunction = 'PiecewiseFunction'
calculator1Display.OpacityArray = ['POINTS', 'Field 3']
calculator1Display.OpacityTransferFunction = 'PiecewiseFunction'
calculator1Display.DataAxesGrid = 'GridAxesRepresentation'
calculator1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
calculator1Display.ScaleTransferFunction.Points = [0.008062, 0.0, 0.5, 0.0, 0.39725, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
calculator1Display.OpacityTransferFunction.Points = [0.008062, 0.0, 0.5, 0.0, 0.39725, 1.0, 0.5, 0.0]

# show data from glyph1
glyph1Display = Show(glyph1, renderView1)

# trace defaults for the display properties.
glyph1Display.Representation = 'Surface'
glyph1Display.ColorArrayName = [None, '']
glyph1Display.OSPRayScaleArray = 'Field 3'
glyph1Display.OSPRayScaleFunction = 'PiecewiseFunction'
glyph1Display.SelectOrientationVectors = 'Field 3'
glyph1Display.ScaleFactor = 0.19509183168411257
glyph1Display.SelectScaleArray = 'Field 3'
glyph1Display.GlyphType = 'Arrow'
glyph1Display.GlyphTableIndexArray = 'Field 3'
glyph1Display.GaussianRadius = 0.009754591584205628
glyph1Display.SetScaleArray = ['POINTS', 'Field 3']
glyph1Display.ScaleTransferFunction = 'PiecewiseFunction'
glyph1Display.OpacityArray = ['POINTS', 'Field 3']
glyph1Display.OpacityTransferFunction = 'PiecewiseFunction'
glyph1Display.DataAxesGrid = 'GridAxesRepresentation'
glyph1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
glyph1Display.ScaleTransferFunction.Points = [0.008062, 0.0, 0.5, 0.0, 0.39725, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
glyph1Display.OpacityTransferFunction.Points = [0.008062, 0.0, 0.5, 0.0, 0.39725, 1.0, 0.5, 0.0]

# ----------------------------------------------------------------
# setup the visualization in view 'spreadSheetView1'
# ----------------------------------------------------------------

# show data from tableToPoints1
tableToPoints1Display = Show(tableToPoints1, spreadSheetView1)

# ----------------------------------------------------------------
# finally, restore active source
SetActiveSource(glyph1)
# ----------------------------------------------------------------

WriteImage(outputFilename)
