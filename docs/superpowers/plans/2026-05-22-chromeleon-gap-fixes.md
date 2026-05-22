# Chromeleon Gap Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the verified gaps between `docs/superpowers/specs/2026-04-21-chromium-multisession-fork-design.md`, `docs/superpowers/plans/2026-04-21-chromeleon-implementation.md`, and the current Chromeleon implementation.

**Architecture:** Keep the Brave-style overlay plus thin patch model. Fix only the missing data-flow edges: browser overlay GN linkage, tab lifecycle cleanup, Browser-to-Renderer seed receiver binding, real Blink readback hook placement, and context-menu resource wiring. Do not introduce a broad rewrite of existing overlay services.

**Tech Stack:** Chromium M135, C++20, GN/Ninja, Mojo associated interfaces, Chromium Views, Python patch validation scripts.

---

## File Structure

- Modify: `build/overlay_gn.patch`
  - Connect `multi_session_overlay_all` to the actual `chrome/browser` build target instead of only defining an unused group.
- Modify: `chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.h`
  - Add `WebContentsDestroyed()` override so the same WebContents observer owns session cleanup.
- Modify: `chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.cc`
  - Call `EphemeralSessionManager::DestroySessionForTab()` on tab destruction and keep the existing seed lookup path.
- Modify: `chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h`
  - Add an explicit associated-interface binding method for `FingerprintSeedReceiver`.
- Modify: `chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.cc`
  - Bind the Mojo receiver endpoint to the existing `HeapMojoAssociatedReceiver`.
- Create: `patches/0010-blink-fingerprint-seed-receiver-binding.patch`
  - Register the renderer-side associated interface and route it to `FingerprintNoiseSource`.
- Modify: `patches/0003-blink-canvas-readback-hook.patch`
  - Replace dummy hunk content with a patch against the real M135 Canvas readback return path.
- Modify: `patches/0004-blink-webgl-readpixels-hook.patch`
  - Replace dummy hunk content with a patch against the real M135 `readPixels` data path.
- Modify: `patches/0005-chrome-render-view-context-menu-hook.patch`
  - Add or reference a real string resource definition for `IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION`.
- Modify: `patches/series`
  - Add `0010-blink-fingerprint-seed-receiver-binding.patch` after `0002-blink-navigator-webdriver-hook.patch` and before readback hooks.
- Modify: `build/ci_patch_dryrun.py`
  - Add a guard that fails if patch files still contain dummy markers such as `Dummy context`, `pixels_span`, or `@@ -1,0` placeholder hunks.
- Test: `chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_unittest.cc`
  - Add an observer cleanup test around `DestroySessionForTab()` if a practical fake WebContents path exists; otherwise add a direct regression test for `OnPartitionDestroyed` notification through manager cleanup helpers introduced in Task 2.
- Test: `chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source_unittest.cc`
  - Add a receiver binding smoke test if Blink test support exposes an associated receiver endpoint; otherwise keep existing unit tests and rely on patch dry-run plus build for binding.

## Task 1: Connect Browser Overlay To The Build Graph

**Files:**

- Modify: `build/overlay_gn.patch`
- Verify: Chromium M135 `chrome/browser/BUILD.gn` after patch application

- [ ] **Step 1: Inspect the M135 target location**

Run inside the Chromium source checkout after `build/sync_overlay.py`:

```bash
grep -n 'source_set("browser")\|static_library("browser")\|group("multi_session_overlay_all")' "$CHROMIUM_SRC/chrome/browser/BUILD.gn"
```

Expected: one browser target definition and no existing `multi_session_overlay_all` unless patches are already applied.

- [ ] **Step 2: Update the patch to add the dependency to the browser target**

Replace the browser-side hunk in `build/overlay_gn.patch` with a hunk that both defines the group and appends it to the browser target. The exact context lines must come from M135, but the resulting patch must add this semantic content:

```gn
# CHROMELEON OVERLAY START
group("multi_session_overlay_all") {
  deps = [
    "//chromium_src/overlay/chrome/browser/multi_session",
    "//chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader",
    "//chromium_src/overlay/chrome/browser/ui/views/tab_grid",
    "//chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog",
  ]
}
# CHROMELEON OVERLAY END
```

And inside the actual browser target:

