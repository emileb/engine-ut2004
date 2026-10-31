/*=============================================================================
	game_interface.cpp: the OpenTouch touch controls to UT2004 (Portable* API).
	Buttons are the keys Engine/Config/android/DefUser.ini binds; movement and
	look go into SDL2Viewport's touch state, read on the engine thread.
=============================================================================*/

// System headers before Engine.h, whose clock() macro breaks <time.h>.
#include "SDL.h"
#include <math.h>
#include <android/log.h>

#include "Engine.h"
#include "game_interface.h"

extern "C" int SDL_main( int argc, char** argv );
extern "C" int SDL_SendKeyboardKey( Uint8 state, SDL_Scancode scancode );

// SDL2Drv/Src/SDL2Viewport.cpp.
extern "C" int  UT2004_TouchMenuVisible();
extern "C" void UT2004_TouchMove( float Strafe, float Back );
extern "C" void UT2004_TouchLook( float DX, float DY );
extern "C" void UT2004_TouchKey( int Key, int Down );

// The touch stick's range at the default sensitivity.
static const float FWD_STICK_RANGE    = 5.0f;
static const float STRAFE_STICK_RANGE = 3.0f;

// Look: mouse-mode deltas are fractions of the screen, joystick mode a held rate per 60 Hz frame.
static const float LOOK_MOUSE_SCALE = 1500.0f;
static const float LOOK_JOY_SCALE   = 20.0f;

static float s_stickFwd = 0.f, s_stickStrafe = 0.f;
static int   s_keyFwd = 0, s_keyBack = 0, s_keyLeft = 0, s_keyRight = 0;
static float s_lookJoyYaw = 0.f, s_lookJoyPitch = 0.f;

static float clampUnit( float V )
{
	return V > 1.f ? 1.f : ( V < -1.f ? -1.f : V );
}

static void publishMove()
{
	const float Fwd    = clampUnit( s_stickFwd + s_keyFwd - s_keyBack );
	const float Strafe = clampUnit( s_stickStrafe + s_keyRight - s_keyLeft );
	UT2004_TouchMove( Strafe, -Fwd );
}

static void key( int State, EInputKey Key )
{
	UT2004_TouchKey( Key, State );
}

static void tap( EInputKey Key )
{
	UT2004_TouchKey( Key, 1 );
	UT2004_TouchKey( Key, 0 );
}

void PortableInit( int argc, const char** argv )
{
	LOGI( "PortableInit" );
	// Runs the engine on this thread until the game exits.
	SDL_main( argc, (char**)argv );
}

void PortableBackButton( void )
{
	tap( IK_Escape );
}

int PortableKeyEvent( int state, int code, int unitcode )
{
	SDL_SendKeyboardKey( state ? SDL_PRESSED : SDL_RELEASED, (SDL_Scancode)code );
	return 0;
}

