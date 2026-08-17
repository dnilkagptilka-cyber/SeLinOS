# SeLinOS Phase 15 immutable VFS version M9 checkpoint

**Status:** verified on the default x86_64/PC99 QEMU TCG image. M9 adds one fixed CPIO member and one exact Linux syscall bridge. It does not add a general filesystem interface.

| Layer | Evidence-bound M9 behavior |
|---|---|
| Fixture source | `generate_romfs_cpio_fixture.py` deterministically appends `/selinos-version` with the eight-byte data `SELINOS9`. |
| CPIO parser gate | `romfsd` validates the generated member as a regular file before serving requests. |
| Server projection | The file is exposed only as selector `4` and fixed FD `6`; the packed pathname remains constrained to 16 printable bytes. |
| Linux ABI bridge | The isolated probe proves `openat(AT_FDCWD, "/selinos-version", 0, 0) → 6`, `read(6, one mapped page, 8) → 8`, payload `SELINOS9`, then `close(6) → 0`. |

> **Scope boundary:** M9 is a third literal immutable record. It is not arbitrary pathname lookup, directory traversal, metadata, a descriptor table, stream position, persistent storage, package extraction or package-management support.

## Reproduction

```bash
cd /home/ubuntu/helixos
./tools/generate_romfs_cpio_fixture.py
cmake --build build
qemu-system-x86_64 -accel tcg,thread=single -icount 1 -cpu max \
  -nographic -serial mon:stdio -m size=1G \
  -kernel build/images/kernel-x86_64-pc99 \
  -initrd build/images/selinos-root-image-x86_64-pc99
./tools/verify_linux_vfs_version_bridge_m9.py
```

The captured QEMU log contains the following marker only after the probe has passed its exact `openat`/`read`/`close` sequence and reached its verified `exit(0)` fault.

```text
SeLinOS ABI M9: exact immutable /selinos-version open/read/close bridge mediated.
```

| Still unimplemented | Why M9 is insufficient |
|---|---|
| Arbitrary files, directory and metadata APIs | The server recognizes only a fixed archive projection and literal paths. |
| Persistent writes and package database | The only mutable object is the reset-on-open volatile word; no block I/O or persistent filesystem exists. |
| Linux runtime/package installation | TLS, clone/process lifecycle, ELF dynamic linking, storage, trusted transport and maintainer-script environment remain absent. |
| Network-backed repository access | Network N1 preserves scalar-only root metadata under the zero-IOMMU stop rule; it confers no NIC authority. |

The M9 image, generator, generated archive, protocol, server, bridge, probe and serial log are cryptographically bound by `tests/artifacts/selinos_linux_vfs_version_bridge_m9.verification.json` and checked by `tools/verify_linux_vfs_version_bridge_m9.py`.