```gn
deps += [ ":multi_session_overlay_all" ]
```

- [ ] **Step 3: Verify overlay targets are reachable**

Run:

```bash
python3 build/ci_patch_dryrun.py
```

Expected: `All 8 patches validated.` or `All 9 patches validated.` after Task 4 adds patch 0010.

- [ ] **Step 4: Verify with Chromium patch application**

Run:

```bash
python3 build/sync_overlay.py
python3 build/apply_patches.py
```

Expected: all patches apply successfully. If `overlay_gn.patch` is applied by a separate build step, run that exact project command and verify `chrome/browser/BUILD.gn` contains `deps += [ ":multi_session_overlay_all" ]` in the browser target.

- [ ] **Step 5: Commit**

```bash
git add build/overlay_gn.patch
git commit -m "fix(build): overlay ターゲットを browser ビルドに接続"
```

## Task 2: Wire Tab Destruction To Session Cleanup

**Files:**

- Modify: `chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.h`
- Modify: `chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.cc`
- Test: `chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_unittest.cc`

- [ ] **Step 1: Add the observer override in the header**

Update `FingerprintSeedDelivery`:

```cpp
class FingerprintSeedDelivery
    : public content::WebContentsObserver,
      public content::WebContentsUserData<FingerprintSeedDelivery> {
 public:
  ~FingerprintSeedDelivery() override;

  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost* rfh) override;
  void WebContentsDestroyed() override;
```

- [ ] **Step 2: Implement cleanup through the existing manager**

Add to `fingerprint_seed_delivery.cc`:

```cpp
void FingerprintSeedDelivery::WebContentsDestroyed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* wc = web_contents();
  if (!wc)
    return;

  auto* profile = Profile::FromBrowserContext(wc->GetBrowserContext());
  auto* mgr = EphemeralSessionManagerFactory::GetForProfile(profile);
  CHECK(mgr);
  mgr->DestroySessionForTab(wc);
}
```

- [ ] **Step 3: Add a regression test for destroy notification**

If the existing test fixture can create a `content::WebContents`, add this test to `ephemeral_session_manager_unittest.cc`:

```cpp
TEST_F(EphemeralSessionManagerTest, DestroySessionForTab_NotifiesObserver) {
  MockObserver obs;
  manager_->AddObserver(&obs);
  const auto h = manager_->CreateSessionForNewTab();

  auto contents = content::WebContentsTester::CreateTestWebContents(
      profile_.get(), nullptr);
  contents->GetController().LoadURLWithParams(
      content::NavigationController::LoadURLParams(GURL("https://example.test/")));
  contents->GetSiteInstance()->SetStoragePartitionConfigForTesting(h.config);

  manager_->DestroySessionForTab(contents.get());

  EXPECT_EQ(obs.destroyed_count, 1);
  EXPECT_EQ(obs.last_destroyed_id, h.partition_id);
  EXPECT_EQ(manager_->GetSessionCountForTesting(), 0u);
  manager_->RemoveObserver(&obs);
}
```

If Chromium M135 test APIs do not expose `SetStoragePartitionConfigForTesting`, do not invent a brittle fake. Instead, add a private test-only helper in `EphemeralSessionManager`:

```cpp
void DestroySessionForPartitionConfigForTesting(
    const content::StoragePartitionConfig& config) {
  DestroySessionForPartitionConfig(config);
}
```

And move the core erase logic from `DestroySessionForTab()` into:

```cpp
void EphemeralSessionManager::DestroySessionForPartitionConfig(
    const content::StoragePartitionConfig& config) {
  const auto it = sessions_.find(config.partition_name());
  if (it == sessions_.end())
    return;
  if (it->second.config != config)
    return;

  const std::string partition_id = it->first;
  sessions_.erase(it);
  for (auto& obs : observers_)
    obs.OnPartitionDestroyed(partition_id);
}
```

- [ ] **Step 4: Run the focused test**

Run from Chromium output after building the test binary:

```bash
autoninja -C "$CHROMIUM_OUT" ephemeral_session_manager_unittest
"$CHROMIUM_OUT/ephemeral_session_manager_unittest" --gtest_filter='*Destroy*'
```

Expected: all matching tests pass.

- [ ] **Step 5: Commit**

