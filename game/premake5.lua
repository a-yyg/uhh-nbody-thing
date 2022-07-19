require("premake5-cuda")

baseName = path.getbasename(os.getcwd());

project (workspaceName)
  	kind "ConsoleApp"
    location "../_build"
    targetdir "../_bin/%{cfg.buildcfg}"
	
    filter "configurations:Release"
      symbols "Off"
      optimize "Full"
		kind "WindowedApp"
		entrypoint "mainCRTStartup"

	filter "action:vs*"
        debugdir "$(SolutionDir)"
		
	filter {"action:vs*", "configurations:Release"}
			kind "WindowedApp"
			entrypoint "mainCRTStartup"
	filter {}
	
    vpaths 
    {
        ["Header Files/*"] = { "src/**.h", "**.h"},
        ["Source Files/*"] = {"src/**.c", "src/**.cpp","**.c", "**.cpp"},
    }
    files {"**.c", "**.cpp", "**.h"}

    includedirs { "./", "src"}
	link_raylib();
	
	-- To link to a lib use link_to("LIB_FOLDER_NAME")

  buildcustomizations "BuildCustomizations/CUDA 11.4"
  cudaPath "/usr/local/cuda" -- Only affects linux, because the windows builds get CUDA from the VS extension

  -- CUDA specific properties
  cudaFiles {"game/**.cu"} -- files NVCC compiles
  -- cudaMaxRegCount "32"

  -- Let's compile for all supported architectures (and also in parallel with -t0)
  cudaCompilerOptions {"-arch=sm_52", "-gencode=arch=compute_52,code=sm_52", "-gencode=arch=compute_60,code=sm_60",
                       "-gencode=arch=compute_61,code=sm_61", "-gencode=arch=compute_70,code=sm_70",
                       "-gencode=arch=compute_75,code=sm_75", "-gencode=arch=compute_80,code=sm_80",
                       "-gencode=arch=compute_86,code=sm_86", "-gencode=arch=compute_86,code=compute_86", "-t0"}                      

  -- On Windows, the link to cudart is done by the CUDA extension, but on Linux, this must be done manually
  if os.target() == "linux" then 
      linkoptions {"-L/usr/local/cuda/lib64 -lcudart"}
  end

  filter "configurations:release"
  cudaFastMath "On" -- enable fast math for release
  filter ""

  filter "configurations:debug"
  cudaFastMath "Off" -- disable fast math for debug
  cudaDebugSymbols "On" -- enable debug symbols for debug

  -- Warnings are errors
  flags { "FatalWarnings" }

  -- Link cuda objects to cpp objects
  
