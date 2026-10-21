// COS_SMOKE=vibration: dVibration_c with 0-bit patterns (d_vibration.cpp's rollshift and makebits). In
// play (e.g. COS_BOOT_STAGE=Asoko:0:0:2), with the camera shake on (pc_controls.h), after kSettleFrames:
//   1. a camera shock (CS_patt[4], 7 frames) and, on the frame its mCurrentFrame reaches its length, a
//      camera quake (CQ_patt[1]): Run's shock + quake case then rolls a pattern of length 0;
//   2. a camera quake from CQ_patt[0], whose length is 0: Run's quake case rolls it;
//   3. a camera shock with CQ_patt[0]'s quake: makebits extends a 0-bit pattern.
// rollshift divided by the 0 length (cases 1 and 2: SIGFPE on x86-64; the GameCube's divw and AArch64's
// sdiv do not trap) and makebits never left its loop (case 3, every host). Each case must leave the game
// running kRunFrames (the quake stopped after it). Exit 0 after the three, else the crash (13) or the
// stall watchdog ends the run.
#include "pc_internal.h"

#include "pc/pc_controls.h"

#include "d/d_com_inf_game.h"
#include "d/d_vibration.h"
#include "f_pc/f_pc_manager.h"
#include "f_pc/f_pc_name.h"

#include <cstring>
#include <unistd.h>

namespace pc {

namespace {

constexpr unsigned int kSettleFrames = 60;
constexpr unsigned int kRunFrames = 30;
constexpr int kCameraFlags = 0x1E; // camera shake bits only (MASK_CAMERA_SHAKE without the motor's 0x1)
constexpr int kShock = 4;          // CS_patt[4]: 7 frames
constexpr int kQuake = 1;          // CQ_patt[1]: 32 frames
constexpr int kZeroQuake = 0;      // CQ_patt[0]: 0 frames

bool sChecked = false;
bool sOn = false;
int sCase = 0; // 0 settle, 1..3 the cases above, 4 done
bool sStarted = false;
bool sQuakeStarted = false;
unsigned int sSince = 0;
int sShakeBefore = 0;

void* isPlayScene(void* proc, void*) {
    return fpcM_GetName(proc) == fpcNm_PLAY_SCENE_e ? proc : nullptr;
}

void nextCase(unsigned int frames) {
    dVibration_c& vib = dComIfGp_getVibration();
    vib.StopQuake(kCameraFlags);
    writef(STDERR_FILENO, "[cos] vibration: frame %u: case %d ran %u frames\n", frames, sCase, kRunFrames);
    sCase++;
    sStarted = false;
    sQuakeStarted = false;
    sSince = 0;
}

} // namespace

void vibrationFrame(unsigned int frames) {
    if (!sChecked) {
        sChecked = true;
        sOn = gConfig.smoke != nullptr && strcmp(gConfig.smoke, "vibration") == 0;
    }
    if (!sOn || dComIfGp_getPlayer(0) == nullptr || fpcM_Search(isPlayScene, nullptr) == nullptr) {
        return;
    }
    dVibration_c& vib = dComIfGp_getVibration();
    const cXyz coord(0.0f, 1.0f, 0.0f);
    ++sSince;
    switch (sCase) {
    case 0:
        if (sSince >= kSettleFrames) {
            sShakeBefore = pc_camera_shake();
            pc_camera_shake_set(1);
            sCase = 1;
            sSince = 0;
        }
        return;
    case 1:
        if (!sStarted) {
            vib.StartShock(kShock, kCameraFlags, coord);
            sStarted = true;
            sSince = 0;
            return;
        }
        // Run has counted the shock's frames up to its length: the quake starts on its last frame.
        if (!sQuakeStarted && vib.mCamera.mShock.mPatternIdx == kShock &&
            vib.mCamera.mShock.mCurrentFrame == vib.mCamera.mShock.mLength) {
            vib.StartQuake(kQuake, kCameraFlags, coord);
            sQuakeStarted = true;
            sSince = 0;
            writef(STDERR_FILENO, "[cos] vibration: frame %u: quake started on the shock's last frame (%d)\n",
                   frames, vib.mCamera.mShock.mLength);
            return;
        }
        if (!sQuakeStarted && sSince > 60) {
            writef(STDERR_FILENO, "[cos] vibration: FAIL the shock never reached its last frame\n");
            pc_camera_shake_set(sShakeBefore);
            sOn = false;
            pc_exit(PC_EXIT_CHECK_FAILED);
            return;
        }
        if (sQuakeStarted && sSince >= kRunFrames) {
            nextCase(frames);
        }
        return;
    case 2:
        if (!sStarted) {
            vib.StartQuake(kZeroQuake, kCameraFlags, coord);
            sStarted = true;
            sSince = 0;
            return;
        }
        if (sSince >= kRunFrames) {
            nextCase(frames);
        }
        return;
    case 3:
        if (!sStarted) {
            vib.StartShock(kShock, kCameraFlags, coord);
            vib.StartQuake(kZeroQuake, kCameraFlags, coord);
            sStarted = true;
            sSince = 0;
            return;
        }
        if (sSince >= kRunFrames) {
            nextCase(frames);
        }
        return;
    default:
        pc_camera_shake_set(sShakeBefore);
        writef(STDERR_FILENO, "[cos] vibration: PASS\n");
        sOn = false;
        pc_exit(PC_EXIT_REACHED);
        return;
    }
}

} // namespace pc