void PortableAction( int state, int action )
{
	// Rebindable custom buttons: keypad 1-0 for 0-9, A-P for 10-25.
	if( action >= PORT_ACT_CUSTOM_0 && action <= PORT_ACT_CUSTOM_25 )
	{
		if( action <= PORT_ACT_CUSTOM_9 )
			key( state, (EInputKey)( IK_NumPad1 + action - PORT_ACT_CUSTOM_0 ) );
		else
			key( state, (EInputKey)( IK_A + action - PORT_ACT_CUSTOM_10 ) );
		return;
	}
	if( action >= PORT_ACT_WEAP0 && action <= PORT_ACT_WEAP9 )
	{
		if( state )
			tap( (EInputKey)( IK_0 + action - PORT_ACT_WEAP0 ) );
		return;
	}

	if( PortableGetScreenMode() == TS_MENU )
	{
		switch( action )
		{
			case PORT_ACT_MENU_UP:      key( state, IK_Up );     return;
			case PORT_ACT_MENU_DOWN:    key( state, IK_Down );   return;
			case PORT_ACT_MENU_LEFT:    key( state, IK_Left );   return;
			case PORT_ACT_MENU_RIGHT:   key( state, IK_Right );  return;
			case PORT_ACT_MENU_SELECT:  key( state, IK_Enter );  return;
			case PORT_ACT_MENU_BACK:    key( state, IK_Escape ); return;
			case PORT_ACT_MENU_CONFIRM: key( state, IK_Y );      return;
			case PORT_ACT_MENU_ABORT:   key( state, IK_N );      return;
		}
	}

	switch( action )
	{
		case PORT_ACT_FWD:        s_keyFwd = state;   publishMove(); return;
		case PORT_ACT_BACK:       s_keyBack = state;  publishMove(); return;
		case PORT_ACT_MOVE_LEFT:  s_keyLeft = state;  publishMove(); return;
		case PORT_ACT_MOVE_RIGHT: s_keyRight = state; publishMove(); return;

		case PORT_ACT_ATTACK:     key( state, IK_LeftMouse );  return;
		case PORT_ACT_ALT_ATTACK: key( state, IK_RightMouse ); return;
		case PORT_ACT_JUMP:       key( state, IK_Space );      return;
		case PORT_ACT_DOWN:
		case PORT_ACT_CROUCH:     key( state, IK_C );          return;
		case PORT_ACT_USE:        key( state, IK_E );          return;
		case PORT_ACT_MP_SCORES:  key( state, IK_F1 );         return;
		case PORT_ACT_VOICE_MENU: key( state, IK_V );          return;
		case PORT_ACT_CONSOLE:    key( state, IK_Tilde );      return;
		case PORT_ACT_MP_SAY:     key( state, IK_T );          return;

		case PORT_ACT_NEXT_WEP:   if( state ) tap( IK_MouseWheelDown ); return;
		case PORT_ACT_PREV_WEP:   if( state ) tap( IK_MouseWheelUp );   return;
	}
}

void PortableMoveFwd( float fwd )
{
	s_stickFwd = clampUnit( fwd / FWD_STICK_RANGE );
	publishMove();
}

void PortableMoveSide( float strafe )
{
	s_stickStrafe = clampUnit( strafe / STRAFE_STICK_RANGE );
	publishMove();
}

void PortableMove( float fwd, float strafe )
{
	s_stickFwd = clampUnit( fwd / FWD_STICK_RANGE );
	s_stickStrafe = clampUnit( strafe / STRAFE_STICK_RANGE );
	publishMove();
}

// OpenTouch's yaw is positive to the left and its pitch positive up.
void PortableLookPitch( int mode, float pitch )
{
	if( mode == LOOK_MODE_JOYSTICK )
		s_lookJoyPitch = pitch;
	else
		UT2004_TouchLook( 0.f, -pitch * LOOK_MOUSE_SCALE );
}

void PortableLookYaw( int mode, float yaw )
{
	if( mode == LOOK_MODE_JOYSTICK )
		s_lookJoyYaw = yaw;
	else
		UT2004_TouchLook( -yaw * LOOK_MOUSE_SCALE, 0.f );
}

void PortableMouse( float dx, float dy )
{
	UT2004_TouchLook( -dx * LOOK_MOUSE_SCALE, -dy * LOOK_MOUSE_SCALE );
}

void PortableMouseAbs( float x, float y )
{
}

void PortableMouseButton( int state, int button, float dx, float dy )
{
	MouseButton( state, button );
}

void PortableCommand( const char* cmd )
{
}

void PortableAutomapControl( float zoom, float x, float y )
{
}

int PortableShowKeyboard( void )
{
	return 0;
}

bool PortableSetAlwaysRun( bool run )
{
	return run;
}

touchscreemode_t PortableGetScreenMode()
{
	return UT2004_TouchMenuVisible() ? TS_MENU : TS_GAME;
}

void PortableSetMouseTapMode( int enable )
{
}

int PortableGetMouseTapMode( void )
{
	return 0;
}

// Once a frame from the touch controls: the joystick look rate as look deltas for the time since the last frame.
extern "C" void UT2004_TouchFrame()
{
	static Uint32 LastTicks = 0;
	const Uint32 Now = SDL_GetTicks();
	const float Frames = LastTicks ? ( Now - LastTicks ) * 0.06f : 0.f;
	LastTicks = Now;
	if( s_lookJoyYaw != 0.f || s_lookJoyPitch != 0.f )
		UT2004_TouchLook( -s_lookJoyYaw * LOOK_JOY_SCALE * Frames, -s_lookJoyPitch * LOOK_JOY_SCALE * Frames );
}
