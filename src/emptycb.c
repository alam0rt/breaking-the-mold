#include "common.h"

/* No-op callback installed at the +0x1C slot of BLB entity vtable D_80012758
 * (paired with func_80082EE8 at +0x14, and BLBEntityDestroyCallback_2758 at
 * +0x0C). By analogy with vtable D_800103AC (UploadEntityTextureIfDirty in
 * the same slot), this is likely a no-op texture-upload callback —
 * unverified. Likely should be `NoopUploadTexture`. */
void func_80082EE0(void) {
}

/* No-op callback installed at the +0x14 slot of BLB entity vtable D_80012758.
 * By analogy with vtable D_800103AC (UpdateEntityRender in the same slot),
 * this is likely a no-op render callback — unverified. Likely should be
 * `NoopRender`. NOT the same as `StubReturnZero` @ 0x80020288. */
void func_80082EE8(void) {
}