```bash
git add chromium_src/overlay/chrome/browser/multi_session/
git commit -m "fix(session): タブ破棄時に ephemeral session を解放"
```

## Task 3: Bind FingerprintSeedReceiver In The Renderer

**Files:**

- Modify: `chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h`
- Modify: `chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.cc`
- Create: `patches/0010-blink-fingerprint-seed-receiver-binding.patch`
- Modify: `patches/series`

- [ ] **Step 1: Add a bind method to the Supplement**

Update `fingerprint_noise_source.h`:

```cpp
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

class MODULES_EXPORT FingerprintNoiseSource final
    : public GarbageCollected<FingerprintNoiseSource>,
      public Supplement<LocalDOMWindow>,
      public mojom::blink::FingerprintSeedReceiver {
 public:
  using SeedReceiver = mojom::blink::FingerprintSeedReceiver;

  static const char kSupplementName[];
  static FingerprintNoiseSource& From(LocalDOMWindow& window);

  explicit FingerprintNoiseSource(LocalDOMWindow& window);

  void BindReceiver(
      mojo::PendingAssociatedReceiver<SeedReceiver> receiver);
```

- [ ] **Step 2: Bind the receiver endpoint**

Add to `fingerprint_noise_source.cc`:

```cpp
void FingerprintNoiseSource::BindReceiver(
    mojo::PendingAssociatedReceiver<SeedReceiver> receiver) {
  receiver_.Bind(std::move(receiver),
                 GetSupplementable()->GetExecutionContext()->GetTaskRunner(
                     TaskType::kInternalDefault));
}
```

If `GetSupplementable()` is not available in M135 for this `Supplement`, store `raw_member<LocalDOMWindow>` or use the constructor `window.GetExecutionContext()` pattern already used by `receiver_`. Keep ownership GC-safe and do not store raw cross-thread pointers.

- [ ] **Step 3: Create renderer associated-interface registration patch**

Create `patches/0010-blink-fingerprint-seed-receiver-binding.patch` against the M135 file that registers frame/window associated interfaces. The resulting inserted code must call `FingerprintNoiseSource::From(*window).BindReceiver(...)` with the receiver endpoint.

The semantic insertion must be equivalent to:

```cpp
#include "chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h"

// CHROMELEON_HOOK: Bind browser-provided fingerprint seed receiver.
associated_interfaces->AddInterface(base::BindRepeating(
    [](LocalDOMWindow* window,
       mojo::PendingAssociatedReceiver<blink::mojom::blink::FingerprintSeedReceiver> receiver) {
      if (!window)
        return;
      FingerprintNoiseSource::From(*window).BindReceiver(std::move(receiver));
    },
    WrapWeakPersistent(window)));
```

Use the actual M135 API names from the registration site. Do not commit code that only allocates `receiver_` without registering it.

- [ ] **Step 4: Register the patch in series**

Update `patches/series`:

```text
0002-blink-navigator-webdriver-hook.patch
0010-blink-fingerprint-seed-receiver-binding.patch
0003-blink-canvas-readback-hook.patch
0004-blink-webgl-readpixels-hook.patch
0005-chrome-render-view-context-menu-hook.patch
0006-chrome-browser-view-grid-toggle-hook.patch
0007-extensions-partition-load-hook.patch
0008-chrome-tab-helpers-hook.patch
0009-chrome-profile-keyed-services-hook.patch
```

- [ ] **Step 5: Verify no unbound receiver-only implementation remains**

Run:

```bash
grep -R "FingerprintSeedReceiver\|BindReceiver\|AddInterface" -n chromium_src/overlay patches
```

Expected: one browser sender in `fingerprint_seed_delivery.cc`, one renderer bind method, and one patch registering the associated interface.

- [ ] **Step 6: Commit**

```bash
git add chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/ patches/0010-blink-fingerprint-seed-receiver-binding.patch patches/series
git commit -m "fix(fingerprint): Renderer 側 seed receiver を bind"
```

## Task 4: Replace Dummy Canvas And WebGL Hooks With Real Readback Hooks

**Files:**

- Modify: `patches/0003-blink-canvas-readback-hook.patch`
- Modify: `patches/0004-blink-webgl-readpixels-hook.patch`
- Modify: `build/ci_patch_dryrun.py`

