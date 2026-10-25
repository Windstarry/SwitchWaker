/*
 * pc_controls.h - control options (native/src/pc/features/pc_controls.cpp).
 *
 * COS_CAMERA_INVERT_X=1   the C stick's horizontal axis turns the camera the other way
 * COS_CAMERA_INVERT_Y=1   the C stick's vertical axis tilts the camera the other way
 * COS_AIM_RIGHT_STICK=1   the C stick aims too (bow, hookshot, boomerang, first person...), issue #23
 * COS_AIM_INVERT_Y=1      aiming's vertical axis the other way: stick up aims up (GameCube: down)
 * COS_CAMERA_SHAKE=1      the camera shakes (quakes, hits, explosions, cutscenes); default off
 * COS_HEAT_HAZE=1         the heat haze (the screen wobbling over lava and fire); default off
 *
 * Applied only where the camera reads the C stick (d_camera.cpp, dCamera_cStickX/Y): the wind
 * baton's left hand, the item menu's songs, the grappling hook and the figure viewer keep the C
 * stick's real directions; so do the camera's own C stick commands (dCamera_cStickYCmd: up enters
 * the look-around view, down ends it, up/down zoom the telescope and the Picto Box), only the
 * camera's turns and tilts are inverted. Options menu: Controles > "Invertir cámara horizontal" and "Invertir
 * cámara vertical", live. Aiming: every first-person view (the bow, the hookshot, the boomerang,
 * the grappling hook's throw, the telescope, the Picto Box, looking around) turns through
 * dCamera_c::CalcSubjectAngle, which reads the main stick; with COS_AIM_RIGHT_STICK the C stick's
 * position is added to it (clamped), once the C stick has been centred since the view opened (the
 * C stick up that enters the look-around view does not aim at once). The telescope's and the Picto
 * Box's zoom keep the C stick's vertical axis (only its horizontal one aims), and the look-around
 * view no longer ends on the C stick down (A, B and L still end it). COS_AIM_INVERT_Y flips the
 * vertical axis of both sticks while aiming; the camera's own inversion never reaches aiming.
 * Controles > "Apuntar con el stick derecho" and "Invertir apuntado vertical", live. The camera shake: every shake reaches the camera through dVibration_c::Run
 * (d_vibration.cpp), which starts none while it is off (the controller rumble is kept); Gráficos >
 * "Temblor de cámara", live. The heat haze: the "kagerou" particles that bend the frame buffer
 * (Dragon Roost and its cavern, fire holes, Magtails...); off, JPAEmitterManager::draw skips their
 * emitters; Gráficos > "Distorsión por calor", live. The game thread calls all of them.
 */
#ifndef PC_CONTROLS_H
#define PC_CONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Nonzero when that C stick axis is inverted (read from the environment the first time). */
int pc_camera_invert_x(void);
int pc_camera_invert_y(void);
/* The options menu: set an axis (0 or 1). */
void pc_camera_invert_x_set(int on);
void pc_camera_invert_y_set(int on);
/* Nonzero when the C stick aims too (COS_AIM_RIGHT_STICK=1; off by default), and its setter. */
int pc_aim_right_stick(void);
void pc_aim_right_stick_set(int on);
/* Nonzero when aiming's vertical axis is inverted (COS_AIM_INVERT_Y=1; off by default), and its setter. */
int pc_aim_invert_y(void);
void pc_aim_invert_y_set(int on);
/* Nonzero when the camera may shake (COS_CAMERA_SHAKE=1; off by default), and its setter. */
int pc_camera_shake(void);
void pc_camera_shake_set(int on);
/* Nonzero when the heat haze is drawn (COS_HEAT_HAZE=1; off by default), and its setter. */
int pc_heat_haze(void);
void pc_heat_haze_set(int on);
/* Nonzero when an emitter of that particle user index must not be drawn (heat haze off). */
int pc_heat_haze_hidden(unsigned short user_id);

#ifdef __cplusplus
}
#endif

#endif
