#!/usr/bin/env bash
# Regenerate declared-fixture M5-M10 Virtio transcripts after a shared rebuild.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LOG="$ROOT/tests/artifacts/phase80_special_virtio_replays.log"
: > "$LOG"

replay() {
    local stem=$1
    local build_dir=$2
    local raw=$3
    local serialize_hashes=$4
    local transcript="$ROOT/tests/artifacts/${stem}.boot.log"
    local fixture="$ROOT/tests/artifacts/${raw}"
    local before after status command

    test -f "$fixture"
    before=$(sha256sum "$fixture" | awk '{print $1}')
    command="timeout 12s qemu-system-x86_64 -cpu max -nographic -serial mon:stdio -m size=1G -drive if=none,format=raw,file=../tests/artifacts/${raw},id=selinosvd0 -device virtio-blk-pci,disable-legacy=on,drive=selinosvd0 -kernel images/kernel-x86_64-pc99 -initrd images/selinos-root-image-x86_64-pc99"
    rm -f "$transcript"
    set +e
    (
        cd "$ROOT/$build_dir"
        script -qefc "$command" "$transcript"
    ) >/dev/null 2>&1
    status=$?
    set -e
    if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
        printf 'FAIL %s qemu_exit=%s\n' "$stem" "$status" | tee -a "$LOG"
        return 1
    fi
    after=$(sha256sum "$fixture" | awk '{print $1}')
    if [ "$before" != "$after" ]; then
        printf 'FAIL %s immutable_fixture_changed before=%s after=%s\n' "$stem" "$before" "$after" | tee -a "$LOG"
        return 1
    fi
    if [ "$serialize_hashes" = yes ]; then
        printf 'fixture_before=%s\nfixture_after=%s\n' "$before" "$after" >> "$transcript"
    fi
    printf 'PASS %s qemu_exit=%s fixture=%s\n' "$stem" "$status" "$before" | tee -a "$LOG"
}

replay selinos_virtio_queue_zero_m5 build-virtio-queue-zero-probe selinos_virtio_queue_zero_m5.raw no
replay selinos_virtio_queue_layout_m6 build-virtio-queue-layout-probe selinos_virtio_queue_layout_m6.raw yes
replay selinos_virtio_queue_enable_m7 build-virtio-queue-enable-probe selinos_virtio_queue_enable_m7.raw yes
replay selinos_virtio_notification_observation_m8 build-virtio-notification-observation-probe selinos_virtio_notification_observation_m8.raw yes
replay selinos_virtio_zero_descriptor_notification_m9 build-virtio-zero-descriptor-notification-probe selinos_virtio_zero_descriptor_notification_m9.raw yes
replay selinos_virtio_zero_index_observation_m10 build-virtio-zero-index-observation-probe selinos_virtio_zero_index_observation_m10.raw yes
printf 'SPECIAL_VIRTIO_REPLAY_PASS count=6\n' | tee -a "$LOG"
