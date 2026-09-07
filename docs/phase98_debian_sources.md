# Phase 98 research notes

## Official Debian 13.6 release information

Debian's official release information states that Debian 13.6 was released on July 11, 2026, and identifies the release as the sixth update of stable Debian 13 (trixie). The official news page states that a point release updates included packages rather than constituting a new Debian major version. The release information lists amd64 among the architectures supported at the initial trixie release.

Sources:

1. https://www.debian.org/News/2026/20260711 — Updated Debian 13: 13.6 released.
2. https://www.debian.org/releases/trixie/ — Debian “trixie” Release Information.

## Compatibility interpretation

These sources establish the target distribution/version and supported amd64 target, but they do not establish that SeLinOS can boot or execute Debian userland. The project must separately demonstrate ELF loading, process initialization, syscall surface, memory/file/network services, and runtime behavior. The Phase 98 auxv witness is therefore evidence for one process-initialization sub-contract only.
