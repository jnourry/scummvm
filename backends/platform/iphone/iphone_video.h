/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef BACKENDS_PLATFORM_IPHONE_IPHONE_VIDEO_H
#define BACKENDS_PLATFORM_IPHONE_IPHONE_VIDEO_H

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#include "iphone_keyboard.h"

@interface iPhoneView : UIView {
	void *_screenSurface;
	NSMutableArray *_events;
	SoftKeyboard *_keyboardView;

	int _widthOffset;
	int _heightOffset;

	EAGLContext *_context;
	GLuint _viewRenderbuffer;
	GLuint _viewFramebuffer;
	GLint _renderBufferWidth;
	GLint _renderBufferHeight;
	GLint _visibleWidth;
	GLint _visibleHeight;
	GLuint _screenTexture;
	GLuint _overlayTexture;
	GLuint _mouseCursorTexture;

	UIDeviceOrientation _orientation;

	GLfloat _gameScreenVertCoords[4 * 2];
	GLfloat _gameScreenTexCoords[4 * 2];

	GLfloat _overlayVertCoords[4 * 2];
	GLfloat _overlayTexCoords[4 * 2];
}

- (id)initWithFrame:(struct CGRect)frame;

- (void)drawRect:(CGRect)frame;

- (void *)getSurface;

- (void)initSurface;

- (void)setGraphicsMode;

- (void)updateSurface;
- (void)updateMainSurface;
- (void)updateOverlaySurface;
- (void)updateMouseSurface;
- (void)clearColorBuffer;

- (void)updateMouseCursor;

- (id)getEvent;

- (void)deviceOrientationChanged:(UIDeviceOrientation)orientation;

- (void)applicationSuspend;

- (void)applicationResume;

@end

#endif