- [ ] **Step 1: Add dummy-marker rejection to patch validation**

In `build/ci_patch_dryrun.py`, after reading each patch file, reject known dummy markers:

```python
forbidden_markers = [
    "Dummy context",
    "Dummy readback logic",
    "ImageData* image_data = nullptr",
    "pixels_span",
    "@@ -1,0",
]

patch_text = patch_path.read_text()
for marker in forbidden_markers:
    if marker in patch_text:
        print(f"ERROR: {patch_name} contains placeholder marker: {marker}")
        errors += 1
```

- [ ] **Step 2: Run validation to prove current hooks fail**

Run:

```bash
python3 build/ci_patch_dryrun.py
```

Expected before fixing patches: failure mentioning `Dummy context`, `Dummy readback logic`, or `pixels_span`.

- [ ] **Step 3: Patch Canvas real readback return path**

Replace `patches/0003-blink-canvas-readback-hook.patch` with a hunk against the actual M135 Canvas2D readback function. The inserted code must run after `ImageData* image_data` contains the returned pixels and before returning to JS:

```cpp
// CHROMELEON_HOOK: Apply fingerprint noise to canvas readback data.
if (image_data) {
  if (auto* window = DynamicTo<LocalDOMWindow>(GetExecutionContext())) {
    auto& noise = FingerprintNoiseSource::From(*window);
    noise.ApplyCanvasNoise(image_data);
  }
}
```

The patch must not contain `ImageData* image_data = nullptr` or other synthetic code.

- [ ] **Step 4: Patch WebGL real readPixels buffer path**

Replace `patches/0004-blink-webgl-readpixels-hook.patch` with a hunk against the actual M135 `WebGLRenderingContextBase::readPixels` path after the underlying GL read completes and before returning to JS. The inserted code must pass a real mutable byte span:

```cpp
// CHROMELEON_HOOK: Apply fingerprint noise to WebGL readPixels data.
if (!pixels.empty()) {
  if (auto* window = DynamicTo<LocalDOMWindow>(GetExecutionContext())) {
    auto& noise = FingerprintNoiseSource::From(*window);
    noise.ApplyWebGLNoise(base::make_span(pixels), format, type);
  }
}
```

If M135 uses an `ArrayBufferView` instead of a vector, adapt only the span creation to the real buffer API and preserve the `ApplyWebGLNoise(span, format, type)` call.

- [ ] **Step 5: Verify validation now passes**

Run:

```bash
python3 build/ci_patch_dryrun.py
```

Expected: all patches validated and no placeholder-marker errors.

- [ ] **Step 6: Commit**

```bash
git add build/ci_patch_dryrun.py patches/0003-blink-canvas-readback-hook.patch patches/0004-blink-webgl-readpixels-hook.patch
git commit -m "fix(fingerprint): Canvas と WebGL の実 readback に hook"
```

## Task 5: Define The Multi-Session Context Menu String Resource

**Files:**

- Modify: `patches/0005-chrome-render-view-context-menu-hook.patch`
- Optional Create: `patches/0011-chrome-context-menu-string-resource.patch`
- Modify: `patches/series` if `0011` is created

- [ ] **Step 1: Locate the M135 context menu string resource file**

Run in Chromium src:

```bash
grep -R "IDS_CONTENT_CONTEXT_OPENLINKNEWTAB" -n "$CHROMIUM_SRC/chrome" "$CHROMIUM_SRC/components" | head -20
```

Expected: one `.grd` or `.grdp` file defining nearby context-menu string IDs.

- [ ] **Step 2: Add a real string resource**

Patch the located resource file with:

```xml
<message name="IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION" desc="Context menu item for opening the selected link in multiple isolated sessions.">
  Open link in multiple sessions...
</message>
```

Keep this in `0005` if the resource file is closely related to the context menu command patch. Create `0011-chrome-context-menu-string-resource.patch` if the resource file patch is large or independent.

- [ ] **Step 3: Verify the command ID remains local and included**

Check `patches/0005-chrome-render-view-context-menu-hook.patch` still includes:

```cpp
#include "chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/chromeleon_command_ids.h"
#include "chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog.h"
```

And still adds:

```cpp
menu_model_.AddItemWithStringId(
    IDC_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION,
    IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION);
```

