# Microsoft Developer Studio Project File - Name="MythForest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=MythForest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MythForest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MythForest.mak" CFG="MythForest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MythForest - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MythForest - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MythForest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /Zm1280 /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "MythForest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MT /W3 /Zd /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /Zm1280 /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "MythForest - Win32 Release"
# Name "MythForest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Animation\AnimationComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Animation\AnimationComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\AntiAliasingRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\AtmosphereModel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Batch\BatchComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Batch\BatchComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\BloomRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Bridge\BridgeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Bridge\BridgeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Cache\CacheComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Cache\CacheComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Camera\CameraComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Camera\CameraComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\CameraCuller.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Compute\ComputeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Compute\ComputeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Data\DataComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Data\DataComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeferredLightingBufferEncodedRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeferredLightingTextureEncodedRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthBoundingRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthBoundingSetupRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthResolveRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeviceRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Engine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Entity.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EnvCube\EnvCubeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EnvCube\EnvCubeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Event.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Event\EventComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Event\EventComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EventGraph\EventGraphComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EventGraph\EventGraphComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\ExplorerComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\ExplorerComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\FieldComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\FieldComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldMesh.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldSimplygon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldTexture.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Follow\FollowComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Follow\FollowComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Form\FormComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Form\FormComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ForwardLightingRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\FrameBarrierRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\GeometryBufferRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Layout\LayoutComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Layout\LayoutComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\LightBufferEncodeRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Light\LightComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Light\LightComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\LightTextureEncodeRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Model\ModelComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Model\ModelComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Module.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\MythForest.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Navigate\NavigateComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Navigate\NavigateComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Particle\ParticleComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Particle\ParticleComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Phase\PhaseComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Phase\PhaseComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\PhaseLightRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Profile\ProfileComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Profile\ProfileComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Rasterize\RasterizeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Rasterize\RasterizeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RayTrace\RayTraceComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RayTrace\RayTraceComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderableComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderableComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderFlowComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderFlowComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderPolicy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortCameraView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortCommandQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortLightSource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortPhaseLightView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortRenderTarget.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortSharedBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortTextureInput.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenOutlineRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenSpaceFilterRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenSpaceTraceRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Script\ScriptComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Script\ScriptComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shader\ShaderComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shader\ShaderComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ShadowMaskRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shape\ShapeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shape\ShapeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\SkyComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\SkyComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sound\SoundComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sound\SoundComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Space\SpaceComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Space\SpaceComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\StencilMaskRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Stream\StreamComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Stream\StreamComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Surface\SurfaceComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Surface\SurfaceComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Tape\TapeComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Tape\TapeComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Terrain\TerrainComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Terrain\TerrainComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\TextView\TextViewComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\TextView\TextViewComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Transform\TransformComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Transform\TransformComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Unit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Visibility\VisibilityComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Visibility\VisibilityComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Widget\WidgetComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Widget\WidgetComponentModule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\WidgetRenderStage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\General\Driver\Archive\Virtual\ZArchiveVirtual.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Animation\AnimationComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Animation\AnimationComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\AntiAliasingRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\AtmosphereModel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Batch\BatchComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Batch\BatchComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\BloomRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Bridge\BridgeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Bridge\BridgeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Cache\CacheComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Cache\CacheComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Camera\CameraComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Camera\CameraComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\CameraCuller.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Compute\ComputeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Compute\ComputeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Data\DataComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Data\DataComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeferredLightingBufferEncodedRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeferredLightingTextureEncodedRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthBoundingRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthBoundingSetupRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DepthResolveRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\DeviceRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Entity.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EnvCube\EnvCubeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EnvCube\EnvCubeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Event.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Event\EventComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Event\EventComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EventGraph\EventGraphComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\EventGraph\EventGraphComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\ExplorerComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\ExplorerComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\FieldComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\FieldComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldMesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldSimplygon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Field\Types\FieldTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Follow\FollowComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Follow\FollowComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Form\FormComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Form\FormComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ForwardLightingRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\FrameBarrierRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\GeometryBufferRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Layout\LayoutComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Layout\LayoutComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\LightBufferEncodeRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Light\LightComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Light\LightComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\LightTextureEncodeRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Model\ModelComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Model\ModelComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Module.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\MythForest.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Navigate\NavigateComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Navigate\NavigateComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Particle\ParticleComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Particle\ParticleComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Phase\PhaseComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Phase\PhaseComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\PhaseLightRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Profile\ProfileComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Profile\ProfileComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Rasterize\RasterizeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Rasterize\RasterizeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RayTrace\RayTraceComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RayTrace\RayTraceComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderableComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderableComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderFlowComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderFlowComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Renderable\RenderPolicy.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortCameraView.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortCommandQueue.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortLightSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortPhaseLightView.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortRenderTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortSharedBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderPort\RenderPortTextureInput.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenOutlineRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenSpaceFilterRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderNode\ScreenSpaceTraceRenderNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ScreenSpaceTraceRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Script\ScriptComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Script\ScriptComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shader\ShaderComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shader\ShaderComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\ShadowMaskRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shape\ShapeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Shape\ShapeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\SkyComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sky\SkyComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sound\SoundComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Sound\SoundComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Space\SpaceComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Space\SpaceComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Explorer\SpaceTraversal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\StencilMaskRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Stream\StreamComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Stream\StreamComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Surface\SurfaceComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Surface\SurfaceComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Pipeline\Transparent\SurfacePipeline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\SurfaceVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Tape\TapeComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Tape\TapeComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Terrain\TerrainComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Terrain\TerrainComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\TextView\TextViewComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\TextView\TextViewComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Transform\TransformComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Transform\TransformComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Unit.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Visibility\VisibilityComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Visibility\VisibilityComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Widget\WidgetComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\Widget\WidgetComponentModule.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\MythForest\Component\RenderFlow\RenderStage\WidgetRenderStage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\General\Driver\Archive\Virtual\ZArchiveVirtual.h
# End Source File
# End Group
# End Target
# End Project
