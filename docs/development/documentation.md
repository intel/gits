---
icon: fontawesome/solid/pen-to-square
title: Write Docs
---

# Documentation

Our documentation is designed with the primary goal of assisting users in effectively utilizing **GITS**. It provides helpful resources, detailed explanations, and step-by-step setup guides to ensure users can easily understand and utilize the software. Additionally, the documentation is structured to be developer-friendly, leveraging the simplicity and flexibility of [MkDocs-Material](https://squidfunk.github.io/mkdocs-material/) and [Markdown](https://www.markdownguide.org/basic-syntax/) files. This approach ensures that updates and edits can be made effortlessly, empowering the development team to maintain accurate and up-to-date information with minimal overhead.

## Contributing

The most straightforward way to contribute to the documentation (especially for minor corrections, fixes, additions and edits) can be found on every page on the top right of the content area:

| Icon              | Function                                                                                                         |
| ----------------- | ---------------------------------------------------------------------------------------------------------------- |
| :material-pencil: | A link to github's file editing feature of the page itself.                                                      |
| :material-eye:    | Show the raw file content and (by doing so)reveals how certain features of mk-ddocs can be realized in markdown. |

For larger edits it's recommended to use a dedicated editor such as [MS VisualStudio Code](https://code.visualstudio.com/). It supports various extensions that can be particularly helpful when editing markdown files such as these:

??? tip "Recommended extensions (extensions.json)"

    ```json
    {
      "recommendations": [
        "bierner.github-markdown-preview",
        "bierner.markdown-mermaid",
        "yzhang.markdown-all-in-one",
      ]
    }
    ```

### Structure

[MkDocs-Material](https://squidfunk.github.io/mkdocs-material/) is the underlying documentation framework in use. Here is a brief overview of documentation relevant files & folders:

```
<root>/ 
├── docs/ 
│ ├── assets/ 
│ ├── [...]/ 
│ ├── index.md
│ └── [...]
│
├── Scripts/ 
│ └── docs/ 
│   ├── [...]
│   ├── run_mkdocs.py
│   └── strip_comments.py 
│
├── mkdocs.yml 
├── [...]
├── README.md 
```

| Type                            | Path                              | Description                                                      |
| ------------------------------- | --------------------------------- | ---------------------------------------------------------------- |
| :material-folder-home:          | `\`                               | Root folder of repository                                        |
| :material-file-edit:            | `\README.md`                      | Repository `README`, shown in the github repository page         |
| :material-file-cog-outline:     | `\mkdocs.yml`                     | Configuration of mkdocs                                          |
| :material-folder-edit:          | `\docs\`                          | Folder containing the `.md` files                                |
| :material-folder-file:          | `\docs\assets\`                   | Folder containing assets (`png`, `css`, `js`, ...)               |
| :material-file-edit:            | `\docs\index.md`                  | start-page of the `mkdocs` documentation                         |
| :material-folder:               | `\Scripts\docs\`                  | Folder containing additional scripts                             |
| :octicons-file-code-16:         | `\Scripts\docs\requirements.txt`  | Python packages required for `MkDocs-Material`, install with PIP |
| :fontawesome-regular-file-code: | `\Scripts\docs\run_mkdocs.py`     | Script to launch `mkdocs serve` with `strip comments` support    |
| :fontawesome-regular-file-code: | `\Scripts\docs\strip_comments.py` | Script that removes all HTML-comments from a markdown page       |


### Live preview

[MkDocs-Material](https://squidfunk.github.io/mkdocs-material/) supports a local live preview. In order to use it several python packages need to be installed as mkdocs itself is python based:

```bash
pip install -r Scripts/docs/requirements.txt
```
> Note: please be sure to use the correct version of pip.

Since HTML-comments can have a negative effect on some of the markdown-features we've added a (local) plugin for **mkdocs** to strip them. To make use of it the `PYTHONPATH` needs to include the `\Scripts\docs\` folder which is done by the `run_mkdocs.py` script:

```bash
python ./Scripts/docs/serve_mkdocs.py
```
The documentation can now be viewed in a browser by visiting [http://127.0.0.1:8000/](http://127.0.0.1:8000/).

### Adding new pages

To add a new page:

1. Place a blank markdown file in the `docs` folder, at the location where it should be integrated into the menu.
2. Add a menu-icon for the page using the [Markdown Frontmatter icon](https://squidfunk.github.io/mkdocs-material/reference/#setting-the-page-icon) feature. You can find a list of all supported icons in the [MkDocs-Material documentation](https://squidfunk.github.io/mkdocs-material/reference/icons-emojis/#search).
3. Add the file into the navigation section in the `\mkdocs.yml` file.


