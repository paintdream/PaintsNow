local args = { ... }
local sourceBuild = args[1] or "Build"
local redirectBuild = args[2] or "BuildLinux"

local targetSolution = "./Purpleave.sln"
local sourceSolution = "../../" .. sourceBuild .. "/PaintsNow.sln"
local cd = io.popen("echo %cd%")
local currentDirectory = cd:read("l")
cd:close()

local extraReferences = {
	["BridgeSunset"] = {
		"PaintsNow"
	},
	["EchoLegend"] = {
		"PaintsNow"
	},
	["GalaxyWeaver"] = {
		"PaintsNow"
	},
	["HeartVioliner"] = {
		"PaintsNow"
	},
	["LeavesFlute"] = {
		"PaintsNow",
		"BridgeSunset",
		"EchoLegend",
		"GalaxyWeaver",
		"HeartVioliner",
		"MythForest",
		"PurpleTrail",
		"Remembery",
		"SnowyStream",
	},
	["MythForest"] = {
		"PaintsNow",
		"BridgeSunset",
		"EchoLegend",
		"HeartVioliner",
		"Remembery",
		"SnowyStream",
	},
	["PurpleTrail"] = {
		"PaintsNow",
		"BridgeSunset"
	},
	["Remembery"] = {
		"PaintsNow",
		"BridgeSunset"
	},
	["SnowyStream"] = {
		"PaintsNow",
		"BridgeSunset"
	},
	["PaintsNow"] = {
		"OpenAL",
		"mp3lame",
		"freetype",
		"FreeImage",
		"event_static",
		"event_core_static",
		"event_extra_static",
	}
}

local blackList = {
	["ALL_BUILD"] = true,
	["INSTALL"] = true,
	["ZERO_CHECK"] = true,
	["PACKAGE"] = true,
	["LeavesWing"] = true,
	["LostDream"] = true,
	["glfw"] = true,
	-- ["OpenAL"] = true,
	-- ["mp3lame"] = true
}

local filereplace = {
	["ComDef.cpp"] = "",
	["ComDispatch.cpp"] = "",
	["ComBridge.cpp"] = "",
	["evthread_win32.c"] = "evthread_pthread.c",
	["buffer_iocp.c"] = "epoll.c",
	["event_iocp.c"] = "epoll_sub.c",
	["bufferevent_async.c"] = "",
	["ZFrameGLFW.cpp"] = "",
	["winmm.c"] = "opensl.c",
	["mmdevapi.c"] = "",
	["mixer_sse.c"] = "mixer_neon.c",
	["mixer_sse2.c"] = "",
	["mixer_sse3.c"] = "",
	["mixer_sse41.c"] = "",
	--["nulleffects.c"] = "",
	--["nullbackends.c"] = ""
}

if not table.unpack then
	table.unpack = unpack
end

local function GenerateGuid()
	local numbers = {}
	for i = 1, 16 do
		table.insert(numbers, math.random(0, 255))
	end

	return string.format("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		table.unpack(numbers)
	)
end

local function GetFolder(path)
	local folder = path:match("(.-)/[^/]*$")
	if folder then
		return folder
	else
		return path:match("(.-)\\[^\\]*$")
	end
end

