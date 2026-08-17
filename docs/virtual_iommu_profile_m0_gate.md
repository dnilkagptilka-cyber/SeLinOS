# SeLinOS virtual IOMMU profile M0 gate

**Result:** the first QEMU virtual-IOMMU profile is **not accepted** as a SeLinOS hardware-containment evidence environment.

A bounded experiment ran the unchanged default SeLinOS image on QEMU 8.2.2 TCG using the documented Q35/intel-IOMMU configuration:

```text
-machine q35
-accel tcg,thread=single
-device intel-iommu,intremap=on,caching-mode=on
```

The system booted to the seL4 root task, but its ACPI report remained:

```text
ACPI: 0 IOMMUs detected
```

| Test condition | Result | Interpretation |
|---|---|---|
| QEMU provides `intel-iommu` device model | Available in QEMU 8.2.2 | A candidate vIOMMU mechanism exists. |
| Q35 QEMU TCG boot | Passed | The unchanged SeLinOS image can boot on this profile. |
| Q35/KVM split-irqchip profile | Unavailable in this sandbox | `/dev/kvm` is absent; the documented KVM-specific vIOMMU profile cannot be tested here. |
| Pinned seL4 IOMMU discovery | Failed | No discovered IOMMU can support an IOSpace containment claim. |
| IOMMU map/unmap test | Not attempted | Unsafe and meaningless until discovery succeeds. |
| Driver MMIO/IRQ/DMA delegation | Not attempted | Authority remains withheld. |

> **Decision:** preserve QEMU PC99/TCG for functional regressions and do not update any containment evidence from this Q35 profile. This sandbox also has no `/dev/kvm`, so it cannot test the QEMU guide's KVM + split-irqchip profile. A later VM profile is admissible only if the pinned seL4 boot log itself reports one or more IOMMUs before an IOSpace capability or mapping operation is considered.

The QEMU VT-d guide requires a Q35 machine and `intel-iommu`; it notes additional properties for interrupt remapping and virtio DMA-remapping scenarios.[1] This trial met the minimal device-model condition but failed SeLinOS’s stricter guest-detection condition.

## References

[1]: https://wiki.qemu.org/Features/VT-d "QEMU VT-d emulation guide"
