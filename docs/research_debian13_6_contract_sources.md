# Debian 13.6.0 compatibility-contract research notes

## Authoritative release facts

Debian’s official release information identifies **Debian 13.6** as the `trixie` stable point release issued on **11 July 2026**. Debian describes it as the sixth update of Debian 13 and states that it is not a new Debian 13 version: it updates some included packages, and installed systems update through current Debian mirrors. The release information lists `amd64` as a supported architecture. These facts support a pinned target of **Debian 13.6 amd64**, but not an unspecified moving mirror state.

The SeLinOS compatibility contract therefore needs a repository snapshot identity in addition to the distribution name and point-release number. The contract must state the suite/component set, architecture, snapshot timestamp or archive manifest hashes, package indices, `Release`/`InRelease` signatures, and the exact `.deb` corpus. “All Debian 13.6 packages” is otherwise not reproducible because point-release package updates are delivered through mirrors and repository content changes.

## Sources

1. Debian Project, “Updated Debian 13: 13.6 released”, 11 July 2026: <https://www.debian.org/News/2026/20260711>.
2. Debian Project, “Debian ‘trixie’ Release Information”: <https://www.debian.org/releases/trixie/>.
3. Debian Project, “Debian 13 — Release Notes”: <https://www.debian.org/releases/trixie/releasenotes>.

## Immediate implication for SeLinOS

A native, no-Linux-kernel SeLinOS target can be specified objectively as an `amd64` user-space and package corpus compatibility contract. It cannot honestly be treated as a claim that every package will work until the contract identifies its archive snapshot and each package class has an executable evidence result. Kernel-image, kernel-module, firmware, installer, and packages that require hardware or a Linux-specific kernel interface need named treatment in the contract instead of silent exclusion.

## Native package-management obligations

Debian Policy specifies that package-management invokes executable `preinst`, `postinst`, `prerm`, and `postrm` maintainer scripts during installation, upgrade, removal, and recovery. Scripts must report success/failure by exit status and must be idempotent for recovery. The policy defines partial and failure package states and explicit error-unwind paths. Consequently, a genuine `dpkg` compatibility claim requires more than parsing `.deb`: it needs durable filesystem updates, executable interpreter/runtime support, process execution and exit status, environment/path handling, permission metadata, package status database semantics, dependency ordering, interruption/recovery behavior, and a policy-compatible implementation of the package-manager visible state machine.

Debian Reference for `trixie` describes source-list locations including deb822 `.sources`, shows the canonical component set `main non-free-firmware contrib non-free` for the `trixie` and `trixie-security` sources, and identifies `dpkg` as the file-based low-level package manager and `apt` as the CLI package-management front end. It records additional front ends and tools including `aptitude`, `tasksel`, `unattended-upgrades`, `synaptic`, and `apt-utils`; their native compatibility must be separately evidenced rather than assumed from a successful `apt` test.

APT’s archive-authentication documentation states that Release-file signatures are checked, repositories need recent authentication information for unimpeded use, and unsigned repositories are refused by default. The SeLinOS contract must therefore include a native crypto/keyring implementation, Release/InRelease signature verification, metadata/package checksum verification, freshness/error handling, DNS/transport/TLS policy, and fail-closed repository trust behavior. Disabling repository authentication is not an acceptable route to an `apt` compatibility claim.

## Additional sources

4. Debian Policy Manual v4.7.4.1, “Package maintainer scripts and installation procedure”: <https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html>.
5. Debian APT, `apt-secure(8)`: <https://manpages.debian.org/unstable/apt/apt-secure.8.en.html>.
6. Debian Reference, “Debian package management”: <https://www.debian.org/doc/manuals/debian-reference/ch02.en.html>.

## Immutable corpus and archive-format requirements

The Debian Snapshot service states that it provides past and current Debian archive packages by date/version and is usable as an APT repository. It exposes timestamped archive states, including the main Debian and Debian Security archives. A SeLinOS Debian 13.6 compatibility corpus can therefore be made reproducible by recording selected Snapshot timestamps separately for `debian` and `debian-security`, together with the fetched signed `InRelease` files and every index/package hash.

The Debian repository-format documentation specifies that an APT client obtains `InRelease` or `Release`/`Release.gpg`, validates the signed index hashes, reads architecture/component `Packages` indices, and verifies package files against SHA-256 or SHA-512 data. It documents `Date`, `Valid-Until`, `Architectures`, `Components`, `Suite`, `Codename`, and package-index fields. It also notes `xz`, gzip, and bzip2 index compression expectations. These are native client requirements for the SeLinOS archive service, not optional mirror conveniences.

## Additional sources

7. Debian Snapshot service: <https://snapshot.debian.org/>.
8. Debian FTP archive README: <https://ftp.debian.org/debian/README.html>.
9. Debian Repository Format: <https://wiki.debian.org/DebianRepository/Format>.
