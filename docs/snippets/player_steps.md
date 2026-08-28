To playback a binary stream you can run `gitsPlayer` with the arguments as needed, e.g.

```batch
gitsPlayer BasicSample_7016_24_06_13_16_57_00.297.gits2
```

Use player's --help option to see basic usage info, or --hh all to list all available options.

Here are some of the most commonly used arguments:

| Argument                        | Windows                                                    |
| ------------------------------- | ---------------------------------------------------------- |
| `--help`                        | Basic usage info                                           |
| `--stats`                       | Show stream statistics                                     |
| `--captureFrames`               | Dump rendered frames to disk                               |
| `--captureFrames 23,420-450:10` | Dump rendered frames `#23, #420, #430, #440, #450` to disk |
| `--captureDraws`                | Dump rendered draw calls to disk                           |

The file `gits_config.yml` configures GITS and the playback options.