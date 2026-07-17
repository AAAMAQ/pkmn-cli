# Linux packaging

Configure and build a release, then run:

```sh
cpack --config build/CPackConfig.cmake -B build/packages
```

Linux configuration produces `.tar.gz` and `.deb` packages. Package contents
are the same installed `pkmn` binary, documentation, man page, SBOM, and
identity-checked Red template verified by the normal release test.
