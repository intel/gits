---
icon: material/cogs
---
# Configuration & Arguments

The configuration and relevant enums of GITS are created at build time using mako-templates based on the metafiles

- `common\configuration\codegen\metafiles\config.yml`and 
- `common\configuration\codegen\metafiles\enums.yml`. 

## Usage

### Configuration file

The configuration file does not have to be "complete", the loader also supports *partial config files*. This is as every option of the configuration is initialized with the default value set in the configuration metafile.

```YAML
Common:
  Player:
    Fullscreen: true
```

### CLI Arguments

The CLI argument(s) for an option are defined in the configuration metafile by the attribute `Arguments`.

Additionally _every_ option (that is not derived) can be set by a _hidden CLI argument_ in the form of a keypath:

```bash
gitsPlayer --Common.Player.Fullscreen=true
```

### Environment

_Every_ option (that is not derived) can be set by an evironment variable in the form of a keypath with prefix `GITS_`:

```bash
export GITS_COMMON_PLAYER_FULLSCREEN=true
```

## Developer guide
### Enums

Here's the structure that define an enum:

```yaml
- Name: string                  # req.     cpp name
  Description: string           #     opt. short: one sentence
  LongDescription: string       #     opt. long:  for md-file
  Values:                       # req.     list of enum values
    - Value: string             # req.     cpp name
      Description: string       #     opt. short: one sentence
      LongDescription: string   #     opt. long:  for md-file
      Labels: [string]          #     opt. alternative string names
```

The `Value`-string defines the cpp value name. The system generates a string-to-enum mapping `stringTo<EnumName>()` based on the value's attributes:
- if `Labels` is set all list entries can be used and are converted to the `Value`.
- otherwise the `Value` is converted as follows:
  ```py
  label_parts = value.split('_')
  label_parts = [part.capitalize() for part in label_parts]
  self.labels = [''.join(label_parts)]
  ```

### Configuration

#### Option
Here's the structure to define a configuration option:

```yaml
- Name: string              # req.     cpp and config name
  Type: string              # req.     cpp type - can't be "Group"
  Default: string           # req.     cpp default value
  ConfigName: string        #     opt. option name in the configuration name
  Description: string       #     opt. short: one sentence
  LongDescription: string   #     opt. long:  for md-file
  Accessibility: string     #     opt. Ways to access/set the option
  AccessLevel: string       #     opt. Intended audience category
  NumericFormat: string     #     opt. Numeric format of the default value
  Arguments: [string]       #     opt. cli argument(s) to set the option
  Tags: [string]            #     opt. tag(s) for help filtering.
  OSVisibility: [string]    #     opt. OSs that support this option.

  DefaultPerPlatform:       #     opt. additional defaults
      - {platform}: string  #     opt. if `platform` matches

  DefaultCondition:         #     opt. additional defaults
      - {set_value}: string #     opt. if `set_value` is present
```

| Name               | Description                                                                                                                                                                                                                                 |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Name               | The cpp variable name and config name.                                                                                                                                                                                                      |
| Type               | The cpp type. *Can not be "Group" :wink:.*                                                                                                                                                                                                  |
| Default            | The default value.                                                                                                                                                                                                                          |
| ConfigName         | Custom option name used in the configuration file.                                                                                                                                                                                          |
| Description        | A short description, e.g. for the CLI help.                                                                                                                                                                                                 |
| LongDescription    | A long description, e.g. for the markdown documentation.                                                                                                                                                                                    |
| Accessibility      | The way this option is accessed: `Derived`,`ArgumentOnly`. A `Derived` option is not read from the configuration file and can not be set a CLI argument. A `ArgumentOnly` option can be set via CLI argument, but not from the config file. |
| AccessLevel        | Intended audience: `Beginner`,`Advanced`,`Expert`,`Developer`(default).                                                                                                                                                                     |
| NumericFormat      | Output format for default value: `Hexadecimal`,`Binary`,``Decimal`(default).                                                                                                                                                                |
| Arguments          | (case-sensitive) CLI-Argument(s) to set this option's value.                                                                                                                                                                                |
| Tags               | Tag(s) for help filtering and future use-cases. All groups "above" a value - the path segments - are added to the tags automatically.                                                                                                       |
| OSVisibility       | OS(s) that use the option: `WINDOWS`,`X11`.                                                                                                                                                                                                 |
| DefaultPerPlatform | Specify defaults for specifi platforms. The value set by `Default` is a fallback, when no platform matched.                                                                                                                                 |
| DefaultCondition   | Specify defaults if certain conditions are _present_. Is evaluated after `DefaultPerPlatform`. When unmatched, the value set by `Default` is used.                                                                                          |

#### Group

The following defines a configuration option:

```yaml
- Name: string              # req.     cpp and config name
  Type: "Group"             # req.     Marks this to be a group.
  Options: [Option]         # req.     A list of options in a group.
  InstanceName: string      #     opt. Custom cpp name
  Description: string       #     opt. short: one sentence
  LongDescription: string   #     opt. long:  for md-file
  OSVisibility: [string]    #     opt. OSs that support this option.
```

| Name            | Description                                              |
| --------------- | -------------------------------------------------------- |
| Name            | The cpp struct, (lowercased) instance and config name.   |
| Type            | **Must be "Group".**                                     |
| Options         | The options within the group.                            |
| InstanceName    | Custom cpp instance name.                                |
| Description     | A short description, e.g. for the CLI help.              |
| LongDescription | A long description, e.g. for the markdown documentation. |
| OSVisibility    | OS(s) that use the group: `WINDOWS`,`X11`.               |


### Generation

The entrypoint for the generation is `common\configuration\codegen\scripts\generate.py`:
```
generate.py --step <Step>                       # generation step to run
            --configYML <configurationMetafile> # path to configuration metafile
            --enumYML <enumMetafile>            # path to enum metafile
            --outDir <outputFolder>             # output base folder
            --platform <platformString>         # [win32, lnx_32, lnx_64, lnx_arm]
            --installpath <installationpath>    # where gits is installed
            --compute                           # flag to indicate compute generation
```

where `Step` can be one of the following:

| `Step`                 | Description                                     |
| ---------------------- | ----------------------------------------------- |
| `Enum`                 | 1. Generate all enum related cpp files.         |
| `Configuration`        | 2. Generate the configuration system cpp files. |
| `Argumentparser`       | 3. Generate the argument parser cpp files.      |
| `DefaultConfiguration` | 4. Generate the default configuration file.     |

*Note: the steps can be run in an arbitrary order. However, the steps with cpp files build up and higher numbered steps require to lower ones to work.*

