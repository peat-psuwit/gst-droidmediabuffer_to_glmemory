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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstdroidmediabuffertoglmemory
 *
 * The droidmediabuffertoglmemory element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! droidmediabuffertoglmemory ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gl/gstglfuncs.h>
#include <gst/gl/gstglutils.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/egl/gstegl.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#include <gst/allocators/gstdroidmediabuffer.h>

#include <GLES2/gl2ext.h>

#include "gstdroidmediabuffertoglmemory.h"

GST_DEBUG_CATEGORY_STATIC (gst_droidmediabuffertoglmemory_debug_category);
#define GST_CAT_DEFAULT gst_droidmediabuffertoglmemory_debug_category

/* prototypes */

static void gst_droidmediabuffertoglmemory_set_context (GstElement * element,
    GstContext * context);

static GstCaps *gst_droidmediabuffertoglmemory_transform_caps (GstBaseTransform
    * trans, GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static gboolean
gst_droidmediabuffertoglmemory_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_droidmediabuffertoglmemory_start (GstBaseTransform * trans);
static gboolean gst_droidmediabuffertoglmemory_stop (GstBaseTransform * trans);
static GstFlowReturn
gst_droidmediabuffertoglmemory_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf);
static GstFlowReturn gst_droidmediabuffertoglmemory_transform (GstBaseTransform
    * trans, GstBuffer * inbuf, GstBuffer * outbuf);

/* pad templates */

static GstStaticPadTemplate gst_droidmediabuffertoglmemory_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES (GST_CAPS_FEATURE_MEMORY_GL_MEMORY, "{ RGBA }")  // XXX: is "RGBA" correct?
        ", texture-target = (string) { external-oes }")
    );

static GstStaticPadTemplate gst_droidmediabuffertoglmemory_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_DROID_MEDIA_BUFFER,
            GST_DROID_MEDIA_BUFFER_MEMORY_VIDEO_FORMATS)));


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstDroidmediabuffertoglmemory,
    gst_droidmediabuffertoglmemory, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_droidmediabuffertoglmemory_debug_category,
        "droidmediabuffertoglmemory", 0,
        "debug category for droidmediabuffertoglmemory element"));

static void
gst_droidmediabuffertoglmemory_class_init (GstDroidmediabuffertoglmemoryClass *
    klass)
{
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_droidmediabuffertoglmemory_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_droidmediabuffertoglmemory_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gstelement_class->set_context =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_set_context);
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform_caps);
  base_transform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_set_caps);
  base_transform_class->start =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_start);
  base_transform_class->stop =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_stop);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_prepare_output_buffer);
  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform);

}

static void
gst_droidmediabuffertoglmemory_init (GstDroidmediabuffertoglmemory *
    droidmediabuffertoglmemory)
{
  droidmediabuffertoglmemory->display = NULL;
  droidmediabuffertoglmemory->context = NULL;
  droidmediabuffertoglmemory->other_context = NULL;
}

static void
gst_droidmediabuffertoglmemory_set_context (GstElement * element,
    GstContext * context)
{
  // Pretty much copied from gstglimagesink.c

  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (element);

  gst_gl_handle_set_context (element, context,
      &droidmediabuffertoglmemory->display,
      &droidmediabuffertoglmemory->other_context);

  // TODO: Is this needed?
#if 0
  if (droidmediabuffertoglmemory->display)
    gst_gl_display_filter_gl_api (droidmediabuffertoglmemory->display,
        SUPPORTED_GL_APIS);
#endif

  GST_ELEMENT_CLASS (gst_droidmediabuffertoglmemory_parent_class)->set_context
      (element, context);
}

static GstCaps *
gst_droidmediabuffertoglmemory_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);
  GstCaps *othercaps;

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "transform_caps");

  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */

    // I don't want to deal with it. Pad template should be enough.
    othercaps = gst_caps_new_any ();
  } else {
    /* transform caps going downstream */

    othercaps = gst_caps_copy (caps);

    // Replace the feature with GLMemory & set color-format to RGBA
    // Also set texture-target
    GstCapsFeatures *glmemoryfeat =
        gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_GL_MEMORY,
        NULL);
    gst_caps_set_features (othercaps, /* index (XXX: other index?) */ 0,
        glmemoryfeat);

    gst_caps_set_simple (othercaps,
        "format", G_TYPE_STRING, "RGBA",
        "texture-target", G_TYPE_STRING, "external-oes", NULL);
  }

  if (filter) {
    GstCaps *intersect;

    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);

    return intersect;
  } else {
    return othercaps;
  }
}

static gboolean
gst_droidmediabuffertoglmemory_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  if (!gst_video_info_from_caps (&droidmediabuffertoglmemory->out_vinfo,
          outcaps)) {
    return FALSE;
  }

  return TRUE;
}

