# DirectX - Portability

GITS streams may not be portable accross GPU architectures due to different feature sets or allocation sizes / offsets.

[Placed resources](https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources#placed-resources) are the main source of incompatibility due to platform specific GPU sizes and alignments.

The Portability Layer can be used to help with resource placement compatibility. It can be used to (1) generate a `resourcePlacementData.dat` file containing resource sizes and offsets and to (2) pre-load a `resourcePlacementData.dat` to resize all the placement heaps set the correct offsets for placed resources.

## Usage

To playback a stream from **Platform A** (🖥️) on **Platform B** (💻):

1. Generate `resourcePlacementData.dat` on **Platform A** (🖥️)

   - Enable Portability Layer in `gits_config.yml` by setting `DirectX.Features.Portability.Enabled` to `true`
   - `resourcePlacementData.dat` can be generated on capture or playback (using either `StorePlacedResourceDataOnCapture` or `StorePlacedResourceDataOnPlayback`)
   - `resourcePlacementData.dat` will be written next to `stream.gits2`

2. Playback stream on **Platform B** (💻)

   - Copy `resourcePlacementData.dat` next to `stream.gits2`
   - Enable Portability Layer in `gits_config.yml` by setting `DirectX.Features.Portability.Enabled` to `true`

## Notes

The Portability Layer will not be able to solve all the issues that may occur by playing back streams from different GPU vendors. It is recommended that you capture a separate stream on each platform.
