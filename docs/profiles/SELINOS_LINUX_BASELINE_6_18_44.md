# `SELINOS_LINUX_BASELINE_6_18_44`

**Статус:** KABI M1 baseline — зафиксирован 13 августа 2026.  
**Назначение:** задать единственную воспроизводимую цель для последующей KABI-совместимости SeLinOS.  
**Не означает:** что SeLinOS уже загружает Linux `.ko` или что совместим с произвольным драйвером Linux.

> Linux kernel internal API и driver ABI не являются стабильным межверсионным контрактом. Поэтому SeLinOS не использует фразу «абсолютная совместимость» без указания exact source, configuration и test matrix.[1]

## Source identity

| Поле | Зафиксированное значение |
|---|---|
| Upstream | [Linux Kernel Archives](https://www.kernel.org/) |
| Version | `6.18.44` longterm |
| Architecture | `x86_64` |
| Official source archive | `linux-6.18.44.tar.xz` |
| Archive SHA-256 | `0f72d938f06828e82c90405174fe572287db7bfe089e2fc46572a99a7f240d43` |
| Source archive URL | `https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.18.44.tar.xz` |
| Runtime target | QEMU PC99 with QEMU `edu`; hardware profiles added separately |
| SeLinOS runtime | seL4; no Linux kernel, Linux VM or LKL at runtime |

The exact source archive is pinned because changing any patch level can alter headers, exported symbol prototypes, `Module.symvers` CRCs, struct layouts, feature gates or module metadata. The official Linux build documentation states that `CONFIG_MODVERSIONS` uses CRC values for exported symbol prototypes as an ABI-consistency check.[2]

## Exact configuration input

The baseline begins from the official `arch/x86/configs/x86_64_defconfig` and applies the tracked fragment [`selinos-linux-6.18.44-kabi.fragment`](selinos-linux-6.18.44-kabi.fragment). It is then normalized using `make olddefconfig`; the full normalized config is the contract, not the fragment alone.

| Artifact | SHA-256 | Purpose |
|---|---|---|
| [`selinos-linux-6.18.44-kabi.config`](selinos-linux-6.18.44-kabi.config) | `486679845b4da93de3013c29464b39cdbbc5491e9398b35d2eb07c855edec5f1` | Exact KABI configuration fingerprint |
| [`selinos-linux-6.18.44-kabi.fragment`](selinos-linux-6.18.44-kabi.fragment) | Version-controlled text fragment | Module, PCI, virtio and module-signature target options |
| [`selinos-linux-6.18.44-kabi-manifest.json`](selinos-linux-6.18.44-kabi-manifest.json) | `5aa9535c043dc9758995001eba0aa51d02385f985f1e2a92908afd1bb439f4de` | Generated list of all 12,748 exported symbols, CRCs and build-artifact hashes |

The selected profile includes `CONFIG_MODULES=y`, `CONFIG_MODULE_UNLOAD=y`, `CONFIG_MODVERSIONS=y`, `CONFIG_MODULE_SIG=y`, `CONFIG_MODULE_SIG_FORCE=y`, `CONFIG_MODULE_SIG_SHA256=y`, `CONFIG_KALLSYMS_ALL=y`, `CONFIG_PCI=y`, `CONFIG_PCI_MSI=y`, `CONFIG_VIRTIO_PCI=y`, `CONFIG_VIRTIO_BLK=m`, and `CONFIG_VIRTIO_NET=m`. Module signature verification follows the Linux model: a module must carry a signature accepted by the target trusted-key policy when enforcement is enabled.[3]

## Compatibility contract

A future binary module is **in scope** only if it passes every identity gate below.

| Gate | Required value in M1 | Verification evidence |
|---|---|---|
| Source | Linux `6.18.44` source archive above | archive SHA-256 |
| Architecture | ELF x86_64 `ET_REL` | ELF parser result |
| Kconfig | exact normalized `.config` above | `.config` SHA-256 |
| Build artifacts | generated `include/generated`, `Module.symvers`, exported-symbol table | artifact hashes recorded in manifest |
| Compiler/toolchain | exact compiler ID/version and relevant flags | module `vermagic` plus manifest |
| Symbol ABI | names, prototypes, version CRCs and relocations matched | generated KABI manifest |
| Signature | trust anchor and signature policy matched | loader verification record |
| Device profile | explicitly granted PCI/MMIO/IRQ/DMA resources | capability-grant log |
| Behaviour | driver-specific test case passes in QEMU/hardware matrix | reproducible test log |

The first M1 candidate should be an intentionally small external module compiled solely against this baseline and designed to exercise module metadata, symbol references, relocations, `module_init/module_exit`, PCI binding and the already demonstrated QEMU edu resource path. A general subsystem driver is not an acceptable first proof.

## KABI boundary and next generated artifacts

SeLinOS has generated the first artifact set from this exact config: `utsrelease.h`, full `vmlinux.symvers`, `System.map`, `modules.builtin.modinfo`, `vmlinux` and the machine-readable manifest. The manifest contains 12,748 exported symbols and their version CRCs; its embedded artifact hashes include `vmlinux.symvers` SHA-256 `8f9f05048e5dc3ee3a024ff5443189f73188db5159b325b5955905c9f38a9d2a`. The next phase adds the ELF module parser, relocation allow-list, layout manifest for the chosen driver subset and signature trust manifest, and then rejects any incoming module that differs from the pinned profile.

## Reproduction

```bash
# Working project root
cd selinos

# The pinned full config is supplied by this repository.
sha256sum docs/profiles/selinos-linux-6.18.44-kabi.config
# Expected: 486679845b4da93de3013c29464b39cdbbc5491e9398b35d2eb07c855edec5f1
```

## References

[1] [The Linux Kernel Driver Interface — Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/process/stable-api-nonsense.html).

[2] [Building External Modules — Linux Kernel Documentation](https://docs.kernel.org/kbuild/modules.html).

[3] [Kernel module signing facility — Linux Kernel Documentation](https://docs.kernel.org/admin-guide/module-signing.html).