static gboolean
_ensure_gl_context (GstDroidmediabuffertoglmemory * droidmediabuffertoglmemory)
{
  // Pretty much copied from gstglimagesink.c, but only for context part.
  GError *error = NULL;

  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "Ensuring GL context");

  if (!droidmediabuffertoglmemory->context) {
    GST_OBJECT_LOCK (droidmediabuffertoglmemory->display);
    do {
      GstGLContext *other_context = NULL;

      if (droidmediabuffertoglmemory->context) {
        gst_object_unref (droidmediabuffertoglmemory->context);
        droidmediabuffertoglmemory->context = NULL;
      }

      GST_DEBUG_OBJECT (droidmediabuffertoglmemory,
          "No current context, creating one for %" GST_PTR_FORMAT,
          droidmediabuffertoglmemory->display);

      if (droidmediabuffertoglmemory->other_context) {
        other_context =
            gst_object_ref (droidmediabuffertoglmemory->other_context);
      } else {
        other_context =
            gst_gl_display_get_gl_context_for_thread
            (droidmediabuffertoglmemory->display, NULL);
      }

      if (!gst_gl_display_create_context (droidmediabuffertoglmemory->display,
              other_context, &droidmediabuffertoglmemory->context, &error)) {
        if (other_context)
          gst_object_unref (other_context);
        GST_OBJECT_UNLOCK (droidmediabuffertoglmemory->display);
        goto context_error;
      }

      GST_DEBUG_OBJECT (droidmediabuffertoglmemory,
          "created context %" GST_PTR_FORMAT " from other context %"
          GST_PTR_FORMAT, droidmediabuffertoglmemory->context,
          droidmediabuffertoglmemory->other_context);
    } while (!gst_gl_display_add_context (droidmediabuffertoglmemory->display,
            droidmediabuffertoglmemory->context));
    GST_OBJECT_UNLOCK (droidmediabuffertoglmemory->display);
  }
  return TRUE;

context_error:
  {
    GST_ELEMENT_ERROR (droidmediabuffertoglmemory, RESOURCE, NOT_FOUND, ("%s",
            error->message), (NULL));

    if (droidmediabuffertoglmemory->context) {
      gst_object_unref (droidmediabuffertoglmemory->context);
      droidmediabuffertoglmemory->context = NULL;
    }

    g_clear_error (&error);

    return FALSE;
  }
}

static gboolean
_populate_egl_proc (GstDroidmediabuffertoglmemory * droidmediabuffertoglmemory)
{
  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "populate egl proc");
  if (G_UNLIKELY (!droidmediabuffertoglmemory->eglCreateImageKHR)) {
    droidmediabuffertoglmemory->eglCreateImageKHR =
        gst_gl_context_get_proc_address (droidmediabuffertoglmemory->context,
        "eglCreateImageKHR");
  }
  if (G_UNLIKELY (!droidmediabuffertoglmemory->eglCreateImageKHR)) {
    return FALSE;
  }

  if (G_UNLIKELY (!droidmediabuffertoglmemory->eglDestroyImageKHR)) {
    droidmediabuffertoglmemory->eglDestroyImageKHR =
        gst_gl_context_get_proc_address (droidmediabuffertoglmemory->context,
        "eglDestroyImageKHR");

  }
  if (G_UNLIKELY (!droidmediabuffertoglmemory->eglDestroyImageKHR)) {
    return FALSE;
  }

  if (G_UNLIKELY (!droidmediabuffertoglmemory->glEGLImageTargetTexture2DOES)) {
    droidmediabuffertoglmemory->glEGLImageTargetTexture2DOES =
        gst_gl_context_get_proc_address (droidmediabuffertoglmemory->context,
        "glEGLImageTargetTexture2DOES");

  }
  if (G_UNLIKELY (!droidmediabuffertoglmemory->glEGLImageTargetTexture2DOES)) {
    return FALSE;
  }

  return TRUE;
}

/* states */
static gboolean
gst_droidmediabuffertoglmemory_start (GstBaseTransform * trans)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "start");

  if (!gst_gl_ensure_element_data (droidmediabuffertoglmemory,
          &droidmediabuffertoglmemory->display,
          &droidmediabuffertoglmemory->other_context)) {
    return FALSE;
  }

  if (!_ensure_gl_context (droidmediabuffertoglmemory)) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "unable to ensure EGL context");
    return FALSE;
  }

  if (!_populate_egl_proc (droidmediabuffertoglmemory)) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "unable to populate required EGL functions");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_droidmediabuffertoglmemory_stop (GstBaseTransform * trans)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "stop");

  // XXX: will this race with buffer destruction?
  if (droidmediabuffertoglmemory->context) {
    gst_object_unref (droidmediabuffertoglmemory->context);
    droidmediabuffertoglmemory->context = NULL;
  }

  if (droidmediabuffertoglmemory->other_context) {
    gst_object_unref (droidmediabuffertoglmemory->other_context);
    droidmediabuffertoglmemory->other_context = NULL;
  }

  if (droidmediabuffertoglmemory->display) {
    gst_object_unref (droidmediabuffertoglmemory->display);
    droidmediabuffertoglmemory->display = NULL;
  }
  // Just in case they changes with context.
  droidmediabuffertoglmemory->eglCreateImageKHR = NULL;
  droidmediabuffertoglmemory->eglDestroyImageKHR = NULL;
  droidmediabuffertoglmemory->glEGLImageTargetTexture2DOES = NULL;

  return TRUE;
}