- [ ] **Step 4: Verify patch format**

Run:

```bash
python3 build/ci_patch_dryrun.py
```

Expected: all patches validated.

- [ ] **Step 5: Commit**

```bash
git add patches/0005-chrome-render-view-context-menu-hook.patch patches/series
git commit -m "fix(dialog): 複数セッションメニューの文字列リソースを追加"
```

If `0011-chrome-context-menu-string-resource.patch` was created, include it in the `git add` command.

## Task 6: End-To-End Verification Checklist

**Files:**

- Verify: `docs/superpowers/specs/2026-04-21-chromium-multisession-fork-design.md`
- Verify: `docs/superpowers/plans/2026-04-21-chromeleon-implementation.md`
- Verify: all files changed by Tasks 1-5

- [ ] **Step 1: Verify static patch quality**

Run:

```bash
python3 build/ci_patch_dryrun.py
```

Expected: all patches validate and no forbidden placeholder markers are reported.

- [ ] **Step 2: Verify Chromium patch application**

Run:

```bash
python3 build/sync_overlay.py
python3 build/apply_patches.py
```

Expected: every patch in `patches/series` applies with exit code 0.

- [ ] **Step 3: Verify GN graph sees overlay code**

Run:

```bash
cd "$CHROMIUM_SRC"
gn gen "$CHROMIUM_OUT" --args="$(cat /workspaces/custom-chromium/config/gn_args.gn)"
gn desc "$CHROMIUM_OUT" //chrome/browser:browser deps | grep 'chromium_src/overlay/chrome/browser/multi_session'
```

Expected: the grep prints the multi-session overlay target.

- [ ] **Step 4: Verify focused tests**

Run:

```bash
autoninja -C "$CHROMIUM_OUT" ephemeral_session_manager_unittest fingerprint_noise_source_unittest partition_extension_autoloader_unittest
"$CHROMIUM_OUT/ephemeral_session_manager_unittest"
"$CHROMIUM_OUT/fingerprint_noise_source_unittest"
"$CHROMIUM_OUT/partition_extension_autoloader_unittest"
```

Expected: each test binary exits 0.

- [ ] **Step 5: Verify design data-flow manually in code**

Confirm these exact edges exist:

```text
RenderViewContextMenu ExecuteCommand
  -> MultiSessionOpenDialog::Show
  -> MultiSessionOpenDialog::Accept
  -> EphemeralSessionManager::ExpandLinkInSessions
  -> EphemeralSessionManager::CreateSessionForNewTab
  -> Observer::OnPartitionCreated
  -> PartitionExtensionAutoloader::OnPartitionCreated

TabHelpers::AttachTabHelpers
  -> FingerprintSeedDelivery::CreateForWebContents
  -> FingerprintSeedDelivery::RenderFrameCreated
  -> EphemeralSessionManager::GetSeedForPartitionConfig
  -> blink.mojom.FingerprintSeedReceiver::SetSeed
  -> FingerprintNoiseSource::SetSeed
  -> ApplyCanvasNoise / ApplyWebGLNoise at real readback hooks

WebContentsDestroyed
  -> FingerprintSeedDelivery::WebContentsDestroyed
  -> EphemeralSessionManager::DestroySessionForTab
  -> Observer::OnPartitionDestroyed
```

- [ ] **Step 6: Commit verification-only documentation updates if needed**

Only update docs if implementation choices differ from the existing design. If docs change, commit separately:

```bash
git add docs/superpowers/specs/2026-04-21-chromium-multisession-fork-design.md docs/superpowers/plans/2026-04-21-chromeleon-implementation.md
git commit -m "docs: Chromeleon データフロー修正内容を反映"
```

## Self-Review

- Spec coverage: The plan covers the verified gaps: browser overlay build linkage, session destruction, seed delivery binding, Canvas/WebGL readback hook placement, context-menu string resource, and verification.
- Placeholder scan: This plan avoids open-ended placeholders and unspecified testing instructions. The only adaptive points are explicitly bounded to M135 API names that must be inspected in the Chromium checkout before patching.
- Type consistency: Existing names are preserved: `EphemeralSessionManager`, `FingerprintSeedDelivery`, `FingerprintNoiseSource`, `PartitionExtensionAutoloader`, `IDC_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION`, and `IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION`.