local function ParseSolution(path)
	local file = io.open(path, "rb")
	if not file then
		error("Enable to open file: " .. path)
	end

	local content = file:read("*all")
	file:close()

	local folder = GetFolder(path)

	local projects = {}
	for guid, name, path, config, dep in content:gmatch("Project%(\"{(.-)}\"%) = \"(.-)\", \"(.-)\", \"{(.-)}\"(.-)\nEndProject") do
		if not blackList[name] then
			print("Guid: " .. guid .. " | Name: " .. name .. " | Path: " .. path)

			-- try to parse project
			local cpps = {}
			local hpps = {}
			local includeFolder = ""
			if guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
				local proj = io.open(folder .. "/" .. path, "rb")
				if not proj then
					error("Unable to open " .. folder .. "/" .. path)
				end
				local xml = proj:read("*all")
				proj:close()

				for cpp in xml:gmatch("<ClCompile Include=\"(.-)\"") do
					local parent = GetFolder(cpp)
					local file = cpp:sub(#parent + 2)
					local rep = filereplace[file]
					if rep then
						if rep ~= "" then
							print("Replacing [" .. file .. "] with [" .. rep .. "]")
							cpp = cpp:sub(1, #parent + 1) .. rep
						else
							print("Removing [" .. file .. "]")
							cpp = nil
						end
					end
			
					if cpp then
						table.insert(cpps, cpp)
					end
				end

				for hpp in xml:gmatch("<ClInclude Include=\"(.-)\"") do
					table.insert(hpps, hpp)
				end
				
				includeFolder = xml:match("<AdditionalIncludeDirectories>(.-)</AdditionalIncludeDirectories>")
				includeFolder = includeFolder:gsub("\\" .. sourceBuild .. "\\Source", "\\" .. redirectBuild .. "\\Source")
				print(includeFolder)
				assert(includeFolder)
			end

			local deps = {}
			print(name)
			for d in dep:gmatch("{(.-)} = {(.-)}") do
				-- print("DEP " .. d)
				table.insert(deps, d)	
			end

			table.insert(projects, {
				Guid = guid,
				Name = name,
				Path = path,
				ConfigurationGuid = config,
				Includes = hpps,
				Sources = cpps,
				IncludeFolder = includeFolder,
				Dependencies = deps
			})
		end
	end

	local nested = content:find("GlobalSection%(NestedProjects%) = preSolution")
	if nested then
		for sub, parent in content:gmatch("{([^%s]+)} = {([^%s]+)}", nested) do
			print("Sub: " .. sub .. " | Parent: " .. parent)
			for i = 1, #projects do
				local project = projects[i]
				if project.ConfigurationGuid == sub then
					project.NestedGuid = parent
					break
				end
			end
		end
	end

	return projects
end

local function GenerateDeclaration(projects)
	local projectDeclare =
[[Project("{%s}") = "%s", "%s", "{%s}"%s
EndProject]]

	local depsFormat =
[[

	ProjectSection(ProjectDependencies) = postProject	
%s
	EndProjectSection]]
	local projectNested = 
[[		{%s} = {%s}]]

	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		local deps = {}

		--print("PROJ: " .. project.Name)
		if project.Dependencies then
			for _, dep in ipairs(project.Dependencies) do
				--print("ADD DEP: " .. dep)
				table.insert(deps, projectNested:format(dep, dep))
			end
		end

		table.insert(target, projectDeclare:format(
			project.Guid, project.Name, project.Path, project.ConfigurationGuid, depsFormat:format(table.concat(deps, "\n"))
		))
	end

	return table.concat(target, "\n")
end

local function GenerateConfiguration(projects)
	local projectConfiguration =
[[		{%s}.Debug|ARM.ActiveCfg = Debug|ARM
		{%s}.Debug|ARM.Build.0 = Debug|ARM
		{%s}.Debug|ARM64.ActiveCfg = Debug|ARM64
		{%s}.Debug|ARM64.Build.0 = Debug|ARM64
		{%s}.Debug|x64.ActiveCfg = Debug|x64
		{%s}.Debug|x64.Build.0 = Debug|x64
		{%s}.Debug|x86.ActiveCfg = Debug|x86
		{%s}.Debug|x86.Build.0 = Debug|x86
		{%s}.Release|ARM.ActiveCfg = Release|ARM
		{%s}.Release|ARM.Build.0 = Release|ARM
		{%s}.Release|ARM64.ActiveCfg = Release|ARM64
		{%s}.Release|ARM64.Build.0 = Release|ARM64
		{%s}.Release|x64.ActiveCfg = Release|x64
		{%s}.Release|x64.Build.0 = Release|x64
		{%s}.Release|x86.ActiveCfg = Release|x86
		{%s}.Release|x86.Build.0 = Release|x86]]

	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		if project.Guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
			local guids = {}
			for j = 1, 16 do
				table.insert(guids, project.ConfigurationGuid)
			end

			table.insert(target, projectConfiguration:format(table.unpack(guids)))
		end
	end

	return table.concat(target, "\n")
end

local function GenerateNested(projects)
	local projectNested = 
[[		{%s} = {%s}]]
	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		if project.NestedGuid then
			table.insert(target, projectNested:format(project.ConfigurationGuid, project.NestedGuid))
		end
	end

	return table.concat(target, "\n")
end

local function InjectAndroidProjects(projects)
	local hostGuid = "F896F62F-0D10-4111-9D67-519610420117"
	local sections = {}
	for i = 1, #projects do
		local project = projects[i]
		table.insert(sections, project.ConfigurationGuid)
	end

	table.insert(projects, {
		Guid = "2150E333-8FDC-42A3-9474-1A3956D46DE8",
		ConfigurationGuid = hostGuid,
		Name = "Purpleave",
		Path = "Purpleave",
	})

	table.insert(projects, {
		Guid = "39E2626F-3545-4960-A6E8-258AD8476CE5",
		ConfigurationGuid = "5950AEA1-2639-4826-9C79-AFA61E9D6468",
		Name = "Purpleave.Packaging",
		Path = "Purpleave.Packaging\\Purpleave.Packaging.androidproj",
		NestedGuid = hostGuid
	})

	table.insert(projects, {
		Guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942",
		ConfigurationGuid = "4E02E07E-1853-47BF-A692-89A7B1C98D5A",
		Name = "Purpleave.NativeActivity",
		Path = "Purpleave.NativeActivity\\Purpleave.NativeActivity.vcxproj",
		NestedGuid = hostGuid,
		Dependencies = sections, -- deps:format(table.concat(sections, "\n")),
		Libraries = [[LeavesFlute.a;Remembery.a;MythForest.a;SnowyStream.a;BridgeSunset.a;PurpleTrail.a;HeartVioliner.a;EchoLegend.a;GalaxyWeaver.a;PaintsNow.a;Source\General\Driver\Font\Freetype\Core\freetype.a;Source\General\Driver\Image\FreeImage\Core\FreeImage.a;Source\General\Driver\Network\LibEvent\Core\lib\event.a;Source\General\Driver\Network\LibEvent\Core\lib\event_core.a]],
	})
end

local function GenerateSolution(projects)
	local slnTemplate = [[
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 16
VisualStudioVersion = 16.0.31205.134
MinimumVisualStudioVersion = 10.0.40219.1
%s
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|ARM = Debug|ARM
		Debug|ARM64 = Debug|ARM64
		Debug|x64 = Debug|x64
		Debug|x86 = Debug|x86
		Release|ARM = Release|ARM
		Release|ARM64 = Release|ARM64
		Release|x64 = Release|x64
		Release|x86 = Release|x86
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
%s
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
%s
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {%s}
	EndGlobalSection
EndGlobal
]]

	return slnTemplate:format(
		GenerateDeclaration(projects),
		GenerateConfiguration(projects),
		GenerateNested(projects),
		"25DC1629-CA39-4D5F-968D-A5BC32ADF9E6"
	)
end

local function WriteFile(path, content)
	local folder = GetFolder(path)
	if folder and #folder > 2 then
		print("MKDIR " .. folder)
		os.execute("mkdir " .. folder)
	end

	local file = io.open(path, "wb")
	if not file then
		error("Unable to write file: " .. path)
		return
	end

	file:write(content)
	file:close()
end

local function GenerateProject(project, projects)
	local vcxprojTemplate =
[[<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|ARM">
      <Configuration>Debug</Configuration>
      <Platform>ARM</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|ARM">
      <Configuration>Release</Configuration>
      <Platform>ARM</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|ARM64">
      <Configuration>Debug</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|ARM64">
      <Configuration>Release</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x86">
      <Configuration>Debug</Configuration>
      <Platform>x86</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x86">
      <Configuration>Release</Configuration>
      <Platform>x86</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{%s}</ProjectGuid>
    <Keyword>Android</Keyword>
    <RootNamespace>%s</RootNamespace>
    <MinimumVisualStudioVersion>14.0</MinimumVisualStudioVersion>
    <ApplicationType>Android</ApplicationType>
    <ApplicationTypeRevision>3.0</ApplicationTypeRevision>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings" />
  <ImportGroup Label="Shared" />
  <ImportGroup Label="PropertySheets" />
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'">
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
	  <RuntimeTypeInfo>true</RuntimeTypeInfo>
	  <PreprocessorDefinitions>NO_LCMS;__ANSI__;DISABLE_PERF_MEASUREMENT;FREEIMAGE_LIB;OPJ_STATIC;LIBRAW_NODLL;_7ZIP_ST;CMAKE_PAINTSNOW;CMAKE_ANDROID;HAVE_NEON;HAVE_PTHREAD_MUTEX_TIMEDLOCK=0;USE_OPTICK=1;HAVE_OPENSL=1;AL_ALEXT_PROTOTYPES=1;FT2_BUILD_LIBRARY;HAVE_MEMCPY=1;HAVE_MPGLIB=1;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <CppLanguageStandard>c++1z</CppLanguageStandard>
	  <ExceptionHandling>Enabled</ExceptionHandling>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemGroup>
%s
  </ItemGroup>
  <ItemGroup>
%s
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets" />
</Project>
]]

	local xmlNode = {}
	print("Generating Project: " .. project.Name)
	for _, cpp in ipairs(project.Sources) do
		table.insert(xmlNode, "    <ClCompile Include=\"" .. cpp .. "\" />")
	end

	for _, hpp in ipairs(project.Includes) do
		table.insert(xmlNode, "    <ClInclude Include=\"" .. hpp .. "\" />")
	end

	local depFormat =
[[    <ProjectReference Include="%s">
      <Project>{%s}</Project>
      <Name>%s</Name>
	</ProjectReference>]]

	local deps = {}
	for _, dep in ipairs(project.Dependencies) do
		for _, proj in ipairs(projects) do
			if proj.ConfigurationGuid == dep then
				local fullPath = currentDirectory .. "\\" .. proj.Path
				print("Project Reference: " .. proj.ConfigurationGuid .. " --- " .. fullPath)
				table.insert(deps, depFormat:format(fullPath, proj.ConfigurationGuid, proj.Name))
				break
			end
		end
	end

	local references = extraReferences[project.Name]
	if references then
		for _, ref in ipairs(references) do
			for _, proj in ipairs(projects) do
				if proj.Name == ref then
					local fullPath = currentDirectory .. "\\" .. proj.Path
					print("Project Reference: " .. proj.ConfigurationGuid .. " --- " .. fullPath)
					table.insert(deps, depFormat:format(fullPath, proj.ConfigurationGuid, proj.Name))
					break
				end
			end
		end
	end

	local include = project.IncludeFolder
	return vcxprojTemplate:format(project.ConfigurationGuid, project.Name, 
		include, include, include, include, include, include, include, include,	
		table.concat(xmlNode, "\n"),
		table.concat(deps, "\n")
	)
end

-- Main
local projects = ParseSolution(sourceSolution)
-- projects
for i = 1, #projects do
	local project = projects[i]
	if project.Guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
		local content = GenerateProject(project, projects)
		WriteFile(project.Path, content)
	end
end

InjectAndroidProjects(projects)
local content = GenerateSolution(projects)
WriteFile(targetSolution, content)