static GstMemory *
_get_droid_media_buffer_memory (GstDroidmediabuffertoglmemory *
    droidmediabuffertoglmemory, GstBuffer * buffer)
{
  // Copied from droideglsink.c
  int x, num;
  GST_TRACE_OBJECT (droidmediabuffertoglmemory,
      "get droid media buffer memory");
  num = gst_buffer_n_memory (buffer);
  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "examining %d memory items",
      num);
  for (x = 0; x < num; x++) {
    GstMemory *mem = gst_buffer_peek_memory (buffer, x);
    if (mem && gst_memory_is_type (mem, GST_ALLOCATOR_DROID_MEDIA_BUFFER)) {
      return mem;
    }
  }
  return NULL;
}

static void
_destroy_egl_image (GstDroidmediabuffertoglmemory * droidmediabuffertoglmemory,
    EGLImageKHR * image)
{
  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "destroy EGLImage %p", image);

  EGLDisplay egl_display = EGL_DEFAULT_DISPLAY;
  GstGLDisplayEGL *display_egl;

  display_egl =
      gst_gl_display_egl_from_gl_display (droidmediabuffertoglmemory->display);
  if (!display_egl) {
    GST_WARNING_OBJECT (droidmediabuffertoglmemory,
        "Failed to retrieve GstGLDisplayEGL from %" GST_PTR_FORMAT,
        droidmediabuffertoglmemory->display);
    return;
  }
  egl_display =
      (EGLDisplay) gst_gl_display_get_handle (GST_GL_DISPLAY (display_egl));
  gst_object_unref (display_egl);

  if (!droidmediabuffertoglmemory->eglDestroyImageKHR (egl_display, image)) {
    GST_WARNING_OBJECT (droidmediabuffertoglmemory,
        "eglDestroyImageKHR failed: %s. EGLImage may leak.",
        gst_egl_get_error_string (eglGetError ()));
  }
}

static EGLImageKHR
_create_egl_image (GstDroidmediabuffertoglmemory * droidmediabuffertoglmemory,
    GstMemory * droidmem)
{
  EGLDisplay egl_display = EGL_DEFAULT_DISPLAY;
  EGLImageKHR img = EGL_NO_IMAGE_KHR;
  GstGLDisplayEGL *display_egl;
  // Copied from droideglsink.c. Not sure why.
  EGLint eglImgAttrs[] =
      { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };

  GST_TRACE_OBJECT (droidmediabuffertoglmemory,
      "creating EGLImage from DroidMediaBufferMemory");

  // This part comes from gsteglimage.c
  display_egl =
      gst_gl_display_egl_from_gl_display (droidmediabuffertoglmemory->display);
  if (!display_egl) {
    GST_WARNING_OBJECT (droidmediabuffertoglmemory,
        "Failed to retrieve GstGLDisplayEGL from %" GST_PTR_FORMAT,
        droidmediabuffertoglmemory->display);
    return NULL;
  }
  egl_display =
      (EGLDisplay) gst_gl_display_get_handle (GST_GL_DISPLAY (display_egl));
  gst_object_unref (display_egl);

  img = droidmediabuffertoglmemory->eglCreateImageKHR (egl_display,
      EGL_NO_CONTEXT,
      EGL_NATIVE_BUFFER_ANDROID,
      (EGLClientBuffer) gst_droid_media_buffer_memory_get_buffer (droidmem),
      eglImgAttrs);

  if (img == EGL_NO_IMAGE_KHR) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "eglCreateImageKHR failed: %s",
        gst_egl_get_error_string (eglGetError ()));
  }

  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "created (EGLImage) %p", img);

  return img;
}

struct _WrappedTextureMetadata
{
  GstDroidmediabuffertoglmemory *self;
  EGLImage image;
  guint tex_id;
};

