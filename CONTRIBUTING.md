# Contributing

Contributing to **GITS** is subject of our [code of conduct](CODE_OF_CONDUCT.md).

## Submitting issues

### Security issues

Do not submit security issues here on GitHub or in any other public channel! Instead, see the [SECURITY.md](SECURITY.md) file for instructions.

### Other issues

Submit regular issues here on GitHub. Include relevant data like GITS version and architecture, hardware and OS/software info, etc.

For now our main bugtracker is an internal one, so don't be surprised if a known bug is not filed on GitHub.

## Contributing code

We accept pull requests here on GitHub. Before starting work on a PR, please let us know what you want to work on. This will avoid duplicating efforts and will ensure that your work aligns with our vision and direction.

Please write proper commit messages. (A popular howto: [cbea.ms/git-commit](https://cbea.ms/git-commit/).) Please create commits that are _logical_, _discrete_, _easy_ to review steps toward your end goal.

### License

**GITS** is licensed under the terms in [LICENSE.md](LICENSE.md). By contributing to the project, you agree to the license and copyright terms therein and release your contribution under these terms.

### Sign your work

Please use the sign-off line at the end of the patch. Your signature certifies that you wrote the patch or otherwise have the right to pass it on as an open-source patch. The rules are pretty simple: if you can certify the below (from [developercertificate.org](http://developercertificate.org/)):

```
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.
660 York Street, Suite 102,
San Francisco, CA 94110 USA

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.

Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

Then you just add a line to every git commit message:

    Signed-off-by: Joe Smith <joe.smith@email.com>

Use your real name (sorry, no pseudonyms or anonymous contributions.)

If you set your `user.name` and `user.email` git configs, you can sign your
commit automatically with `git commit -s`.
