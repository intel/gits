---
hide:
  - navigation
---
# Using GITS

This page aims to provide a general overview of how to use gits. For an in-depth look please see the comprehensive [documentation section](documentation/terminology.md)
## GITS build output
**GITS** consists of three parts:

- a **recorder** that consists of one or multiple libraries,
- a **gitsPlayer**-executable and
- a **configuration files**: `gits_config.yml`.

You can find all three in the output folders of GITS. After completing the [build & install process](building.md) you will have multiple folders in the installation directory (default location: `<gits-root-folder>\<build-folder>\<install-folder>`):


| API                          | Windows             | Unix          |
| ---------------------------- | ------------------- | ------------- |
| :simple-intel: LevelZero     | `FilesToCopyL0`     | `LevelZero`   |
| :simple-opengl: OpenGL       | `FilesToCopyOGL`    | `OpenGL`      |
| :simple-opengl: OpenGL ES    | `FilesToCopyES`     | `OpenGL`      |
| :simple-opengl: OpenCL       | `FilesToCopyOCL`    | `OpenCL`      |
| :simple-vulkan: Vulkan       | `FilesToCopyVulkan` | `Vulkan`      |
| :simple-vulkan: Vulkan Layer | `VulkanLayer`       | `VulkanLayer` |

=== "Windows 64-bit Rel."
	```batch
	mkdir Release\x64
	cd Release\x64
	cmake -A x64 ..\..\
	cmake --build . --config Release
	```

=== "Linux 64-bit Rel."
	```bash
	mkdir -p Release/x64
	cd Release/x64
	cmake -DARCH=-m64 ../../
	# The `-j` option without an argument sets the number of threads automatically.
	# You can also set it manually, e.g., `-j8` for 8 threads.
	# Lower thread counts decrease RAM usage, which can help with OOM errors.
	cmake --build . --config Release -j
	```


## Record

To record the API-calls from an application you first need to identify the *correct folder* for your endeavor by choosen the right API and OS and use it in the recording process.

- On **Windows**, this is typically done by copying a DLL into the app directory.
- On **Linux**, by manipulating loader environment variables. 
- When recording **Vulkan**, it is also possible to use **GITS** as a **Vulkan layer** instead. This method works on both OSs.

Once the files are in place you simply start the application normally while gits initiates the recording. For information on how to adjust parameters of the recording please see the [configuration section](#configuration).

## Playback

To playback a binary stream you can use the commandline to run `gitsPlayer` with the arguments as needed. 

```batch
gitsPlayer BasicSample_7016_24_06_13_16_57_00.297
```

Use player's --help option to see basic usage info, or --hh all to list all available options. 

Here are some of the most commonly used arguments:

| argument                        | Windows                                                    |
| ------------------------------- | ---------------------------------------------------------- |
| `--help`                        | Basic usage info                                           |
| `--hh all`                      | List all available options                                 |
| `--stats`                       | Show stream statistics                                     |
| `--captureFrames`               | dump rendered frames to disk                               |
| `--captureFrames 23,420-450:10` | dump rendered frames `#23, #420, #430, #440, #450` to disk |
| `--captureDraws`                | dump rendered draw calls to disk                           |

For information on how to adjust parameters of the recording please see the [configuration section](#configuration).

## Configuration
Aside from various binaries there's also the pre-configured `gits_config.yml` inside the folder containing all default values. This file is used by both `gitsRecorder` as well as `gitsPlayer` and exposes all the dials and knobs the respective application offers. As YAML files are hierarchically structured finding the values/settings you need to customize should not be too dificult. 

> <span style="font-size:30px;">:writing_hand: TODO</span>  
> Talk about the most important settings

## Further information

You can find further information about the [configuration file](documentation/configuration/general_options.md) in the **documentation** section. 

Among other information the userguide contains [general recipes](userguides/recipes.md) as well as os-specific recipes for [windows](userguides/recipes_windows.md) and [linux](userguides/recipes_linux.md). There is also a collection of [application-specific issues](userguides/applicationspecific_issues.md).