static void
_create_texture_from_image_thread (GstGLContext * context, gpointer data)
{
  struct _WrappedTextureMetadata *metadata = data;
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory = metadata->self;

  GstGLFuncs *gl = droidmediabuffertoglmemory->context->gl_vtable;

  if (!gst_gl_context_activate (context, TRUE)) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "cannot activate this element's context.");
    return;
  }

  gl->GenTextures ( /* n */ 1, &metadata->tex_id);
  gl->BindTexture (GL_TEXTURE_EXTERNAL_OES, metadata->tex_id);

  gl->TexParameterf (GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->TexParameterf (GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl->TexParameteri (GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
      GL_CLAMP_TO_EDGE);
  gl->TexParameteri (GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
      GL_CLAMP_TO_EDGE);

  droidmediabuffertoglmemory->glEGLImageTargetTexture2DOES
      (GL_TEXTURE_EXTERNAL_OES, metadata->image);

  if (!gst_gl_context_activate (context, FALSE)) {
    GST_WARNING_OBJECT (droidmediabuffertoglmemory,
        "cannot deactivate this element's context");
  }

  return;
}

static void
_delete_texture_thread (GstGLContext * context, gpointer data)
{
  struct _WrappedTextureMetadata *metadata = data;
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory = metadata->self;
  GstGLFuncs *gl = droidmediabuffertoglmemory->context->gl_vtable;

  if (!gst_gl_context_activate (context, TRUE)) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "cannot activate this element's context.");
    return;
  }

  gl->DeleteTextures ( /* n */ 1, &metadata->tex_id);

  if (!gst_gl_context_activate (droidmediabuffertoglmemory->context, FALSE)) {
    GST_WARNING_OBJECT (droidmediabuffertoglmemory,
        "cannot deactivate this element's context");
  }
}

static void
_on_glmemory_destroy (gpointer data)
{
  struct _WrappedTextureMetadata *metadata = data;
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory = metadata->self;

  _destroy_egl_image (droidmediabuffertoglmemory, metadata->image);

  gst_gl_context_thread_add (droidmediabuffertoglmemory->context,
      _delete_texture_thread, metadata);

  g_free (metadata);
}

static GstFlowReturn
gst_droidmediabuffertoglmemory_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "prepare_output_buffer");

  GstMemory *droidmem =
      _get_droid_media_buffer_memory (droidmediabuffertoglmemory, input);

  if (!droidmem) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "buffer is not DroidMediaBufferMemory");
    return GST_FLOW_NOT_NEGOTIATED;
  }

  EGLImage image = _create_egl_image (droidmediabuffertoglmemory, droidmem);

  if (image == EGL_NO_IMAGE_KHR) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "unable to create GstEGLImage.");
    return GST_FLOW_ERROR;
  }

  struct _WrappedTextureMetadata *metadata =
      g_new (struct _WrappedTextureMetadata, 1);
  metadata->self = droidmediabuffertoglmemory;
  metadata->image = image;
  metadata->tex_id = 0;

  gst_gl_context_thread_add (droidmediabuffertoglmemory->context,
      _create_texture_from_image_thread, metadata);

  if (!metadata->tex_id) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "unable to create texture from image.");
    _destroy_egl_image (droidmediabuffertoglmemory, image);
    g_free (metadata);
    return GST_FLOW_ERROR;
  }

  GstGLBaseMemoryAllocator *allocator =
      GST_GL_BASE_MEMORY_ALLOCATOR (gst_allocator_find
      (GST_GL_MEMORY_ALLOCATOR_NAME));

  GstGLVideoAllocationParams *alloc_params =
      gst_gl_video_allocation_params_new_wrapped_gl_handle
      (droidmediabuffertoglmemory->context,
      /* parent alloc_params */ NULL,
      &droidmediabuffertoglmemory->out_vinfo,
      /* plane (???) */ 0,
      /* valign (???) */ NULL,
      GST_GL_TEXTURE_TARGET_EXTERNAL_OES,
      /* tex_format (???) */ GST_GL_RGBA,
      GUINT_TO_POINTER (metadata->tex_id),
      metadata,
      _on_glmemory_destroy);

  GstGLBaseMemory *glmem = gst_gl_base_memory_alloc (allocator,
      (GstGLAllocationParams *) alloc_params);

  gst_gl_allocation_params_free ((GstGLAllocationParams *) alloc_params);
  gst_object_unref (allocator);

  if (!glmem) {
    GST_ERROR_OBJECT (droidmediabuffertoglmemory,
        "unable to allocate GstGLMemoryEGL wrapper");
    g_free (metadata);
    return GST_FLOW_ERROR;
  }

  *outbuf = gst_buffer_new ();
  gst_buffer_append_memory (*outbuf, GST_MEMORY_CAST (glmem));

  return GST_FLOW_OK;
}

/* transform */
static GstFlowReturn
gst_droidmediabuffertoglmemory_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "transform");

  // Do nothing because prepare_output_buffer already transform the buffer,
  // but cannot be removed because it's required by GstBaseTransform.

  return GST_FLOW_OK;
}
