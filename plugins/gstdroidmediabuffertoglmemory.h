/* GStreamer
 * Copyright (C) 2019 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_DROIDMEDIABUFFERTOGLMEMORY_H_
#define _GST_DROIDMEDIABUFFERTOGLMEMORY_H_

#include <gst/base/gstbasetransform.h>

#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglcontext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

G_BEGIN_DECLS
#define GST_TYPE_DROIDMEDIABUFFERTOGLMEMORY   (gst_droidmediabuffertoglmemory_get_type())
#define GST_DROIDMEDIABUFFERTOGLMEMORY(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DROIDMEDIABUFFERTOGLMEMORY,GstDroidmediabuffertoglmemory))
#define GST_DROIDMEDIABUFFERTOGLMEMORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DROIDMEDIABUFFERTOGLMEMORY,GstDroidmediabuffertoglmemoryClass))
#define GST_IS_DROIDMEDIABUFFERTOGLMEMORY(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DROIDMEDIABUFFERTOGLMEMORY))
#define GST_IS_DROIDMEDIABUFFERTOGLMEMORY_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DROIDMEDIABUFFERTOGLMEMORY))
typedef struct _GstDroidmediabuffertoglmemory GstDroidmediabuffertoglmemory;
typedef struct _GstDroidmediabuffertoglmemoryClass
    GstDroidmediabuffertoglmemoryClass;

struct _GstDroidmediabuffertoglmemory
{
  GstBaseTransform base_droidmediabuffertoglmemory;

  GstGLDisplay *display;
  GstGLContext *context;
  GstGLContext *other_context;

    EGLImageKHR (*eglCreateImageKHR) (EGLDisplay dpy, EGLContext ctx,
      EGLenum target, EGLClientBuffer buffer, const EGLint * attrib_list);
    EGLBoolean (*eglDestroyImageKHR) (EGLDisplay dpy, EGLImageKHR image);
};

struct _GstDroidmediabuffertoglmemoryClass
{
  GstBaseTransformClass base_droidmediabuffertoglmemory_class;
};

GType gst_droidmediabuffertoglmemory_get_type (void);

G_END_DECLS
#endif
